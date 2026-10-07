#include "Rescue/SSPowerGrid.h"
#include "Rescue/SSRescueSession.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace SSPowerGridTest
{
	// 열린 면 개수
	int32 CountSides(uint8 Mask)
	{
		return FMath::CountBits(Mask);
	}

	// 가로 3칸 한 줄 판: 전원 → 칸0 → 칸1 → 칸2 → 잠금
	FSSPowerGrid MakeRow(uint8 Cell0, uint8 Cell1, uint8 Cell2)
	{
		FSSPowerGrid Grid;
		Grid.Width = 3;
		Grid.Height = 1;
		Grid.Tiles = { Cell0, Cell1, Cell2 };
		Grid.SourceCell = 0;
		Grid.LockCell = 2;
		return Grid;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSPowerGridRotateTest, "SS.Rescue.Rotate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSPowerGridRotateTest::RunTest(const FString& Parameters)
{
	using namespace SSPowerSide;

	// 한 방향씩: 북 → 동 → 남 → 서 → 북
	TestEqual(TEXT("North -> East"), FSSPowerGrid::RotateClockwise(North), East);
	TestEqual(TEXT("East -> South"), FSSPowerGrid::RotateClockwise(East), South);
	TestEqual(TEXT("South -> West"), FSSPowerGrid::RotateClockwise(South), West);
	TestEqual(TEXT("West -> North (wraps)"), FSSPowerGrid::RotateClockwise(West), North);

	// 꺾임 타일: 북+동 → 동+남, 남+서 → 서+북
	TestEqual(TEXT("Corner N+E -> E+S"), FSSPowerGrid::RotateClockwise(North | East), static_cast<uint8>(East | South));
	TestEqual(TEXT("Corner S+W -> W+N"), FSSPowerGrid::RotateClockwise(South | West), static_cast<uint8>(West | North));

	// 모든 모양: 4번 돌리면 원래대로, 열린 면 수는 그대로
	bool bFourTurnsBack = true;
	bool bSideCountKept = true;
	for (uint8 Mask = 0; Mask < 16; ++Mask)
	{
		uint8 Turned = Mask;
		for (int32 Turn = 0; Turn < 4; ++Turn)
		{
			Turned = FSSPowerGrid::RotateClockwise(Turned);
			bSideCountKept &= SSPowerGridTest::CountSides(Turned) == SSPowerGridTest::CountSides(Mask);
		}
		bFourTurnsBack &= Turned == Mask;
	}
	TestTrue(TEXT("Four turns return to the start"), bFourTurnsBack);
	TestTrue(TEXT("Rotation keeps the number of open sides"), bSideCountKept);

	// 범위 밖 칸을 돌려도 아무 일 없음
	FSSPowerGrid Grid = SSPowerGridTest::MakeRow(North, North, North);
	Grid.RotateTile(-1);
	Grid.RotateTile(3);
	TestTrue(TEXT("Out-of-range rotate does nothing"), Grid.Tiles[0] == North && Grid.Tiles[1] == North && Grid.Tiles[2] == North);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSPowerGridPoweredTest, "SS.Rescue.Powered",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSPowerGridPoweredTest::RunTest(const FString& Parameters)
{
	using namespace SSPowerSide;

	// 직선 - 직선 - 세로 직선: 칸2가 옆으로 막혀 있음
	FSSPowerGrid Grid = SSPowerGridTest::MakeRow(West | East, West | East, North | South);
	TSet<int32> Powered = Grid.ComputePowered();
	TestTrue(TEXT("Power reaches cells 0 and 1"), Powered.Contains(0) && Powered.Contains(1));
	TestFalse(TEXT("Cell 2 blocked"), Powered.Contains(2));
	TestFalse(TEXT("Not unlocked yet"), Grid.IsUnlocked());

	// 칸2를 한 번 돌리면 가로 직선 → 잠금까지 이어짐
	Grid.RotateTile(2);
	TestEqual(TEXT("Cell 2 now W+E"), Grid.Tiles[2], static_cast<uint8>(West | East));
	TestEqual(TEXT("All three powered"), Grid.ComputePowered().Num(), 3);
	TestTrue(TEXT("Unlocked"), Grid.IsUnlocked());

	// 한쪽만 열린 면은 안 이어짐: 칸0 동쪽은 열렸지만 칸1 서쪽이 닫힘
	FSSPowerGrid HalfOpen = SSPowerGridTest::MakeRow(West | East, North | East, West | East);
	TestEqual(TEXT("One-sided opening carries no power"), HalfOpen.ComputePowered().Num(), 1);

	// 전원 칸 서쪽이 닫혀 있으면 아무 데도 전력 없음
	FSSPowerGrid NoSource = SSPowerGridTest::MakeRow(North | East, West | East, West | East);
	TestEqual(TEXT("Source side closed, nothing powered"), NoSource.ComputePowered().Num(), 0);

	// 잠금 칸까지 전력이 와도 동쪽(잠금 장치 쪽)이 닫혀 있으면 안 풀림
	FSSPowerGrid LockFacingAway = SSPowerGridTest::MakeRow(West | East, West | East, West | North);
	TestTrue(TEXT("Lock cell powered"), LockFacingAway.ComputePowered().Contains(2));
	TestFalse(TEXT("Lock side closed, still locked"), LockFacingAway.IsUnlocked());

	// 경보: 칸1 북쪽 단자. 칸1이 T자(서+동+북)면 켜지고, 직선이면 꺼짐
	FSSPowerGrid Alarmed = SSPowerGridTest::MakeRow(West | East, West | East | North, West | East);
	FSSPowerAlarm Alarm;
	Alarm.Cell = 1;
	Alarm.Side = North;
	Alarmed.Alarms.Add(Alarm);
	TestTrue(TEXT("Alarm hot when a powered tile faces it"), Alarmed.GetHotAlarms() == TArray<int32>{ 0 });
	Alarmed.Tiles[1] = West | East;
	TestEqual(TEXT("Alarm quiet when the tile faces away"), Alarmed.GetHotAlarms().Num(), 0);

	// 전력 없는 칸이 단자를 향해도 안 켜짐
	Alarmed.Tiles[0] = North | South;
	Alarmed.Tiles[1] = West | East | North;
	TestEqual(TEXT("Unpowered tile does not trip the alarm"), Alarmed.GetHotAlarms().Num(), 0);

	// Generate 전 빈 판: 크래시 없이 전력 없음·잠김
	FSSPowerGrid Empty;
	TestEqual(TEXT("Empty grid has no power"), Empty.ComputePowered().Num(), 0);
	TestFalse(TEXT("Empty grid is locked"), Empty.IsUnlocked());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSPowerGridGenerateTest, "SS.Rescue.GridGenerate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSPowerGridGenerateTest::RunTest(const FString& Parameters)
{
	using namespace SSPowerSide;
	constexpr int32 Width = 5;
	constexpr int32 Height = 4;
	constexpr int32 CellCount = Width * Height;

	bool bSizeOk = true;
	bool bTreeSideCount = true;
	bool bMatchedSides = true;
	bool bNoStrayBorder = true;
	bool bSolutionSolves = true;
	bool bSolutionAllPowered = true;
	bool bSolutionNoAlarm = true;
	bool bScrambledLocked = true;
	bool bScrambleIsRotation = true;
	bool bAlarmsOk = true;
	bool bEndsOnSides = true;

	for (int32 Seed = 1; Seed <= 200; ++Seed)
	{
		FRandomStream Random(Seed);
		FSSPowerGrid Grid;
		Grid.Generate(Width, Height, 3, Random);

		bSizeOk &= Grid.Tiles.Num() == CellCount && Grid.Solution.Num() == CellCount;
		bEndsOnSides &= Grid.SourceCell % Width == 0 && Grid.LockCell % Width == Width - 1;

		// 신장 트리: 선 19개 = 열린 면 38개 + 전원 서쪽 1 + 잠금 동쪽 1
		int32 TotalSides = 0;
		for (const uint8 Mask : Grid.Solution) TotalSides += SSPowerGridTest::CountSides(Mask);
		bTreeSideCount &= TotalSides == 2 * (CellCount - 1) + 2;

		// 정답 배선: 열린 면은 이웃도 마주 보고 열려 있음, 바깥은 전원 서쪽·잠금 동쪽만
		for (int32 Cell = 0; Cell < CellCount; ++Cell)
		{
			const int32 Row = Cell / Width;
			const int32 Col = Cell % Width;
			const uint8 Mask = Grid.Solution[Cell];

			if (Mask & East)
			{
				if (Col == Width - 1) bNoStrayBorder &= Cell == Grid.LockCell;
				else bMatchedSides &= (Grid.Solution[Cell + 1] & West) != 0;
			}
			if (Mask & West)
			{
				if (Col == 0) bNoStrayBorder &= Cell == Grid.SourceCell;
				else bMatchedSides &= (Grid.Solution[Cell - 1] & East) != 0;
			}
			if (Mask & South)
			{
				if (Row == Height - 1) bNoStrayBorder = false;
				else bMatchedSides &= (Grid.Solution[Cell + Width] & North) != 0;
			}
			if (Mask & North)
			{
				if (Row == 0) bNoStrayBorder = false;
				else bMatchedSides &= (Grid.Solution[Cell - Width] & South) != 0;
			}

			// 섞은 타일은 정답 타일을 돌린 것 중 하나
			bool bIsRotation = false;
			uint8 Turned = Mask;
			for (int32 Turn = 0; Turn < 4; ++Turn)
			{
				bIsRotation |= Turned == Grid.Tiles[Cell];
				Turned = FSSPowerGrid::RotateClockwise(Turned);
			}
			bScrambleIsRotation &= bIsRotation;
		}

		// 섞은 상태는 잠겨 있음
		bScrambledLocked &= !Grid.IsUnlocked();

		// 경보 3개, 위·아래 테두리, 중복 없음
		TSet<int32> AlarmKeys;
		for (const FSSPowerAlarm& Alarm : Grid.Alarms)
		{
			const bool bTop = Alarm.Side == North && Alarm.Cell / Width == 0;
			const bool bBottom = Alarm.Side == South && Alarm.Cell / Width == Height - 1;
			bAlarmsOk &= bTop || bBottom;
			AlarmKeys.Add(Alarm.Cell * 16 + Alarm.Side);
		}
		bAlarmsOk &= Grid.Alarms.Num() == 3 && AlarmKeys.Num() == 3;

		// 정답대로 이으면: 풀림, 모든 칸에 전력, 경보 0
		Grid.Tiles = Grid.Solution;
		bSolutionSolves &= Grid.IsUnlocked();
		bSolutionAllPowered &= Grid.ComputePowered().Num() == CellCount;
		bSolutionNoAlarm &= Grid.GetHotAlarms().Num() == 0;
	}

	TestTrue(TEXT("Tile arrays sized to the grid"), bSizeOk);
	TestTrue(TEXT("Source on the left column, lock on the right"), bEndsOnSides);
	TestTrue(TEXT("Solution is a spanning tree (19 links)"), bTreeSideCount);
	TestTrue(TEXT("Every open side meets an open neighbour"), bMatchedSides);
	TestTrue(TEXT("No openings to the outside except source and lock"), bNoStrayBorder);
	TestTrue(TEXT("Scrambled tiles are rotations of the solution"), bScrambleIsRotation);
	TestTrue(TEXT("Scrambled grid starts locked"), bScrambledLocked);
	TestTrue(TEXT("Three distinct alarms on top/bottom edges"), bAlarmsOk);
	TestTrue(TEXT("Solution unlocks"), bSolutionSolves);
	TestTrue(TEXT("Solution powers every cell"), bSolutionAllPowered);
	TestTrue(TEXT("Solution trips no alarm"), bSolutionNoAlarm);

	// 같은 시드 = 같은 퍼즐
	FRandomStream RandomA(42);
	FRandomStream RandomB(42);
	FSSPowerGrid GridA;
	FSSPowerGrid GridB;
	GridA.Generate(Width, Height, 3, RandomA);
	GridB.Generate(Width, Height, 3, RandomB);
	TestTrue(TEXT("Same seed, same puzzle"), GridA.Tiles == GridB.Tiles && GridA.Solution == GridB.Solution
		&& GridA.SourceCell == GridB.SourceCell && GridA.LockCell == GridB.LockCell);

	// 경보를 후보보다 많이 달라고 해도 크래시 없이 있는 만큼 (5×4 판 위·아래 테두리 = 10곳)
	FRandomStream RandomMany(7);
	FSSPowerGrid Many;
	Many.Generate(Width, Height, 50, RandomMany);
	TestEqual(TEXT("Alarm count capped by candidates"), Many.Alarms.Num(), 2 * Width);
	return true;
}
#endif

#if WITH_DEV_AUTOMATION_TESTS
#include "UI/Rescue/SSPowerPanelWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSPowerTileShapeTest, "SS.Rescue.TileShape",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSPowerTileShapeTest::RunTest(const FString& Parameters)
{
	using namespace SSPowerSide;

	// 모양이 있는 15가지 전부: 그림을 고르고, 기본 방향을 그 횟수만큼 돌리면 원래 모양
	const uint8 BaseMasks[] = { 0, North, North | South, North | East, North | East | South, North | East | South | West };
	bool bAllResolved = true;
	bool bRotationMatches = true;
	for (uint8 Mask = 1; Mask < 16; ++Mask)
	{
		ESSPowerTileShape Shape;
		int32 Turns;
		bAllResolved &= USSPowerPanelWidget::ResolveTileShape(Mask, Shape, Turns);

		uint8 Turned = BaseMasks[static_cast<int32>(Shape)];
		for (int32 Turn = 0; Turn < Turns; ++Turn) Turned = FSSPowerGrid::RotateClockwise(Turned);
		bRotationMatches &= Turned == Mask && Turns >= 0 && Turns < 4;
	}
	TestTrue(TEXT("Every non-empty tile has a picture"), bAllResolved);
	TestTrue(TEXT("Base picture rotated by the turns gives the tile"), bRotationMatches);

	// 대표 예: 동+서 직선 = 직선 1회, 서+북 꺾임 = 꺾임 3회, 빈 칸은 그림 없음
	ESSPowerTileShape Shape;
	int32 Turns;
	USSPowerPanelWidget::ResolveTileShape(East | West, Shape, Turns);
	TestTrue(TEXT("E+W is a straight turned once"), Shape == ESSPowerTileShape::Straight && Turns == 1);
	USSPowerPanelWidget::ResolveTileShape(West | North, Shape, Turns);
	TestTrue(TEXT("W+N is a corner turned three times"), Shape == ESSPowerTileShape::Corner && Turns == 3);
	TestFalse(TEXT("Empty tile has no picture"), USSPowerPanelWidget::ResolveTileShape(0, Shape, Turns));
	return true;
}
#endif

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSPowerGridBudgetTest, "SS.Rescue.GridBudget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSPowerGridBudgetTest::RunTest(const FString& Parameters)
{
	// 예산 상한 1~24, 시드 100개: 정답 경로 회전 수가 상한 이하이고, 섞은 판은 잠겨 있고, 경로는 전원→잠금으로 이어짐
	bool bWithinLimit = true;
	bool bStillLocked = true;
	bool bPathEnds = true;
	bool bPathFixesIt = true;
	for (int32 Limit = 1; Limit <= 24; ++Limit)
	{
		for (int32 Seed = 1; Seed <= 100; ++Seed)
		{
			FRandomStream Random(Seed * 37 + Limit);
			FSSPowerGrid Grid;
			Grid.Generate(5, 4, 3, Random, Limit);

			bWithinLimit &= Grid.CountTurnsToSolve() <= Limit;
			bStillLocked &= !Grid.IsUnlocked();

			const TArray<int32> Path = Grid.FindSolutionPath();
			bPathEnds &= Path.Num() >= 1 && Path[0] == Grid.LockCell && Path.Last() == Grid.SourceCell;

			// 경로 칸만 정답 모양으로 맞추면 풀림
			for (const int32 Cell : Path) Grid.Tiles[Cell] = Grid.Solution[Cell];
			bPathFixesIt &= Grid.IsUnlocked();
		}
	}
	TestTrue(TEXT("Solve turns within the limit"), bWithinLimit);
	TestTrue(TEXT("Still starts locked"), bStillLocked);
	TestTrue(TEXT("Path runs from lock back to source"), bPathEnds);
	TestTrue(TEXT("Fixing only the path unlocks"), bPathFixesIt);

	// 혼자·행동력 2 (8회) → 상한 4: 실제 세션도 그 안에 풀림
	USSRescueSession* Small = NewObject<USSRescueSession>();
	Small->Start(NAME_None, USSRescueSession::CalcMoveBudget(2, false), false, 5);
	TestTrue(TEXT("Smallest budget is solvable"), Small->GetGrid().CountTurnsToSolve() <= Small->GetMoveBudget());
	return true;
}
#endif
