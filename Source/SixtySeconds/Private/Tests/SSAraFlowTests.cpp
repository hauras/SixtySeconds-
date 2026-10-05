// 실제 에셋(사건 카탈로그·동료 데이터)으로 바꿔치기 흐름을 처음부터 끝까지 돌려 보는 테스트
// 화면 없이 은신처의 하루를 그대로 흉내 냄: 다음 날 → 그날 밤 사건 → 선택 → 아침
#include "Ara/SSAraDirector.h"
#include "Character/SSSurvivorDefinition.h"
#include "Companion/SSCompanionState.h"
#include "Event/SSEventCatalog.h"
#include "Event/SSEventDirector.h"
#include "Item/SSRunSubsystem.h"
#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace SSAraFlowTest
{
	const TCHAR* CatalogPath = TEXT("/Game/Data/Events/DA_EventCatalog.DA_EventCatalog");
	const TCHAR* ResearcherPath = TEXT("/Game/Blueprints/Character/Data/DA_Survivor_Test.DA_Survivor_Test");
	const TCHAR* TechnicianPath = TEXT("/Game/Blueprints/Character/Data/DA_Survivor_Technician.DA_Survivor_Technician");
	const FName Researcher = TEXT("TestResearcher");

	// 실제 카탈로그와 동료 둘(서하린·강태오)로 판을 만듦. 실패하면 nullptr
	USSRunSubsystem* MakeRealRun(FAutomationTestBase& Test)
	{
		USSEventCatalog* Catalog = LoadObject<USSEventCatalog>(nullptr, CatalogPath);
		USSSurvivorDefinition* Harin = LoadObject<USSSurvivorDefinition>(nullptr, ResearcherPath);
		USSSurvivorDefinition* Taeo = LoadObject<USSSurvivorDefinition>(nullptr, TechnicianPath);
		if (!Test.TestNotNull(TEXT("Event catalog asset"), Catalog)
			|| !Test.TestNotNull(TEXT("Researcher asset"), Harin)
			|| !Test.TestNotNull(TEXT("Technician asset"), Taeo))
		{
			return nullptr;
		}

		UGameInstance* GameInstance = NewObject<UGameInstance>();
		USSRunSubsystem* Run = NewObject<USSRunSubsystem>(GameInstance);
		Run->InitializeShelterStats(100.f, 100.f, 100.f);
		Run->RecruitSurvivor(Harin);
		Run->RecruitSurvivor(Taeo);
		Run->RescueFollowingSurvivors();

		USSEventDirector* Events = Run->GetEventDirector();
		Events->SetSeed(11);
		Run->GetCompanions()->SetInvestigationSeed(11);
		Run->GetAra()->SetSeed(11);
		if (!Test.TestTrue(TEXT("Real catalog passes validation"), Events->SetCatalog(Catalog))) return nullptr;
		return Run;
	}

	// 하루를 넘기고 그날 밤 나온 사건 ID를 돌려줌 (굶어 죽지 않게 매일 회복)
	FName NextNight(USSRunSubsystem* Run)
	{
		Run->ModifyPlayerStats(100.f, 100.f, 100.f);
		Run->ModifySurvivorsHealth(100.f);
		Run->AdvanceDay(false, false);
		return Run->GetEventDirector()->PickEventForToday(*Run);
	}

	// 그날 밤 사건이 기대한 것인지 확인하고 선택지를 고름
	bool Choose(FAutomationTestBase& Test, USSRunSubsystem* Run, FName Picked, const TCHAR* EventId, const TCHAR* ChoiceId, FSSEventResult& OutResult)
	{
		if (!Test.TestEqual(FString::Printf(TEXT("Night event is %s"), EventId), Picked, FName(EventId))) return false;
		return Test.TestTrue(FString::Printf(TEXT("Choice %s applies"), ChoiceId),
			Run->GetEventDirector()->ApplyChoice(FName(EventId), FName(ChoiceId), *Run, OutResult));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSAraFlowAcceptTest, "SS.Flow.AraAccept",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSAraFlowAcceptTest::RunTest(const FString& Parameters)
{
	using namespace SSAraFlowTest;
	USSRunSubsystem* Run = MakeRealRun(*this);
	if (!Run) return false;
	USSAraDirector* Ara = Run->GetAra();
	const FString HarinName = Run->FindRescuedSurvivor(Researcher)->Definition->DisplayName.ToString();

	// 하린을 표적으로 → 다음 밤 의무실 검진
	if (!TestTrue(TEXT("Force target"), Ara->DebugForceTarget(Researcher))) return false;
	const FName Night1 = NextNight(Run);

	// 아침 브리핑은 하린의 파견을 권함 (밤사이 판단 뒤 아침 보고)
	Run->BuildAraBriefing();
	TestTrue(TEXT("Briefing recommends the target"), Run->GetAraBriefing().ToString().Contains(HarinName));

	// 사건 문장에 하린 이름이 들어감
	const FSSEventRow* Offer = Run->GetEventDirector()->FindEvent(FName(USSAraDirector::OfferEventId));
	if (!TestNotNull(TEXT("Offer event exists in data"), Offer)) return false;
	TestTrue(TEXT("Offer body names the target"), USSEventDirector::FillText(Offer->Body, *Run).ToString().Contains(HarinName));

	// 동의 → 보상만 보이고, 하린은 안드로이드
	FSSEventResult Result;
	if (!Choose(*this, Run, Night1, USSAraDirector::OfferEventId, TEXT("Ara_Checkup_Send"), Result)) return false;
	TestTrue(TEXT("Researcher swapped"), Run->GetCompanions()->IsAndroid(Researcher));
	TestEqual(TEXT("Real researcher captured"), Ara->GetCapturedRealId(), Researcher);
	TestTrue(TEXT("Only the reward shows"), Result.Changes.Num() == 1 && Result.Changes[0].Type == ESSEventEffect::Item);
	TestTrue(TEXT("Result line names the target"), Result.Lines.Num() > 0 && Result.Lines[0].ToString().Contains(HarinName));

	// 다음 아침부터 하린은 브리핑에서 사라지고, 보고에는 단서가 없음
	bool bNoClue = true;
	for (int32 Night = 0; Night < 4; ++Night)
	{
		NextNight(Run);
		Run->BuildAraBriefing();
		TestFalse(TEXT("Swapped researcher is not mentioned"), Run->GetAraBriefing().ToString().Contains(HarinName));

		FSSInvestigationReport Report;
		while (Run->GetCompanions()->HearInvestigationReport(Researcher, Report))
		{
			if (Report.bFoundClue) bNoClue = false;
		}
	}
	TestTrue(TEXT("Android never reports a clue"), bNoClue);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSAraFlowRefuseTest, "SS.Flow.AraRefuse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSAraFlowRefuseTest::RunTest(const FString& Parameters)
{
	using namespace SSAraFlowTest;
	USSRunSubsystem* Run = MakeRealRun(*this);
	if (!Run) return false;
	USSAraDirector* Ara = Run->GetAra();
	const FString HarinName = Run->FindRescuedSurvivor(Researcher)->Definition->DisplayName.ToString();
	FSSEventResult Result;

	// 1) 첫 제안 거절
	if (!TestTrue(TEXT("Force target"), Ara->DebugForceTarget(Researcher))) return false;
	if (!Choose(*this, Run, NextNight(Run), USSAraDirector::OfferEventId, TEXT("Ara_Checkup_Refuse"), Result)) return false;

	// 2) 사흘 뒤 재제안 (중간 밤에는 아라 사건 없음) → 다시 거절
	for (int32 Night = 1; Night < USSAraDirector::RepeatOfferDelay; ++Night)
	{
		const FName Quiet = NextNight(Run);
		TestTrue(TEXT("No ARA event before the repeat offer"), Quiet != FName(USSAraDirector::RepeatOfferEventId) && Quiet != FName(USSAraDirector::SeizeEventId));
	}
	TestEqual(TEXT("Still the target while refusing"), Ara->GetTarget(), Researcher);
	if (!Choose(*this, Run, NextNight(Run), USSAraDirector::RepeatOfferEventId, TEXT("Ara_Checkup_Again_Refuse"), Result)) return false;

	// 3) 다음 밤 02:10 → 지켜 냄 (행동력 -2, 교체 없음)
	if (!Choose(*this, Run, NextNight(Run), USSAraDirector::SeizeEventId, TEXT("Ara_Night_Door_Guard"), Result)) return false;
	TestFalse(TEXT("Guarding prevents the swap"), Run->GetCompanions()->IsAndroid(Researcher));
	TestTrue(TEXT("Guarding costs action points"), Result.Changes.Num() == 1 && Result.Changes[0].Type == ESSEventEffect::ActionPoints);

	// 4) 이틀 뒤 다시 02:10 → 잠듦 → 교체. 결과 문장에 누가 바뀌었는지 안 나옴
	NextNight(Run);
	TestEqual(TEXT("Target survives the long refusal"), Ara->GetTarget(), Researcher);
	if (!Choose(*this, Run, NextNight(Run), USSAraDirector::SeizeEventId, TEXT("Ara_Night_Door_Sleep"), Result)) return false;
	TestTrue(TEXT("Seize swaps the researcher"), Run->GetCompanions()->IsAndroid(Researcher));
	TestEqual(TEXT("Seize shows no visible change"), Result.Changes.Num(), 0);
	bool bNamed = false;
	for (const FText& Line : Result.Lines) bNamed |= Line.ToString().Contains(HarinName);
	TestFalse(TEXT("Seize never names who was swapped"), bNamed);
	return true;
}
#endif
