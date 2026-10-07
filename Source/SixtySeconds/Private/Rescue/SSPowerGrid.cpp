

#include "Rescue/SSPowerGrid.h"

namespace
{
	// 이웃한 두 칸 사이 이음선 하나
	struct FSSPowerEdge
	{
		int32 CellA = 0;
		int32 CellB = 0;
		uint8 SideA = 0;   // A 칸에서 B 쪽으로 열 면
		uint8 SideB = 0;   // B 칸에서 A 쪽으로 열 면
	};

	int32 FindRoot(TArray<int32>& Parent, int32 Cell)
	{
		while (Parent[Cell] != Cell)
		{
			Cell = Parent[Cell];
		}
		return Cell;
	}

	// 두 칸을 한 묶음으로 (이미 같은 묶음이면 그대로)
	void UnionCells(TArray<int32>& Parent, int32 A, int32 B)
	{
		const int32 RootA = FindRoot(Parent, A);
		const int32 RootB = FindRoot(Parent, B);
		if (RootA != RootB) Parent[RootA] = RootB;
	}
}

void FSSPowerGrid::Generate(int32 InWidth, int32 InHeight, int32 AlarmCount, FRandomStream& Random, int32 MaxSolveTurns)
{
	Width = InWidth;
	Height = InHeight;

	const int32 CellCount = Width * Height;
	
	Tiles.Init(0, CellCount);
	Solution.Init(0, CellCount);

	TArray<FSSPowerEdge> Edges;
	for (int32 Row = 0; Row < Height; ++Row)
	{
		for (int32 Col = 0; Col < Width; ++Col)
		{
			const int32 Cell = Row * Width + Col;

			if (Col < Width - 1)
			{
				FSSPowerEdge Edge;                    // 1) 이음선 하나 만들기
				Edge.CellA = Cell;                    // 2) 이 칸
				Edge.CellB = Cell + 1;                //    오른쪽 이웃
				Edge.SideA = SSPowerSide::East;       // 3) 이 칸은 동쪽을 열고
				Edge.SideB = SSPowerSide::West;       //    이웃은 서쪽을 염
				Edges.Add(Edge);                      // 4) 목록에 넣기
			}

			// 아래 이웃이 있으면 (맨 아래 행이 아니면)
			if (Row < Height - 1)
			{
				FSSPowerEdge Edge;
				Edge.CellA = Cell;
				Edge.CellB = Cell + Width;            // 한 행 아래 = 한 줄(Width칸)을 건너뜀
				Edge.SideA = SSPowerSide::South;      // 이 칸은 아래(남)를 열고
				Edge.SideB = SSPowerSide::North;      // 아래 칸은 위(북)를 염
				Edges.Add(Edge);
			}
		}
	}
	for (int32 Index = Edges.Num() - 1; Index > 0; --Index)
	{
		Edges.Swap(Index, Random.RandRange(0, Index));
	}

	TArray<int32> Parent;
	for (int32 Index = 0; Index < CellCount; ++Index)
	{
		Parent.Add(Index);   // 0번 칸의 대장은 0, 1번 칸의 대장은 1, ...
	}

	for (const FSSPowerEdge& Edge : Edges)
	{
		const int32 RootA = FindRoot(Parent, Edge.CellA);
		const int32 RootB = FindRoot(Parent, Edge.CellB);

		if (RootA == RootB) continue;

		Parent[RootA] = RootB;

		Solution[Edge.CellA] |= Edge.SideA;
		Solution[Edge.CellB] |= Edge.SideB;
	}

	const int32 SourceRow = Random.RandRange(0, Height - 1);
	SourceCell = SourceRow * Width;
	Solution[SourceCell] |= SSPowerSide::West;

	const int32 LockRow = Random.RandRange(0, Height - 1);
	LockCell = LockRow * Width + (Width - 1);
	Solution[LockCell] |= SSPowerSide::East;

	// 경보 단자: 위·아래 테두리 중 정답 배선이 열지 않은 면 (정답대로 이으면 경보 0회)
	TArray<FSSPowerAlarm> Candidates;
	for (int32 Col = 0; Col < Width; ++Col)
	{
		// 맨 위 행의 칸 (행 0): 북쪽이 닫혀 있으면 후보
		const int32 TopCell = Col;
		if ((Solution[TopCell] & SSPowerSide::North) == 0)
		{
			FSSPowerAlarm Alarm;
			Alarm.Cell = TopCell;
			Alarm.Side = SSPowerSide::North;
			Candidates.Add(Alarm);
		}

		// 맨 아래 행의 칸 (행 Height - 1): 남쪽이 닫혀 있으면 후보
		const int32 BottomCell = (Height - 1) * Width + Col;
		if ((Solution[BottomCell] & SSPowerSide::South) == 0)
		{
			FSSPowerAlarm Alarm;
			Alarm.Cell = BottomCell;
			Alarm.Side = SSPowerSide::South;
			Candidates.Add(Alarm);
		}
	}

	// 후보 순서 섞기 (Fisher-Yates)
	for (int32 Index = Candidates.Num() - 1; Index > 0; --Index)
	{
		Candidates.Swap(Index, Random.RandRange(0, Index));
	}

	// 앞에서 AlarmCount개 (후보가 모자라면 있는 만큼)
	Alarms.Reset();
	const int32 PickCount = FMath::Min(AlarmCount, Candidates.Num());
	for (int32 Index = 0; Index < PickCount; ++Index)
	{
		Alarms.Add(Candidates[Index]);
	}

	// 정답에서 출발해 칸마다 0~3번 돌려 섞기. 섞자마자 풀려 있으면 다시 (혹시 모를 무한 반복은 100번에서 끊음)
	// 예산 상한은 최소 1 (0이면 처음부터 풀린 판이 됨)
	const int32 SolveLimit = FMath::Max(1, MaxSolveTurns);
	const TArray<int32> Path = FindSolutionPath();

	int32 Attempts = 0;
	do
	{
		Tiles = Solution;
		for (int32 Cell = 0; Cell < CellCount; ++Cell)
		{
			const int32 Turns = Random.RandRange(0, 3);
			for (int32 Turn = 0; Turn < Turns; ++Turn)
			{
				RotateTile(Cell);
			}
		}

		// 정답 경로를 맞추는 데 너무 많이 돌려야 하면, 경로 칸 하나씩 정답 쪽으로 한 칸 돌려 놓음
		// (시계 방향으로 한 번 돌리면 남은 횟수가 정확히 1 줄어듦 → 반드시 끝남)
		while (CountTurnsToSolve() > SolveLimit)
		{
			TArray<int32> Wrong;
			for (const int32 Cell : Path)
			{
				if (TurnsToSolve(Cell) > 0) Wrong.Add(Cell);
			}
			RotateTile(Wrong[Random.RandRange(0, Wrong.Num() - 1)]);
		}
		++Attempts;
	}
	while (IsUnlocked() && Attempts < 100);
}

