#include "Item/SSItemSpawnTypes.h"
#include "Item/SSItemDefinition.h"

TArray<int32> FSSItemSpawnPlanner::PlanSurvivors(const TArray<FName>& PointRooms, int32 Count, FRandomStream& Random)
{
	TArray<int32> Chosen;
	const int32 Wanted = FMath::Min(Count, PointRooms.Num());
	if (Wanted <= 0) return Chosen;

	// 자리 순서를 섞고 (피셔-예이츠)
	TArray<int32> Order;
	for (int32 Index = 0; Index < PointRooms.Num(); ++Index) Order.Add(Index);
	for (int32 Index = Order.Num() - 1; Index > 0; --Index)
	{
		Order.Swap(Index, Random.RandRange(0, Index));
	}

	// 1) 아직 아무도 없는 방의 자리부터
	TSet<FName> UsedRooms;
	for (const int32 Point : Order)
	{
		if (Chosen.Num() >= Wanted) break;
		if (UsedRooms.Contains(PointRooms[Point])) continue;
		UsedRooms.Add(PointRooms[Point]);
		Chosen.Add(Point);
	}

	// 2) 방이 모자라면 남은 자리로
	for (const int32 Point : Order)
	{
		if (Chosen.Num() >= Wanted) break;
		if (!Chosen.Contains(Point)) Chosen.Add(Point);
	}
	return Chosen;
}

int32 FSSItemSpawnPlanner::PickEntry(const USSItemSpawnTable& Table, FRandomStream& Random)
{
	float TotalWeight = 0.f;
	for (const FSSItemSpawnEntry& Entry : Table.Entries)
	{
		if (IsValid(Entry.Item)) TotalWeight += FMath::Max(0.f, Entry.Weight);
	}
	if (TotalWeight <= 0.f) return INDEX_NONE;

	// 0 ~ 전체 비율 사이 한 점이 들어간 칸
	float Roll = Random.FRandRange(0.f, TotalWeight);
	int32 Last = INDEX_NONE;
	for (int32 Index = 0; Index < Table.Entries.Num(); ++Index)
	{
		const FSSItemSpawnEntry& Entry = Table.Entries[Index];
		if (!IsValid(Entry.Item) || Entry.Weight <= 0.f) continue;
		Last = Index;
		if (Roll < Entry.Weight) return Index;
		Roll -= Entry.Weight;
	}
	return Last; // 소수점 오차로 끝까지 왔으면 마지막 칸
}

TArray<FSSItemPlacement> FSSItemSpawnPlanner::Plan(const TArray<FName>& PointRooms, const USSItemSpawnTable& Table, FRandomStream& Random)
{
	TArray<FSSItemPlacement> Placements;
	const int32 Wanted = FMath::Min(Table.SpawnCount, PointRooms.Num());
	if (Wanted <= 0) return Placements;

	// 자리 순서를 섞음 (피셔-예이츠)
	TArray<int32> Order;
	for (int32 Index = 0; Index < PointRooms.Num(); ++Index) Order.Add(Index);
	for (int32 Index = Order.Num() - 1; Index > 0; --Index)
	{
		Order.Swap(Index, Random.RandRange(0, Index));
	}

	// 1) 방마다 섞인 순서에서 처음 만나는 자리 하나 (방 이름이 없는 자리는 방 보장에서 뺌)
	TArray<int32> Chosen;
	TSet<FName> CoveredRooms;
	for (const int32 Point : Order)
	{
		const FName Room = PointRooms[Point];
		if (Room.IsNone() || CoveredRooms.Contains(Room) || Chosen.Num() >= Wanted) continue;
		CoveredRooms.Add(Room);
		Chosen.Add(Point);
	}

	// 2) 남은 자리를 섞인 순서대로 채움
	for (const int32 Point : Order)
	{
		if (Chosen.Num() >= Wanted) break;
		if (!Chosen.Contains(Point)) Chosen.Add(Point);
	}

	// 3) 자리마다 아이템과 개수
	for (const int32 Point : Chosen)
	{
		const int32 Entry = PickEntry(Table, Random);
		if (Entry == INDEX_NONE) break;
		const FSSItemSpawnEntry& Row = Table.Entries[Entry];

		FSSItemPlacement& Placement = Placements.AddDefaulted_GetRef();
		Placement.PointIndex = Point;
		Placement.EntryIndex = Entry;
		Placement.Quantity = Random.RandRange(Row.MinQuantity, FMath::Max(Row.MinQuantity, Row.MaxQuantity));
	}
	return Placements;
}
