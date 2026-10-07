#include "Rescue/SSRescueSession.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace SSRescueSessionTest
{
	// 정답 경로 칸만 정답 모양으로 돌림 (세션이 끝나면 멈춤)
	void SolveBySession(USSRescueSession* Session)
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

	// 정답 경로 밖의 칸 하나 (없으면 INDEX_NONE)
	int32 FindOffPathCell(const FSSPowerGrid& Grid)
	{
		const TArray<int32> Path = Grid.FindSolutionPath();
		for (int32 Cell = 0; Cell < Grid.Tiles.Num(); ++Cell)
		{
			if (!Path.Contains(Cell)) return Cell;
		}
		return INDEX_NONE;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSRescueSessionTest, "SS.Rescue.Session",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSRescueSessionTest::RunTest(const FString& Parameters)
{
	using namespace SSRescueSessionTest;

	// 회전 횟수: 태오가 도우면 × 8, 혼자면 × 4, 음수 행동력은 0
	TestEqual(TEXT("Technician helps: AP x 8"), USSRescueSession::CalcMoveBudget(5, true), 40);
	TestEqual(TEXT("Alone: AP x 4"), USSRescueSession::CalcMoveBudget(5, false), 20);
	TestEqual(TEXT("Negative AP gives zero moves"), USSRescueSession::CalcMoveBudget(-2, true), 0);

	// 시작 상태
	USSRescueSession* Session = NewObject<USSRescueSession>();
	int32 Broadcasts = 0;
	Session->OnChanged.AddLambda([&Broadcasts]() { ++Broadcasts; });
	Session->Start(TEXT("TestResearcher"), 40, false, 11);
	TestTrue(TEXT("Target stored"), Session->GetTargetId() == FName(TEXT("TestResearcher")));
	TestEqual(TEXT("Moves start full"), Session->GetRemainingMoves(), 40);
	TestEqual(TEXT("No alarms at start"), Session->GetAlarmCount(), 0);
	TestTrue(TEXT("In progress"), Session->GetOutcome() == ESSRescueOutcome::InProgress);
	TestEqual(TEXT("Grid is 5 x 4"), Session->GetGrid().Tiles.Num(), USSRescueSession::GridWidth * USSRescueSession::GridHeight);
	TestEqual(TEXT("Start notifies the screen"), Broadcasts, 1);

	// 정답 경로를 맞추면 풀림. 섞기는 예산의 60% 안에 풀리게 되어 있음
	const int32 Needed = Session->GetGrid().CountTurnsToSolve();
	TestTrue(TEXT("Solvable within 60% of the budget"), Needed >= 1 && Needed <= FMath::FloorToInt(40 * USSRescueSession::SolvableShare));
	SolveBySession(Session);
	TestTrue(TEXT("Solving unlocks"), Session->GetOutcome() == ESSRescueOutcome::Unlocked);
	TestTrue(TEXT("Used at most the needed turns"), 40 - Session->GetRemainingMoves() <= Needed);

	// 끝난 뒤에는 돌리기·중단 모두 결과를 안 바꿈
	const int32 MovesAfter = Session->GetRemainingMoves();
	TestFalse(TEXT("No rotation after finish"), Session->Rotate(0));
	Session->Abort();
	TestTrue(TEXT("Abort after success keeps success"), Session->GetOutcome() == ESSRescueOutcome::Unlocked);
	TestEqual(TEXT("Moves unchanged after finish"), Session->GetRemainingMoves(), MovesAfter);

	// 판 밖 칸은 회전을 안 씀
	USSRescueSession* OutOfRange = NewObject<USSRescueSession>();
	OutOfRange->Start(NAME_None, 10, false, 11);
	TestFalse(TEXT("Out-of-range cell refused"), OutOfRange->Rotate(-1) || OutOfRange->Rotate(20));
	TestEqual(TEXT("Refused rotation costs nothing"), OutOfRange->GetRemainingMoves(), 10);

	// 회전이 모자라면 실패: 경로 밖 칸만 돌리며 회전을 다 씀
	USSRescueSession* Wasted = NewObject<USSRescueSession>();
	Wasted->Start(NAME_None, 10, false, 21);
	const int32 OffPath = FindOffPathCell(Wasted->GetGrid());
	if (TestTrue(TEXT("Grid has a cell off the solution path"), OffPath != INDEX_NONE))
	{
		while (!Wasted->IsFinished()) Wasted->Rotate(OffPath);
		TestTrue(TEXT("Running out of moves fails"), Wasted->GetOutcome() == ESSRescueOutcome::OutOfMoves);
		TestEqual(TEXT("All moves used"), Wasted->GetRemainingMoves(), 0);
	}

	// 마지막 회전으로 풀면 회전이 0이어도 성공: 경로 밖 칸을 4번씩(제자리) 돌려 회전을 맞춘 뒤 풀기
	bool bLastMoveChecked = false;
	for (int32 Seed = 1; Seed <= 50 && !bLastMoveChecked; ++Seed)
	{
		USSRescueSession* Exact = NewObject<USSRescueSession>();
		Exact->Start(NAME_None, 20, false, Seed);
		const int32 Need = Exact->GetGrid().CountTurnsToSolve();
		const int32 Spare = Exact->GetRemainingMoves() - Need;
		const int32 Idle = FindOffPathCell(Exact->GetGrid());
		if (Spare % 4 != 0 || Idle == INDEX_NONE) continue;

		for (int32 Turn = 0; Turn < Spare; ++Turn) Exact->Rotate(Idle);
		if (Exact->IsFinished()) continue;   // 드물게 경로 밖 칸이 다른 길을 만들면 건너뜀
		SolveBySession(Exact);
		bLastMoveChecked = true;
		TestTrue(TEXT("Unlock on the last move is a success"), Exact->GetOutcome() == ESSRescueOutcome::Unlocked);
		TestEqual(TEXT("No moves left"), Exact->GetRemainingMoves(), 0);
	}
	TestTrue(TEXT("Last-move unlock case found"), bLastMoveChecked);

	// 중단: 진행 중이면 Aborted, 이후엔 못 돌림
	USSRescueSession* Aborted = NewObject<USSRescueSession>();
	Aborted->Start(NAME_None, 10, false, 3);
	Aborted->Abort();
	TestTrue(TEXT("Abort while in progress"), Aborted->GetOutcome() == ESSRescueOutcome::Aborted);
	TestFalse(TEXT("No rotation after abort"), Aborted->Rotate(0));

	// 안드로이드 태오: 시작부터 경보 1회
	USSRescueSession* Android = NewObject<USSRescueSession>();
	Android->Start(NAME_None, 40, true, 11);
	TestEqual(TEXT("Android helper starts with one alarm"), Android->GetAlarmCount(), 1);

	// 경보 세기: 무작위로 돌리면서, "바로 전엔 안 닿았는데 지금 닿은 단자" 수만큼만 늘어나는지
	// (시작할 때 이미 닿아 있던 단자는 안 셈, 계속 닿아 있으면 안 셈, 떨어졌다 다시 닿으면 셈)
	bool bCountMatches = true;
	bool bSawAlarm = false;
	for (int32 Seed = 1; Seed <= 100; ++Seed)
	{
		USSRescueSession* Counter = NewObject<USSRescueSession>();
		Counter->Start(NAME_None, 200, false, Seed);

		// 세션과 똑같이 돌릴 거울 판
		FSSPowerGrid Mirror = Counter->GetGrid();
		TSet<int32> Touching(Mirror.GetHotAlarms());
		int32 Expected = 0;

		FRandomStream Clicks(Seed * 31);
		for (int32 Click = 0; Click < 60 && !Counter->IsFinished(); ++Click)
		{
			const int32 Cell = Clicks.RandRange(0, Mirror.Tiles.Num() - 1);
			Counter->Rotate(Cell);
			Mirror.RotateTile(Cell);

			const TArray<int32> Hot = Mirror.GetHotAlarms();
			for (const int32 Index : Hot)
			{
				if (!Touching.Contains(Index)) ++Expected;
			}
			Touching = TSet<int32>(Hot);
			bCountMatches &= Counter->GetAlarmCount() == Expected;
		}
		bSawAlarm |= Expected > 0;
	}
	TestTrue(TEXT("Alarm count = newly touched terminals only"), bCountMatches);
	TestTrue(TEXT("Random play does trip alarms sometimes"), bSawAlarm);
	return true;
}
#endif
