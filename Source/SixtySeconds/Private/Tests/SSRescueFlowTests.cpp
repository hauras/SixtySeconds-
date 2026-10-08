#include "Ara/SSAraDirector.h"
#include "Character/SSSurvivorDefinition.h"
#include "Companion/SSCompanionState.h"
#include "Item/SSRunSubsystem.h"
#include "Ending/SSEndingState.h"
#include "Rescue/SSRescueState.h"
#include "Rescue/SSRescueSession.h"
#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace SSRescueFlowTest
{
	const FName Researcher = TEXT("TestResearcher");

	// 은신처에 하린(TestResearcher)과 태오(Technician)가 있는 판
	USSRunSubsystem* MakeRun()
	{
		UGameInstance* GameInstance = NewObject<UGameInstance>();
		USSRunSubsystem* Run = NewObject<USSRunSubsystem>(GameInstance);
		Run->InitializeShelterStats(100.f, 100.f, 100.f);
		for (const TCHAR* Id : { TEXT("TestResearcher"), TEXT("Technician") })
		{
			USSSurvivorDefinition* Survivor = NewObject<USSSurvivorDefinition>(Run);
			Survivor->SurvivorId = Id;
			Survivor->DisplayName = FText::FromString(Id);
			Survivor->AraInfluence = 1.f;
			Run->RecruitSurvivor(Survivor);
		}
		Run->RescueFollowingSurvivors();
		return Run;
	}

	// 정답 경로 칸만 맞춰서 끝냄
	void Solve(USSRescueSession* Session)
	{
		const FSSPowerGrid& Grid = Session->GetGrid();
		for (const int32 Cell : Grid.FindSolutionPath())
		{
			const int32 Turns = Grid.TurnsToSolve(Cell);
			for (int32 Turn = 0; Turn < Turns && !Session->IsFinished(); ++Turn)
			{
				Session->Rotate(Cell);
			}
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSRescueFlowTest, "SS.Rescue.Flow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSRescueFlowTest::RunTest(const FString& Parameters)
{
	using namespace SSRescueFlowTest;
	const FName Technician = SSRescueIds::Technician();

	// ── 열림 조건 ──
	USSRunSubsystem* Run = MakeRun();
	TestTrue(TEXT("Nobody captured"), Run->GetRescue()->GetBlock() == ESSRescueBlock::NobodyCaptured);

	Run->MoveSurvivorToCaptured(Researcher);
	TestTrue(TEXT("Route unknown"), Run->GetRescue()->GetBlock() == ESSRescueBlock::NoRoute);

	Run->GetCompanions()->DebugHearClue(SSRescueIds::RouteClue());
	TestTrue(TEXT("Can open the panel"), Run->GetRescue()->GetBlock() == ESSRescueBlock::None);
	TestNull(TEXT("Cannot target someone not captured"), Run->GetRescue()->Start(Technician, 5));

	// ── 태오가 도우면 행동력 × 8, 행동력은 전부 씀 ──
	USSRescueSession* Session = Run->GetRescue()->Start(Researcher, 5);
	if (!TestNotNull(TEXT("Rescue started"), Session)) return false;
	TestEqual(TEXT("Technician helps: 5 AP x 8"), Session->GetMoveBudget(), 40);
	TestEqual(TEXT("All action points spent"), Run->GetActionPoints(), 0);
	TestTrue(TEXT("Panel log seen"), Run->GetEnding()->HasSeenPanelLog());
	TestEqual(TEXT("Panel log is hidden truth 1"), Run->GetEnding()->CountHiddenTruths(), 1);
	TestTrue(TEXT("Blocked while in progress"), Run->GetRescue()->GetBlock() == ESSRescueBlock::InProgress);

	FSSRescueReport Report;
	TestFalse(TEXT("Cannot finish while in progress"), Run->GetRescue()->Finish(Report));

	// ── 성공: 하린 귀환 ──
	const int32 JournalBefore = Run->GetJournalEntries().Num();
	Solve(Session);
	TestTrue(TEXT("Finish rescue"), Run->GetRescue()->Finish(Report));
	TestTrue(TEXT("Unlocked"), Report.Outcome == ESSRescueOutcome::Unlocked);
	TestTrue(TEXT("Report names the target"), Report.TargetId == Researcher);
	TestFalse(TEXT("No android to replace"), Report.bReplacedAndroid);
	TestTrue(TEXT("Researcher back in the shelter"), Run->IsSurvivorRescued(Researcher));
	TestFalse(TEXT("No longer captured"), Run->IsSurvivorCaptured(Researcher));
	TestEqual(TEXT("Journal entry added"), Run->GetJournalEntries().Num(), JournalBefore + 1);
	TestNull(TEXT("Session cleared"), Run->GetRescue()->GetActive());
	TestEqual(TEXT("Report shows action points actually spent"), Report.ActionPointsSpent, 5);

	// 하린이 돌아오면 다음 대화에서 증언 (숨은 진실 2). "!"가 뜨고, 한 번만 들음
	const FSSCompanionRecord* HarinRecord = Run->GetCompanions()->FindRecord(Researcher);
	TestTrue(TEXT("Rescued Harin has something to say"), HarinRecord && HarinRecord->HasSomethingToSay());
	FText Testimony;
	TestTrue(TEXT("Testimony heard"), Run->GetCompanions()->HearTestimony(Researcher, Testimony));
	TestFalse(TEXT("Testimony line not empty"), Testimony.IsEmpty());
	TestFalse(TEXT("Testimony only once"), Run->GetCompanions()->HearTestimony(Researcher, Testimony));
	TestTrue(TEXT("Testimony truth added"), Run->GetEnding()->AddHiddenTruth(SSRescueIds::TestimonyTruth(), FText::FromString(TEXT("t"))));
	TestFalse(TEXT("Same truth not added twice"), Run->GetEnding()->AddHiddenTruth(SSRescueIds::TestimonyTruth(), FText::FromString(TEXT("t"))));
	TestEqual(TEXT("Two hidden truths"), Run->GetEnding()->CountHiddenTruths(), 2);

	// 결과 확정은 한 번만: 다시 불러도 아무 일 없음 (중복 귀환·기록·경보 없음)
	const int32 JournalAfter = Run->GetJournalEntries().Num();
	const int32 ShelterAfter = Run->GetRescuedSurvivors().Num();
	FSSRescueReport Again;
	TestFalse(TEXT("Second finish is refused"), Run->GetRescue()->Finish(Again));
	TestEqual(TEXT("No second journal entry"), Run->GetJournalEntries().Num(), JournalAfter);
	TestEqual(TEXT("No duplicate return"), Run->GetRescuedSurvivors().Num(), ShelterAfter);

	// ── 하루 한 번, 행동력 부족 ──
	Run->MoveSurvivorToCaptured(Researcher);
	Run->AdjustActionPoints(5);
	TestTrue(TEXT("Once per day"), Run->GetRescue()->GetBlock() == ESSRescueBlock::AlreadyToday);

	USSRunSubsystem* Tired = MakeRun();
	Tired->MoveSurvivorToCaptured(Researcher);
	Tired->GetCompanions()->DebugHearClue(SSRescueIds::RouteClue());
	Tired->AdjustActionPoints(-(Tired->GetActionPoints() - 1));
	TestTrue(TEXT("Needs at least 2 AP"), Tired->GetRescue()->GetBlock() == ESSRescueBlock::NotEnoughActionPoints);

	// 행동력 3으로 시작하면 보고서에 3 (회전 수 24와 다름)
	USSRunSubsystem* Partial = MakeRun();
	Partial->MoveSurvivorToCaptured(Researcher);
	Partial->GetCompanions()->DebugHearClue(SSRescueIds::RouteClue());
	Partial->AdjustActionPoints(-2);
	USSRescueSession* PartialSession = Partial->GetRescue()->Start(Researcher, 7);
	if (!TestNotNull(TEXT("Partial-AP rescue started"), PartialSession)) return false;
	TestEqual(TEXT("3 AP x 8 moves"), PartialSession->GetMoveBudget(), 24);
	PartialSession->Abort();
	FSSRescueReport PartialReport;
	Partial->GetRescue()->Finish(PartialReport);
	TestEqual(TEXT("Report keeps the 3 AP spent"), PartialReport.ActionPointsSpent, 3);

	// ── 태오가 붙잡혀 있으면 혼자: × 4, 중단하면 그대로 붙잡혀 있음 ──
	USSRunSubsystem* Alone = MakeRun();
	Alone->MoveSurvivorToCaptured(Technician);
	Alone->GetCompanions()->DebugHearClue(SSRescueIds::RouteClue());
	USSRescueSession* AloneSession = Alone->GetRescue()->Start(Technician, 5);
	if (!TestNotNull(TEXT("Alone rescue started"), AloneSession)) return false;
	TestEqual(TEXT("Alone: 5 AP x 4"), AloneSession->GetMoveBudget(), 20);
	AloneSession->Abort();
	TestTrue(TEXT("Finish aborted rescue"), Alone->GetRescue()->Finish(Report));
	TestTrue(TEXT("Aborted"), Report.Outcome == ESSRescueOutcome::Aborted);
	TestTrue(TEXT("Still captured after abort"), Alone->IsSurvivorCaptured(Technician));
	const FSSCompanionRecord* TaeoRecord = Alone->GetCompanions()->FindRecord(Technician);
	TestFalse(TEXT("No testimony for the technician"), TaeoRecord && TaeoRecord->bPendingTestimony);

	// ── 바꿔치기된 하린을 구하면: 은신처의 안드로이드가 진짜로 교체 ──
	USSRunSubsystem* Swapped = MakeRun();
	Swapped->CopySurvivorToCaptured(Researcher);
	Swapped->GetCompanions()->MakeAndroid(Researcher);
	Swapped->GetCompanions()->DebugHearClue(SSRescueIds::RouteClue());
	const int32 ShelterCount = Swapped->GetRescuedSurvivors().Num();
	USSRescueSession* SwapSession = Swapped->GetRescue()->Start(Researcher, 9);
	if (!TestNotNull(TEXT("Swap rescue started"), SwapSession)) return false;
	Solve(SwapSession);
	TestTrue(TEXT("Finish swap rescue"), Swapped->GetRescue()->Finish(Report));
	TestTrue(TEXT("Android replaced"), Report.bReplacedAndroid);
	TestEqual(TEXT("No duplicate in the shelter"), Swapped->GetRescuedSurvivors().Num(), ShelterCount);
	TestFalse(TEXT("The one in the shelter is human again"), Swapped->GetCompanions()->IsAndroid(Researcher));
	TestFalse(TEXT("Not captured"), Swapped->IsSurvivorCaptured(Researcher));

	// ── 안드로이드 태오가 도우면 시작부터 경보 1회 → 태오 의심 상승 ──
	USSRunSubsystem* AndroidHelper = MakeRun();
	AndroidHelper->MoveSurvivorToCaptured(Researcher);
	AndroidHelper->GetCompanions()->MakeAndroid(Technician);
	AndroidHelper->GetCompanions()->DebugHearClue(SSRescueIds::RouteClue());
	const float SuspicionBefore = AndroidHelper->GetAra()->GetSuspicion(Technician);
	USSRescueSession* AndroidSession = AndroidHelper->GetRescue()->Start(Researcher, 5);
	if (!TestNotNull(TEXT("Android-helped rescue started"), AndroidSession)) return false;
	TestEqual(TEXT("Same budget as the real Technician"), AndroidSession->GetMoveBudget(), 40);
	TestEqual(TEXT("Alarm already tripped"), AndroidSession->GetAlarmCount(), 1);
	AndroidSession->Abort();
	AndroidHelper->GetRescue()->Finish(Report);
	TestEqual(TEXT("Report carries the alarm"), Report.AlarmCount, 1);
	TestTrue(TEXT("Helper suspicion rises with alarms"), AndroidHelper->GetAra()->GetSuspicion(Technician) > SuspicionBefore);

	// 혼자 하다 경보: 누군지 몰라 은신처 동료 전원이 조금씩 의심받음 (도운 사람이 있을 때보다 약하게)
	USSAraDirector* Solo = NewObject<USSAraDirector>();
	Solo->ObserveAlarm(TEXT("Helper"), 1);
	Solo->ObserveAlarm(TEXT("Bystander"), 1, false);
	TestTrue(TEXT("Unidentified alarm still raises suspicion"), Solo->GetSuspicion(TEXT("Bystander")) > USSAraDirector::PriorSuspicion);
	TestTrue(TEXT("Identified helper is suspected more"), Solo->GetSuspicion(TEXT("Helper")) > Solo->GetSuspicion(TEXT("Bystander")));

	// 경보 수가 많을수록 더 의심
	USSAraDirector* Ara = NewObject<USSAraDirector>();
	Ara->ObserveAlarm(TEXT("A"), 1);
	Ara->ObserveAlarm(TEXT("B"), 3);
	Ara->ObserveAlarm(TEXT("C"), 0);
	TestTrue(TEXT("More alarms, more suspicion"), Ara->GetSuspicion(TEXT("B")) > Ara->GetSuspicion(TEXT("A")));
	TestEqual(TEXT("Zero alarms change nothing"), Ara->GetSuspicion(TEXT("C")), USSAraDirector::PriorSuspicion);
	return true;
}
#endif