TArray<int32> FSSPowerGrid::FindSolutionPath() const
{
	TArray<int32> Path;
	const int32 CellCount = Width * Height;
	if (!Solution.IsValidIndex(SourceCell) || !Solution.IsValidIndex(LockCell)) return Path;

	// 전원 칸에서 정답 배선을 따라 퍼져 나가며 "어디서 왔나"를 적고, 잠금 칸에서 거꾸로 따라감
	TArray<int32> CameFrom;
	CameFrom.Init(INDEX_NONE, CellCount);
	CameFrom[SourceCell] = SourceCell;

	const int32 Steps[4] = { -Width, 1, Width, -1 };   // 북·동·남·서 이웃의 칸 번호 차이
	const uint8 Sides[4] = { SSPowerSide::North, SSPowerSide::East, SSPowerSide::South, SSPowerSide::West };

	TArray<int32> Stack = { SourceCell };
	while (!Stack.IsEmpty())
	{
		const int32 Cell = Stack.Pop();
		const int32 Col = Cell % Width;
		for (int32 Dir = 0; Dir < 4; ++Dir)
		{
			if ((Solution[Cell] & Sides[Dir]) == 0) continue;

			// 판 밖으로 나가는 면(전원 서쪽·잠금 동쪽)은 건너뜀
			if ((Dir == 1 && Col == Width - 1) || (Dir == 3 && Col == 0)) continue;
			const int32 Next = Cell + Steps[Dir];
			if (!Solution.IsValidIndex(Next) || CameFrom[Next] != INDEX_NONE) continue;

			CameFrom[Next] = Cell;
			Stack.Push(Next);
		}
	}
	if (CameFrom[LockCell] == INDEX_NONE) return Path;

	for (int32 Cell = LockCell; Cell != SourceCell; Cell = CameFrom[Cell])
	{
		Path.Add(Cell);
	}
	Path.Add(SourceCell);
	return Path;
}

