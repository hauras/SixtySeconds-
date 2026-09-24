#include "Exploration/SSExplorationMapDefinition.h"
#include "Misc/DataValidation.h"

#define LOCTEXT_NAMESPACE "SSExplorationMap"

int32 USSExplorationMapDefinition::FindRoomIndex(FName RoomId) const
{
	if (RoomId.IsNone()) return INDEX_NONE;
	return Rooms.IndexOfByPredicate([RoomId](const FSSExplorationRoom& Room) { return Room.RoomId == RoomId; });
}

TArray<TArray<int32>> USSExplorationMapDefinition::BuildAdjacency() const
{
	TArray<TArray<int32>> Adjacency;
	Adjacency.SetNum(Rooms.Num());
	for (const FSSExplorationPassage& Passage : Passages)
	{
		const int32 A = FindRoomIndex(Passage.RoomA);
		const int32 B = FindRoomIndex(Passage.RoomB);
		if (A == INDEX_NONE || B == INDEX_NONE || A == B) continue;
		Adjacency[A].AddUnique(B);
		Adjacency[B].AddUnique(A);
	}
	return Adjacency;
}

TArray<int32> USSExplorationMapDefinition::FindShortestPath(const TArray<TArray<int32>>& Adjacency, int32 From, int32 To)
{
	TArray<int32> Path;
	if (!Adjacency.IsValidIndex(From) || !Adjacency.IsValidIndex(To)) return Path;

	TArray<int32> Previous;
	Previous.Init(INDEX_NONE, Adjacency.Num());
	TArray<bool> Visited;
	Visited.Init(false, Adjacency.Num());

	TArray<int32> Queue = { From };
	Visited[From] = true;
	for (int32 Head = 0; Head < Queue.Num(); ++Head)
	{
		const int32 Current = Queue[Head];
		if (Current == To) break;
		for (const int32 Next : Adjacency[Current])
		{
			if (Visited[Next]) continue;
			Visited[Next] = true;
			Previous[Next] = Current;
			Queue.Add(Next);
		}
	}

	if (!Visited[To]) return Path;
	for (int32 Node = To; Node != INDEX_NONE; Node = Previous[Node])
		Path.Insert(Node, 0);
	return Path;
}

bool USSExplorationMapDefinition::Validate(TArray<FText>& OutErrors) const
{
	const int32 ErrorsBefore = OutErrors.Num();

	TSet<FName> SeenIds;
	for (const FSSExplorationRoom& Room : Rooms)
	{
		if (Room.RoomId.IsNone())
			OutErrors.Add(LOCTEXT("EmptyRoomId", "RoomId가 비어 있는 구역이 있습니다."));
		else if (SeenIds.Contains(Room.RoomId))
			OutErrors.Add(FText::Format(LOCTEXT("DuplicateRoomId", "RoomId '{0}'가 중복됩니다."), FText::FromName(Room.RoomId)));
		SeenIds.Add(Room.RoomId);
	}

	for (const FSSExplorationPassage& Passage : Passages)
	{
		if (FindRoomIndex(Passage.RoomA) == INDEX_NONE || FindRoomIndex(Passage.RoomB) == INDEX_NONE || Passage.RoomA == Passage.RoomB)
			OutErrors.Add(FText::Format(LOCTEXT("BadPassage", "통로 '{0}' - '{1}'가 잘못됐습니다."),
				FText::FromName(Passage.RoomA), FText::FromName(Passage.RoomB)));
	}

	const TArray<TArray<int32>> Adjacency = BuildAdjacency();
	const int32 Entrance = FindRoomIndex(EntranceRoomId);
	const int32 Exit = FindRoomIndex(ExitRoomId);
	if (Entrance == INDEX_NONE || Exit == INDEX_NONE)
	{
		OutErrors.Add(LOCTEXT("MissingEndpoints", "입구 또는 출구 구역이 지정되지 않았습니다."));
	}
	else
	{
		const TArray<int32> ExitPath = FindShortestPath(Adjacency, Entrance, Exit);
		if (ExitPath.IsEmpty())
			OutErrors.Add(LOCTEXT("Unreachable", "입구에서 출구까지 이어지는 경로가 없습니다."));
		else if (ExitPath.Num() - 1 > MaxTurns)
			OutErrors.Add(LOCTEXT("TooFar", "출구까지의 최소 이동 턴이 MaxTurns보다 많습니다."));
	}

	if (PatrolRoute.IsEmpty())
		OutErrors.Add(LOCTEXT("NoPatrol", "순찰 경로가 비어 있습니다."));
	for (int32 Index = 0; Index < PatrolRoute.Num(); ++Index)
	{
		const int32 Current = FindRoomIndex(PatrolRoute[Index]);
		const int32 Next = FindRoomIndex(PatrolRoute[(Index + 1) % PatrolRoute.Num()]);
		if (Current == INDEX_NONE || Next == INDEX_NONE)
		{
			OutErrors.Add(FText::Format(LOCTEXT("BadPatrolRoom", "순찰 경로 {0}번째 구역이 없습니다."), Index));
			continue;
		}
		if (PatrolRoute.Num() > 1 && Current != Next && !Adjacency[Current].Contains(Next))
			OutErrors.Add(FText::Format(LOCTEXT("PatrolGap", "순찰 경로 '{0}' → '{1}'가 통로로 연결되어 있지 않습니다."),
				FText::FromName(PatrolRoute[Index]), FText::FromName(PatrolRoute[(Index + 1) % PatrolRoute.Num()])));
	}
	if (!PatrolRoute.IsEmpty() && PatrolRoute[0] == EntranceRoomId)
		OutErrors.Add(LOCTEXT("GuardAtEntrance", "경비 로봇이 입구에서 시작하면 출발과 동시에 발각됩니다."));

	return OutErrors.Num() == ErrorsBefore;
}

#if WITH_EDITOR
EDataValidationResult USSExplorationMapDefinition::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	TArray<FText> Errors;
	if (!Validate(Errors))
	{
		for (const FText& Error : Errors) Context.AddError(Error);
		Result = EDataValidationResult::Invalid;
	}
	return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
