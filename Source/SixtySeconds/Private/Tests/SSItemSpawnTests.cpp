#include "Item/SSItemSpawnTypes.h"
#include "Item/SSItemDefinition.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace SSItemSpawnTest
{
	// 비율 4 : 4 : 1 (식량·물·배터리), 한 판에 6자리
	USSItemSpawnTable* MakeTable()
	{
		USSItemSpawnTable* Table = NewObject<USSItemSpawnTable>();
		Table->SpawnCount = 6;
		const auto Add = [Table](const TCHAR* Id, float Weight, int32 MinQ, int32 MaxQ)
		{
			USSItemDefinition* Item = NewObject<USSItemDefinition>();
			Item->ItemId = Id;
			FSSItemSpawnEntry& Entry = Table->Entries.AddDefaulted_GetRef();
			Entry.Item = Item;
			Entry.Weight = Weight;
			Entry.MinQuantity = MinQ;
			Entry.MaxQuantity = MaxQ;
		};
		Add(TEXT("Food"), 4.f, 1, 2);
		Add(TEXT("Water"), 4.f, 1, 2);
		Add(TEXT("Battery"), 1.f, 1, 1);
		return Table;
	}

	// 방 A 5자리, B 3자리, C 2자리, 방 없는 자리 2개 = 12자리
	TArray<FName> MakeRooms()
	{
		TArray<FName> Rooms;
		for (int32 Each = 0; Each < 5; ++Each) Rooms.Add(TEXT("A"));
		for (int32 Each = 0; Each < 3; ++Each) Rooms.Add(TEXT("B"));
		for (int32 Each = 0; Each < 2; ++Each) Rooms.Add(TEXT("C"));
		Rooms.Add(NAME_None);
		Rooms.Add(NAME_None);
		return Rooms;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSItemSpawnPlanTest, "SS.Scramble.ItemSpawnPlan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSItemSpawnPlanTest::RunTest(const FString& Parameters)
{
	using namespace SSItemSpawnTest;
	USSItemSpawnTable* Table = MakeTable();
	const TArray<FName> Rooms = MakeRooms();

	// 같은 시드면 같은 배치
	FRandomStream RandomA(77);
	FRandomStream RandomB(77);
	const TArray<FSSItemPlacement> PlanA = FSSItemSpawnPlanner::Plan(Rooms, *Table, RandomA);
	const TArray<FSSItemPlacement> PlanB = FSSItemSpawnPlanner::Plan(Rooms, *Table, RandomB);
	bool bSame = PlanA.Num() == PlanB.Num();
	for (int32 Index = 0; bSame && Index < PlanA.Num(); ++Index)
	{
		bSame = PlanA[Index].PointIndex == PlanB[Index].PointIndex && PlanA[Index].EntryIndex == PlanB[Index].EntryIndex
			&& PlanA[Index].Quantity == PlanB[Index].Quantity;
	}
	TestTrue(TEXT("Same seed, same placement"), bSame);

	// 여러 시드에서: 정확히 6자리, 자리 중복 없음, 방 A·B·C 모두 하나 이상, 개수는 범위 안
	bool bCountOk = true;
	bool bUnique = true;
	bool bRoomsCovered = true;
	bool bQuantityOk = true;
	for (int32 Seed = 1; Seed <= 200; ++Seed)
	{
		FRandomStream Random(Seed);
		const TArray<FSSItemPlacement> Plan = FSSItemSpawnPlanner::Plan(Rooms, *Table, Random);
		bCountOk &= Plan.Num() == Table->SpawnCount;

		TSet<int32> UsedPoints;
		TSet<FName> UsedRooms;
		for (const FSSItemPlacement& Placement : Plan)
		{
			bUnique &= !UsedPoints.Contains(Placement.PointIndex);
			UsedPoints.Add(Placement.PointIndex);
			UsedRooms.Add(Rooms[Placement.PointIndex]);

			const FSSItemSpawnEntry& Entry = Table->Entries[Placement.EntryIndex];
			bQuantityOk &= Placement.Quantity >= Entry.MinQuantity && Placement.Quantity <= Entry.MaxQuantity;
		}
		bRoomsCovered &= UsedRooms.Contains(TEXT("A")) && UsedRooms.Contains(TEXT("B")) && UsedRooms.Contains(TEXT("C"));
	}
	TestTrue(TEXT("Fills exactly SpawnCount points"), bCountOk);
	TestTrue(TEXT("Never two items on one point"), bUnique);
	TestTrue(TEXT("Every room gets at least one item"), bRoomsCovered);
	TestTrue(TEXT("Quantity within range"), bQuantityOk);

	// 자리가 모자라면 있는 자리만큼만
	Table->SpawnCount = 50;
	FRandomStream RandomAll(5);
	TestEqual(TEXT("Never more than the points"), FSSItemSpawnPlanner::Plan(Rooms, *Table, RandomAll).Num(), Rooms.Num());
	Table->SpawnCount = 6;

	// 비율대로 뽑힘: 배터리는 1/9 ≈ 11% (2000번이면 7~15% 사이)
	FRandomStream RandomPick(9);
	int32 Batteries = 0;
	constexpr int32 Picks = 2000;
	for (int32 Each = 0; Each < Picks; ++Each)
	{
		if (FSSItemSpawnPlanner::PickEntry(*Table, RandomPick) == 2) ++Batteries;
	}
	TestTrue(TEXT("Weighted pick ratio"), Batteries > Picks * 7 / 100 && Batteries < Picks * 15 / 100);

	// 동료 3명: 방 4곳(각 2자리)이면 늘 서로 다른 방, 같은 시드면 같은 자리
	TArray<FName> SurvivorRooms;
	for (const TCHAR* Room : { TEXT("Lab"), TEXT("Lab"), TEXT("Med"), TEXT("Med"), TEXT("Store"), TEXT("Store"), TEXT("Break"), TEXT("Break") })
	{
		SurvivorRooms.Add(Room);
	}
	bool bDifferentRooms = true;
	for (int32 Seed = 1; Seed <= 200; ++Seed)
	{
		FRandomStream Random(Seed);
		const TArray<int32> Chosen = FSSItemSpawnPlanner::PlanSurvivors(SurvivorRooms, 3, Random);
		TSet<FName> Used;
		for (const int32 Point : Chosen) Used.Add(SurvivorRooms[Point]);
		bDifferentRooms &= Chosen.Num() == 3 && Used.Num() == 3;
	}
	TestTrue(TEXT("Survivors land in different rooms"), bDifferentRooms);
	FRandomStream SurvivorA(4);
	FRandomStream SurvivorB(4);
	TestTrue(TEXT("Same seed, same survivor spots"),
		FSSItemSpawnPlanner::PlanSurvivors(SurvivorRooms, 3, SurvivorA) == FSSItemSpawnPlanner::PlanSurvivors(SurvivorRooms, 3, SurvivorB));

	// 방이 2곳뿐이면 같은 방을 쓰더라도 자리는 겹치지 않음
	const TArray<FName> TwoRooms = { TEXT("Lab"), TEXT("Lab"), TEXT("Med"), TEXT("Med") };
	FRandomStream RandomTwo(3);
	const TArray<int32> Crowded = FSSItemSpawnPlanner::PlanSurvivors(TwoRooms, 3, RandomTwo);
	TestEqual(TEXT("Three spots even with two rooms"), Crowded.Num(), 3);
	TestEqual(TEXT("No shared spot"), TSet<int32>(Crowded).Num(), 3);

	// 비율이 전부 0이면 아무것도 안 놓음
	for (FSSItemSpawnEntry& Entry : Table->Entries) Entry.Weight = 0.f;
	FRandomStream RandomZero(1);
	TestEqual(TEXT("No weights, no items"), FSSItemSpawnPlanner::Plan(Rooms, *Table, RandomZero).Num(), 0);
	return true;
}
#endif
