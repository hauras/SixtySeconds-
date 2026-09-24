#include "Exploration/SSExplorationMapDefinition.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSExplorationMapTest, "SS.Exploration.MapValidation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSExplorationMapTest::RunTest(const FString& Parameters)
{
    USSExplorationMapDefinition* Map = NewObject<USSExplorationMapDefinition>();
    for (const TCHAR* Id : { TEXT("Entrance"), TEXT("Hall"), TEXT("Storage"), TEXT("Exit") })
    {
        FSSExplorationRoom& Room = Map->Rooms.AddDefaulted_GetRef();
        Room.RoomId = Id;
    }
    const auto Connect = [Map](const TCHAR* A, const TCHAR* B)
    {
        FSSExplorationPassage& Passage = Map->Passages.AddDefaulted_GetRef();
        Passage.RoomA = A;
        Passage.RoomB = B;
    };
    Connect(TEXT("Entrance"), TEXT("Hall"));
    Connect(TEXT("Hall"), TEXT("Storage"));
    Connect(TEXT("Hall"), TEXT("Exit"));
    Map->EntranceRoomId = TEXT("Entrance");
    Map->ExitRoomId = TEXT("Exit");
    Map->PatrolRoute = { TEXT("Storage"), TEXT("Hall") };

    TArray<FText> Errors;
    TestTrue(TEXT("Valid map passes"), Map->Validate(Errors));

    const TArray<TArray<int32>> Adjacency = Map->BuildAdjacency();
    const TArray<int32> Path = USSExplorationMapDefinition::FindShortestPath(Adjacency, 0, 3);
    TestEqual(TEXT("Shortest path length"), Path.Num(), 3);
    TestEqual(TEXT("Path passes hall"), Path.IsValidIndex(1) ? Path[1] : INDEX_NONE, 1);
    TestEqual(TEXT("Path to self"), USSExplorationMapDefinition::FindShortestPath(Adjacency, 2, 2).Num(), 1);

    Map->MaxTurns = 1;
    Errors.Reset();
    TestFalse(TEXT("Exit farther than turn limit"), Map->Validate(Errors));
    Map->MaxTurns = 12;

    Map->Passages.RemoveAt(2);
    Errors.Reset();
    TestFalse(TEXT("Unreachable exit rejected"), Map->Validate(Errors));
    Connect(TEXT("Hall"), TEXT("Exit"));

    Map->PatrolRoute = { TEXT("Storage"), TEXT("Exit") };
    Errors.Reset();
    TestFalse(TEXT("Patrol gap rejected"), Map->Validate(Errors));

    Map->PatrolRoute = { TEXT("Entrance"), TEXT("Hall") };
    Errors.Reset();
    TestFalse(TEXT("Guard starting at entrance rejected"), Map->Validate(Errors));

    Map->PatrolRoute = { TEXT("Storage"), TEXT("Hall") };
    Map->Rooms[2].RoomId = TEXT("Hall");
    Errors.Reset();
    TestFalse(TEXT("Duplicate room id rejected"), Map->Validate(Errors));
    return true;
}
#endif
