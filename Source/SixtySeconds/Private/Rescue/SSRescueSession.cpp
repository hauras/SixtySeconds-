#include "Rescue/SSRescueSession.h"

int32 USSRescueSession::CalcMoveBudget(int32 ActionPoints, bool bTechnicianHelps)
{
	// 태오가 도우면 행동력 × 8, 혼자면 서툴러서 × 4
	const int32 MovesPerPoint = bTechnicianHelps ? 8 : 4;
	return FMath::Max(0, ActionPoints) * MovesPerPoint;
}

void USSRescueSession::Start(FName InTargetId, int32 InMoveBudget, bool bAlarmAlreadyTripped, int32 Seed)
{
	// 1) 받은 값 저장
	TargetId = InTargetId;
	MoveBudget = FMath::Max(0, InMoveBudget);
	RemainingMoves = MoveBudget;

	// 2) 상태 초기화
	Outcome = ESSRescueOutcome::InProgress;
	AlarmCount = 0;
	TouchingAlarms.Reset();

	// 3) 퍼즐 생성 (같은 시드 = 같은 퍼즐)
	FRandomStream Random(Seed);
	// 회전 예산 안에 반드시 풀리게: 정답까지 드는 회전을 예산의 60% 이하로
	const int32 MaxSolveTurns = FMath::Max(1, FMath::FloorToInt(MoveBudget * SolvableShare));
	Grid.Generate(GridWidth, GridHeight, AlarmTerminals, Random, MaxSolveTurns);

	// 섞인 상태에서 이미 닿아 있던 단자는 플레이어 탓이 아니라 세지 않음 (떨어졌다 다시 닿으면 셈)
	TouchingAlarms = TSet<int32>(Grid.GetHotAlarms());

	// 4) 안드로이드 태오가 도왔으면 아라가 처음부터 접근을 알고 있음
	if (bAlarmAlreadyTripped)
	{
		AlarmCount = 1;
	}

	// 5) 화면에 알림
	OnChanged.Broadcast();
}

bool USSRescueSession::Rotate(int32 Cell)
{
	if (IsFinished() || RemainingMoves <= 0 || !Grid.Tiles.IsValidIndex(Cell)) return false;

	Grid.RotateTile(Cell);
	--RemainingMoves;
	CountNewAlarms();

	if (Grid.IsUnlocked())
	{
		Outcome = ESSRescueOutcome::Unlocked;
	}
	else if (RemainingMoves == 0)
	{
		Outcome = ESSRescueOutcome::OutOfMoves;
	}

	// 5) 화면 알림
	OnChanged.Broadcast();
	return true;
}

void USSRescueSession::Abort()
{
	if (IsFinished()) return;

	Outcome = ESSRescueOutcome::Aborted;
	OnChanged.Broadcast();
}

void USSRescueSession::CountNewAlarms()
{
	const TArray<int32> Hot = Grid.GetHotAlarms();

	for (const int32 Alarm : Hot)
	{
		if (!TouchingAlarms.Contains(Alarm))
		{
			++AlarmCount;
		}
	}

	TouchingAlarms = TSet<int32>(Hot);
}
