#include "Ara/SSAraDirector.h"
#include "Character/SSSurvivorDefinition.h"
#include "Companion/SSCompanionState.h"
#include "Event/SSEventCatalog.h"
#include "Event/SSEventDirector.h"
#include "Item/SSRunSubsystem.h"
#include "Rescue/SSRescueState.h"
#include "Ending/SSEndingState.h"
#include "UI/Ending/SSEndingWidget.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace SSEndingTest
{
	const FName Researcher = TEXT("TestResearcher");

	// 은신처에 하린(TestResearcher)과 태오가 있는 판
	// AraInfluence 0이면 밤 조사로 의심이 쌓여도 위협도가 0이라 아라가 스스로 표적을 못 정함 (진행 보장만 확인)
	USSRunSubsystem* MakeRun(float AraInfluence = 1.f)
	{
		UGameInstance* GameInstance = NewObject<UGameInstance>();
		USSRunSubsystem* Run = NewObject<USSRunSubsystem>(GameInstance);
		Run->InitializeShelterStats(100.f, 100.f, 100.f);
		for (const TCHAR* Id : {TEXT("TestResearcher"), TEXT("Technician")})
		{
			USSSurvivorDefinition* Survivor = NewObject<USSSurvivorDefinition>(Run);
			Survivor->SurvivorId = Id;
			Survivor->DisplayName = FText::FromString(Id);
			Survivor->AraInfluence = AraInfluence;
			Run->RecruitSurvivor(Survivor);
		}
		Run->RescueFollowingSurvivors();
		return Run;
	}

	// 숨은 진실 2개를 앎
	void LearnBothTruths(USSRunSubsystem* Run)
	{
		Run->GetEnding()->AddHiddenTruth(SSRescueIds::PanelLogTruth(), FText::GetEmpty());
		Run->GetEnding()->AddHiddenTruth(SSRescueIds::TestimonyTruth(), FText::GetEmpty());
	}

	// 서버실 사건만 있는 카탈로그 (실제 CSV와 같은 선택지 조건·효과)
	USSEventCatalog* MakeFinalCatalog()
	{
		USSEventCatalog* Catalog = NewObject<USSEventCatalog>();
		Catalog->DailyEventChance = 0.f; // 평소 사건은 안 나오게

		UDataTable* Events = NewObject<UDataTable>();
		Events->RowStruct = FSSEventRow::StaticStruct();
		FSSEventRow Final;
		Final.bScheduledOnly = true;
		Events->AddRow(USSEventDirector::GetFinalEventId(), Final);

		UDataTable* Choices = NewObject<UDataTable>();
		Choices->RowStruct = FSSEventChoiceRow::StaticStruct();
		const auto AddChoice = [Choices](const TCHAR* Id, int32 Order, ESSEventCondition Condition, FName Target, int32 Amount)
		{
			FSSEventChoiceRow Row;
			Row.EventId = USSEventDirector::GetFinalEventId();
			Row.Order = Order;
			Row.Text = FText::FromString(Id);
			Row.Condition = Condition;
			Row.ConditionTarget = Target;
			Row.ConditionAmount = Amount;
			Choices->AddRow(Id, Row);
		};
		AddChoice(TEXT("ServerRoom_Shutdown"), 0, ESSEventCondition::SurvivorAlive, Researcher, 0);
		AddChoice(TEXT("ServerRoom_Trust"), 1, ESSEventCondition::HiddenTruthCount, NAME_None, 2);
		AddChoice(TEXT("ServerRoom_Endure"), 2, ESSEventCondition::None, NAME_None, 0);

		UDataTable* Effects = NewObject<UDataTable>();
		Effects->RowStruct = FSSEventEffectRow::StaticStruct();
		const auto AddEffect = [Effects](const TCHAR* Id, const TCHAR* ChoiceId, const TCHAR* Ending)
		{
			FSSEventEffectRow Row;
			Row.ChoiceId = ChoiceId;
			Row.Type = ESSEventEffect::Ending;
			Row.Target = Ending;
			Effects->AddRow(Id, Row);
		};
		AddEffect(TEXT("E1"), TEXT("ServerRoom_Shutdown"), TEXT("Resolve"));
		AddEffect(TEXT("E2"), TEXT("ServerRoom_Trust"), TEXT("Reversal"));
		AddEffect(TEXT("E3"), TEXT("ServerRoom_Endure"), TEXT("Dominion"));

		Catalog->EventTable = Events;
		Catalog->ChoiceTable = Choices;
		Catalog->EffectTable = Effects;
		return Catalog;
	}

	// 선택지가 열려 있나
	bool IsChoiceOpen(const USSEventDirector* Director, const USSRunSubsystem* Run, FName ChoiceId)
	{
		for (const FSSEventChoiceView& View : Director->GetChoices(USSEventDirector::GetFinalEventId(), *Run))
		{
			if (View.ChoiceId == ChoiceId) return View.bAvailable;
		}
		return false;
	}

	// 하루씩 넘김 (굶지 않게 매일 채움)
	void AdvanceTo(USSRunSubsystem* Run, int32 Day)
	{
		while (Run->GetCurrentDay() < Day)
		{
			Run->ModifyPlayerStats(100.f, 100.f, 100.f);
			Run->ModifySurvivorsHealth(100.f); // 배급 없이 넘겨도 동료가 죽지 않게
			Run->AdvanceDay(false, false);
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSEndingResolveTest, "SS.Ending.Resolve",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSEndingResolveTest::RunTest(const FString& Parameters)
{
	using namespace SSEndingTest;

	// 하린(사람)과 종료 → 해결
	USSRunSubsystem* Run = MakeRun();
	TestFalse(TEXT("Not ended at start"), Run->GetEnding()->HasEnded());
	Run->GetEnding()->ReachEnding(ESSEnding::Resolve);
	TestTrue(TEXT("Resolve"), Run->GetEnding()->GetReport().Ending == ESSEnding::Resolve);
	TestFalse(TEXT("Truth unknown"), Run->GetEnding()->GetReport().bKnewTruth);

	// 한 번만: 다른 엔딩으로 덮어쓰지 않음
	Run->GetEnding()->ReachEnding(ESSEnding::Dominion);
	TestTrue(TEXT("Ending fixed once"), Run->GetEnding()->GetReport().Ending == ESSEnding::Resolve);

	// 진실을 알고 종료하면 정화 프로토콜 한 줄이 붙음
	USSRunSubsystem* Knowing = MakeRun();
	Knowing->GetEnding()->AddHiddenTruth(SSRescueIds::PanelLogTruth(), FText::GetEmpty());
	Knowing->GetEnding()->ReachEnding(ESSEnding::Resolve);
	TestTrue(TEXT("Knew truth"), Knowing->GetEnding()->GetReport().bKnewTruth);
	const FString PlainBody = USSEndingWidget::GetEndingBody(Run->GetEnding()->GetReport()).ToString();
	const FString TruthBody = USSEndingWidget::GetEndingBody(Knowing->GetEnding()->GetReport()).ToString();
	TestTrue(TEXT("Truth adds a line to the resolve ending"), TruthBody.Len() > PlainBody.Len() && TruthBody.StartsWith(PlainBody));

	// 데려간 하린이 안드로이드면 종료 실패 → 지배 (방해)
	USSRunSubsystem* Swapped = MakeRun();
	Swapped->GetCompanions()->MakeAndroid(Researcher);
	Swapped->GetEnding()->ReachEnding(ESSEnding::Resolve);
	TestTrue(TEXT("Android Harin turns resolve into dominion"), Swapped->GetEnding()->GetReport().Ending == ESSEnding::Dominion);
	TestTrue(TEXT("Marked as sabotaged"), Swapped->GetEnding()->GetReport().bSabotaged);

	// 새 판이면 엔딩도 지워짐
	Run->ResetRun();
	TestFalse(TEXT("Reset clears the ending"), Run->GetEnding()->HasEnded());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSEndingServerRoomTest, "SS.Ending.ServerRoom",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSEndingServerRoomTest::RunTest(const FString& Parameters)
{
	using namespace SSEndingTest;
	const FName Final = USSEventDirector::GetFinalEventId();

	USSRunSubsystem* Run = MakeRun();
	USSEventDirector* Director = Run->GetEventDirector();
	if (!TestTrue(TEXT("Final catalog accepted"), Director->SetCatalog(MakeFinalCatalog()))) return false;

	// 마지막 날 전에는 안 나옴
	AdvanceTo(Run, USSEndingState::FinalDay - 1);
	TestTrue(TEXT("No server room before the final day"), Director->PickEventForToday(*Run).IsNone());

	// 마지막 날 밤: 확률(0)과 상관없이 서버실
	AdvanceTo(Run, USSEndingState::FinalDay);
	TestTrue(TEXT("Server room on the final day"), Director->PickEventForToday(*Run) == Final);
	TestTrue(TEXT("Only once"), Director->PickEventForToday(*Run).IsNone());

	// 선택지 조건: 하린이 있으면 종료, 진실 2개면 믿기, 버티기는 항상
	TestTrue(TEXT("Shutdown open with Harin"), IsChoiceOpen(Director, Run, TEXT("ServerRoom_Shutdown")));
	TestFalse(TEXT("Trust closed without truths"), IsChoiceOpen(Director, Run, TEXT("ServerRoom_Trust")));
	TestTrue(TEXT("Endure always open"), IsChoiceOpen(Director, Run, TEXT("ServerRoom_Endure")));
	LearnBothTruths(Run);
	TestTrue(TEXT("Trust open with two truths"), IsChoiceOpen(Director, Run, TEXT("ServerRoom_Trust")));

	// 하린이 붙잡혀 있으면 종료 선택지 닫힘
	USSRunSubsystem* NoHarin = MakeRun();
	NoHarin->GetEventDirector()->SetCatalog(MakeFinalCatalog());
	NoHarin->MoveSurvivorToCaptured(Researcher);
	TestFalse(TEXT("Shutdown closed without Harin"), IsChoiceOpen(NoHarin->GetEventDirector(), NoHarin, TEXT("ServerRoom_Shutdown")));

	// 선택하면 엔딩 효과로 확정 (결과 칩에는 안 나옴)
	FSSEventResult Result;
	TestTrue(TEXT("Apply trust"), Director->ApplyChoice(Final, TEXT("ServerRoom_Trust"), *Run, Result));
	TestTrue(TEXT("Reversal ending"), Run->GetEnding()->GetReport().Ending == ESSEnding::Reversal);
	TestEqual(TEXT("Ending is not a visible change"), Result.Changes.Num(), 0);
	TestEqual(TEXT("Days survived recorded"), Run->GetEnding()->GetReport().DaysSurvived, USSEndingState::FinalDay);

	// 엔딩마다 제목·문장이 있음
	for (const ESSEnding Ending : {ESSEnding::Resolve, ESSEnding::Dominion, ESSEnding::Reversal})
	{
		FSSEndingReport Report;
		Report.Ending = Ending;
		TestFalse(TEXT("Ending has a title"), USSEndingWidget::GetEndingTitle(Report).IsEmpty());
		TestFalse(TEXT("Ending has a body"), USSEndingWidget::GetEndingBody(Report).IsEmpty());
	}
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSAraForcedTargetTest, "SS.Ara.ForcedTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSAraForcedTargetTest::RunTest(const FString& Parameters)
{
	using namespace SSEndingTest;

	// 표적 판단은 그날 밤(날짜가 넘어가기 직전)에 함.
	// 아무 의심도 안 쌓인 판: 정해진 날이 되기까지는 표적 없음
	USSRunSubsystem* Run = MakeRun(0.f);
	USSAraDirector* Ara = Run->GetAra();
	AdvanceTo(Run, USSAraDirector::ForceTargetDay);
	TestFalse(TEXT("No target before the forced day's night"), Ara->HasTarget());

	// 정해진 날의 밤이 지나면: 진행 보장으로 표적이 생기고, 검진 제안이 예약됨
	AdvanceTo(Run, USSAraDirector::ForceTargetDay + 1);
	TestTrue(TEXT("Forced target on the day"), Ara->HasTarget());
	TestTrue(TEXT("Offer scheduled"), Run->GetEventDirector()->FindScheduledDay(FName(USSAraDirector::OfferEventId)) > 0);

	// 의심이 낮아도 다음 밤에 풀리지 않음
	const FName Target = Ara->GetTarget();
	AdvanceTo(Run, USSAraDirector::ForceTargetDay + 2);
	TestTrue(TEXT("Forced target kept"), Ara->GetTarget() == Target);

	// 거절 → 재제안 → 강제 교체까지 걸려도 마지막 밤 전에 끝날 수 있음 (예약은 표적을 정한 밤의 날짜 기준)
	const int32 LatestSwapDay = USSAraDirector::ForceTargetDay + USSAraDirector::OfferDelay + USSAraDirector::RepeatOfferDelay + USSAraDirector::SeizeDelay;
	TestTrue(TEXT("Swap flow fits before the final night"), LatestSwapDay < USSEndingState::FinalDay);

	// 이미 바꿨으면 진행 보장으로 새 표적을 잡지 않음
	USSRunSubsystem* Swapped = MakeRun();
	Swapped->GetAra()->DebugForceTarget(Researcher);
	Swapped->GetAra()->SwapTarget();
	AdvanceTo(Swapped, USSAraDirector::ForceTargetDay + 2);
	TestFalse(TEXT("No forced target after a swap"), Swapped->GetAra()->HasTarget());
	return true;
}
#endif
