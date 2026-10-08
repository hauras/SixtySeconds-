#include "Ara/SSAraDirector.h"
#include "Character/SSSurvivorDefinition.h"
#include "Companion/SSCompanionState.h"
#include "Item/SSInventoryTypes.h"
#include "Item/SSItemDefinition.h"
#include "Item/SSRunSubsystem.h"
#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace SSIdentityTest
{
	// 구조된 동료 둘(Researcher, Technician)이 있는 판
	USSRunSubsystem* MakeRun()
	{
		UGameInstance* GameInstance = NewObject<UGameInstance>();
		USSRunSubsystem* Run = NewObject<USSRunSubsystem>(GameInstance);
		Run->InitializeShelterStats(100.f, 100.f, 100.f);
		for (const TCHAR* Id : {TEXT("Researcher"), TEXT("Technician")})
		{
			USSSurvivorDefinition* Survivor = NewObject<USSSurvivorDefinition>(Run);
			Survivor->SurvivorId = Id;
			Survivor->DisplayName = FText::FromString(Id);
			Survivor->AraInfluence = 1.2f;
			Run->RecruitSurvivor(Survivor);
		}
		Run->RescueFollowingSurvivors();
		return Run;
	}

	// 배터리를 보관함에 넣음
	void AddBatteries(USSRunSubsystem* Run, int32 Count)
	{
		USSItemDefinition* Battery = NewObject<USSItemDefinition>(Run);
		Battery->ItemId = SSItemIds::Battery;
		Battery->DisplayName = FText::FromString(TEXT("Battery"));
		FSSItemStack Stack;
		Stack.Item = Battery;
		Stack.Quantity = Count;
		Run->DepositItems({Stack});
	}

	bool IsCaptured(const USSRunSubsystem* Run, FName Id)
	{
		return Run->IsSurvivorCaptured(Id);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSInspectTest, "SS.Identity.Inspect",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSInspectTest::RunTest(const FString& Parameters)
{
	using namespace SSIdentityTest;
	USSRunSubsystem* Run = MakeRun();
	USSCompanionState* Companions = Run->GetCompanions();
	const FName Researcher = TEXT("Researcher");

	// 배터리가 없으면 검사 못 함 (행동력도 그대로)
	const int32 ApStart = Run->GetActionPoints();
	TestFalse(TEXT("No battery, no inspection"), Companions->InspectCompanion(Researcher));
	TestEqual(TEXT("Failed inspection costs nothing"), Run->GetActionPoints(), ApStart);

	// 사람 검사: 행동력 1 · 배터리 1, 결과 '정상'
	AddBatteries(Run, 2);
	TestTrue(TEXT("Inspect human"), Companions->InspectCompanion(Researcher));
	TestEqual(TEXT("Inspection costs action points"), Run->GetActionPoints(), ApStart - USSCompanionState::InspectActionCost);
	TestEqual(TEXT("Inspection costs a battery"), Run->GetStoredQuantityById(SSItemIds::Battery), 1);
	const FSSCompanionRecord* Record = Companions->FindRecord(Researcher);
	if (!TestNotNull(TEXT("Record after inspection"), Record)) return false;
	TestEqual(TEXT("Inspection day recorded"), Record->InspectedDay, Run->GetCurrentDay());
	TestFalse(TEXT("Human looks human"), Record->bInspectedAndroid);

	// 배터리를 다 쓰면 더 못 함
	TestTrue(TEXT("One more inspection"), Companions->InspectCompanion(TEXT("Technician")));
	TestFalse(TEXT("Out of batteries"), Companions->CanInspect(Researcher));

	// 반복 검사: 사람은 한 번도 이상이 아니고, 안드로이드는 위장(40%) 때문에 가끔만 이상
	Companions->SetInvestigationSeed(9);
	Companions->MakeAndroid(Researcher);
	constexpr int32 Tries = 200;
	AddBatteries(Run, Tries * 2);
	int32 AndroidRevealed = 0;
	int32 HumanRevealed = 0;
	for (int32 Try = 0; Try < Tries; ++Try)
	{
		Run->AdjustActionPoints(USSRunSubsystem::MaxActionPoints);
		Companions->InspectCompanion(Researcher);
		if (Companions->FindRecord(Researcher)->bInspectedAndroid) ++AndroidRevealed;
		Companions->InspectCompanion(TEXT("Technician"));
		if (Companions->FindRecord(TEXT("Technician"))->bInspectedAndroid) ++HumanRevealed;
	}
	TestEqual(TEXT("Humans never look like androids"), HumanRevealed, 0);
	// 기대 60% (120번). 시드 고정이라 결과는 늘 같음
	TestTrue(TEXT("Android is caught most of the time, not always"), AndroidRevealed > Tries * 4 / 10 && AndroidRevealed < Tries * 8 / 10);

	// 결과 문장이 서로 다름
	TestFalse(TEXT("Two different results"), USSCompanionState::GetInspectionText(true).EqualTo(USSCompanionState::GetInspectionText(false)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSIsolateTest, "SS.Identity.Isolate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSIsolateTest::RunTest(const FString& Parameters)
{
	using namespace SSIdentityTest;
	USSRunSubsystem* Run = MakeRun();
	USSCompanionState* Companions = Run->GetCompanions();
	USSAraDirector* Ara = Run->GetAra();
	const FName Researcher = TEXT("Researcher");
	const FName Technician = TEXT("Technician");

	// 아라가 연구원을 바꿔치기 → 진짜는 붙잡힌 목록, 은신처엔 같은 이름의 안드로이드
	TestTrue(TEXT("Force target"), Ara->DebugForceTarget(Researcher));
	TestTrue(TEXT("Swap"), Ara->SwapTarget());
	TestTrue(TEXT("Real researcher is captured"), IsCaptured(Run, Researcher));
	TestTrue(TEXT("Android stays in the shelter"), Run->IsSurvivorRescued(Researcher));

	// 같은 동료는 한 번만 붙잡힘
	TestFalse(TEXT("No duplicate capture"), Run->CopySurvivorToCaptured(Researcher));
	TestEqual(TEXT("One captured researcher"), Run->GetCapturedSurvivors().Num(), 1);

	// 안드로이드 격리 → 은신처에서 제거, 진짜는 여전히 붙잡혀 있음
	bool bWasAndroid = false;
	TestTrue(TEXT("Isolate android"), Companions->IsolateCompanion(Researcher, bWasAndroid));
	TestTrue(TEXT("It was the android"), bWasAndroid);
	TestFalse(TEXT("Android removed"), Run->IsSurvivorRescued(Researcher));
	TestTrue(TEXT("Real one still captured"), IsCaptured(Run, Researcher));
	TestFalse(TEXT("Record cleared for a later rescue"), Companions->IsAndroid(Researcher));

	// 사람 격리 → 은신처에서 사라지고 붙잡힌 목록으로 (사망이 아님)
	TestTrue(TEXT("Isolate human"), Companions->IsolateCompanion(Technician, bWasAndroid));
	TestFalse(TEXT("It was a human"), bWasAndroid);
	TestFalse(TEXT("Human leaves the shelter"), Run->IsSurvivorRescued(Technician));
	TestTrue(TEXT("Human is captured, not dead"), IsCaptured(Run, Technician));
	TestEqual(TEXT("Two people in B2"), Run->GetCapturedSurvivors().Num(), 2);

	// 다시 격리할 수 없음 (은신처에 없음)
	TestFalse(TEXT("Cannot isolate twice"), Companions->IsolateCompanion(Technician, bWasAndroid));

	// 격리된 사람은 밤 조사에서 빠짐
	Companions->RunNight();
	const FSSCompanionRecord* TechRecord = Companions->FindRecord(Technician);
	TestTrue(TEXT("Isolated survivor does not investigate"), !TechRecord || TechRecord->PendingReports.Num() == 0);

	// 새 게임이면 붙잡힌 목록도 비움
	Run->ResetRun();
	TestEqual(TEXT("Reset clears captured"), Run->GetCapturedSurvivors().Num(), 0);
	return true;
}
#endif