int32 FSSPowerGrid::TurnsToSolve(int32 Cell) const
{
	if (!Tiles.IsValidIndex(Cell) || !Solution.IsValidIndex(Cell)) return 0;

	uint8 Turned = Tiles[Cell];
	for (int32 Turns = 0; Turns < 4; ++Turns)
	{
		if (Turned == Solution[Cell]) return Turns;
		Turned = RotateClockwise(Turned);
	}
	return 0;   // 같은 타일을 돌린 것이라 여기 올 일은 없음
}

int32 FSSPowerGrid::CountTurnsToSolve() const
{
	int32 Total = 0;
	for (const int32 Cell : FindSolutionPath())
	{
		Total += TurnsToSolve(Cell);
	}
	return Total;
}

uint8 FSSPowerGrid::RotateClockwise(uint8 Mask)
{
	return ((Mask << 1) | (Mask >> 3)) & 15;
}

void FSSPowerGrid::RotateTile(int32 Cell)
{
	if (!Tiles.IsValidIndex(Cell)) return;

	Tiles[Cell] = RotateClockwise(Tiles[Cell]);
}

TSet<int32> FSSPowerGrid::ComputePowered() const
{
	// 아직 Generate 전이면 전력 없음
	if (!Tiles.IsValidIndex(SourceCell)) return {};

	const int32 CellCount = Width * Height;

	// 칸 번호 다음 자리를 전원 노드로
	const int32 PowerNode = CellCount;

	// 1) 칸 수 + 전원 1개, 처음엔 모두 혼자
	TArray<int32> Parent;
	for (int32 Index = 0; Index < CellCount + 1; ++Index)
	{
		Parent.Add(Index);
	}

	// 2) 전원 칸이 서쪽(바깥 전원 쪽)을 열고 있으면 전원과 이어짐
	if ((Tiles[SourceCell] & SSPowerSide::West) != 0)
	{
		UnionCells(Parent, SourceCell, PowerNode);
	}

	// 3) 이웃끼리 마주 보는 면이 둘 다 열렸으면 한 묶음 (한쪽만 열리면 벽에 막힌 관)
	for (int32 Row = 0; Row < Height; ++Row)
	{
		for (int32 Col = 0; Col < Width; ++Col)
		{
			const int32 Cell = Row * Width + Col;

			// 오른쪽 이웃: 내 동쪽 + 이웃 서쪽
			if (Col < Width - 1
				&& (Tiles[Cell] & SSPowerSide::East) != 0
				&& (Tiles[Cell + 1] & SSPowerSide::West) != 0)
			{
				UnionCells(Parent, Cell, Cell + 1);
			}

			// 아래 이웃: 내 남쪽 + 이웃 북쪽
			if (Row < Height - 1
				&& (Tiles[Cell] & SSPowerSide::South) != 0
				&& (Tiles[Cell + Width] & SSPowerSide::North) != 0)
			{
				UnionCells(Parent, Cell, Cell + Width);
			}
		}
	}

	// 4) 전원과 대장이 같은 칸 = 전력이 흐르는 칸
	TSet<int32> Powered;
	const int32 PowerRoot = FindRoot(Parent, PowerNode);
	for (int32 Cell = 0; Cell < CellCount; ++Cell)
	{
		if (FindRoot(Parent, Cell) == PowerRoot)
		{
			Powered.Add(Cell);
		}
	}
	return Powered;
}

bool FSSPowerGrid::IsUnlocked() const
{
	// 아직 Generate 전이면 잠김
	if (!Tiles.IsValidIndex(LockCell)) return false;

	// 잠금 칸에 전력이 오고, 그 칸 관이 오른쪽(잠금 장치)을 향하면 풀림
	return ComputePowered().Contains(LockCell) && (Tiles[LockCell] & SSPowerSide::East) != 0;
}

TArray<int32> FSSPowerGrid::GetHotAlarms() const
{
	TArray<int32> Hot;

	// 전력 칸은 한 번만 계산 (단자마다 다시 계산하면 낭비)
	const TSet<int32> Powered = ComputePowered();

	for (int32 Index = 0; Index < Alarms.Num(); ++Index)
	{
		const FSSPowerAlarm& Alarm = Alarms[Index];

		// 단자 칸에 전력이 있고, 그 칸이 단자 쪽 면을 열고 있으면 켜짐
		if (Powered.Contains(Alarm.Cell) && (Tiles[Alarm.Cell] & Alarm.Side) != 0)
		{
			Hot.Add(Index);
		}
	}
	return Hot;
}
