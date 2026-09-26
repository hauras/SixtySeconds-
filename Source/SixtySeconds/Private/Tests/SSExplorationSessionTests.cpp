#include "Exploration/SSExplorationSession.h"
#include "Exploration/SSExplorationMapDefinition.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSExplorationSessionTest, "SS.Exploration.SessionMove",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSExplorationSessionTest::RunTest(const FString& Parameters)
{
    // 지도: Entrance(0) - Hall(1) - Exit(3), Hall(1) - Storage(2)
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
    Map->PatrolRoute = { TEXT("Exit")};   // Validate 통과용
    Map->MaxTurns = 2;                                      // 턴 소진을 빨리 확인하려고 2턴

    // 1. 초기화: 입구에서 시작, 턴 = MaxTurns
    USSExplorationSession* Session = NewObject<USSExplorationSession>();
    TestTrue(TEXT("Initialize with valid map"), Session->Initialize(Map));
    TestEqual(TEXT("Starts at entrance"), Session->GetCurrentRoom(), 0);
    TestEqual(TEXT("Starts with max turns"), Session->GetRemainingTurns(), 2);

    // 2. 연결 안 된 방: 실패하고 상태도 그대로
    TestFalse(TEXT("Cannot move to unconnected room"), Session->MoveTo(3));
    TestEqual(TEXT("Position unchanged after failed move"), Session->GetCurrentRoom(), 0);
    TestEqual(TEXT("Turns unchanged after failed move"), Session->GetRemainingTurns(), 2);

    // 3. 연결된 방: 이동하고 턴 1 감소
    TestTrue(TEXT("Move to connected room"), Session->MoveTo(1));
    TestEqual(TEXT("Moved to hall"), Session->GetCurrentRoom(), 1);
    TestEqual(TEXT("One turn used"), Session->GetRemainingTurns(), 1);

    // 4. 마지막 턴 사용 후에는 연결된 방이어도 이동 불가
    TestTrue(TEXT("Use last turn"), Session->MoveTo(2));
    TestEqual(TEXT("No turns left"), Session->GetRemainingTurns(), 0);
    TestFalse(TEXT("CanMoveTo false with no turns"), Session->CanMoveTo(1));
    TestFalse(TEXT("MoveTo false with no turns"), Session->MoveTo(1));
    TestEqual(TEXT("Position unchanged with no turns"), Session->GetCurrentRoom(), 2);

    // 5. 잘못된 인덱스
    USSExplorationSession* Fresh = NewObject<USSExplorationSession>();
    Fresh->Initialize(Map);
    TestFalse(TEXT("Negative index rejected"), Fresh->MoveTo(-1));
    TestFalse(TEXT("Out of range index rejected"), Fresh->MoveTo(99));

    // 6. 초기화 전 세션은 어디로도 이동 불가
    USSExplorationSession* Uninitialized = NewObject<USSExplorationSession>();
    TestFalse(TEXT("Uninitialized session cannot move"), Uninitialized->CanMoveTo(1));

    // 7. 초기화 실패: null 지도, 잘못된 지도
    TestFalse(TEXT("Null map rejected"), NewObject<USSExplorationSession>()->Initialize(nullptr));
    Map->EntranceRoomId = NAME_None;   // Validate 실패 → 경고 로그가 찍히는 게 정상
    TestFalse(TEXT("Invalid map rejected"), NewObject<USSExplorationSession>()->Initialize(Map));

    // 8. 정상 세션을 잘못된 지도로 재초기화하면 이전 상태가 남지 않아야 함
    TestTrue(TEXT("Fresh session was usable"), Fresh->GetCurrentRoom() == 0);
    TestFalse(TEXT("Reinitialize with invalid map fails"), Fresh->Initialize(Map));
    TestEqual(TEXT("Position cleared after failed reinit"), Fresh->GetCurrentRoom(), (int32)INDEX_NONE);
    TestEqual(TEXT("Turns cleared after failed reinit"), Fresh->GetRemainingTurns(), 0);
    TestFalse(TEXT("Cannot move after failed reinit"), Fresh->CanMoveTo(1));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSExplorationGuardTest, "SS.Exploration.SessionGuard",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSExplorationGuardTest::RunTest(const FString& Parameters)
{
    // 일자 지도: Entrance(0) - A(1) - B(2) - C(3) - Exit(4)
    USSExplorationMapDefinition* Map = NewObject<USSExplorationMapDefinition>();
    for (const TCHAR* Id : { TEXT("Entrance"), TEXT("A"), TEXT("B"), TEXT("C"), TEXT("Exit") })
    {
        Map->Rooms.AddDefaulted_GetRef().RoomId = Id;
    }
    const auto Connect = [Map](const TCHAR* A, const TCHAR* B)
    {
        FSSExplorationPassage& Passage = Map->Passages.AddDefaulted_GetRef();
        Passage.RoomA = A;
        Passage.RoomB = B;
    };
    Connect(TEXT("Entrance"), TEXT("A"));
    Connect(TEXT("A"), TEXT("B"));
    Connect(TEXT("B"), TEXT("C"));
    Connect(TEXT("C"), TEXT("Exit"));
    Map->EntranceRoomId = TEXT("Entrance");
    Map->ExitRoomId = TEXT("Exit");
    Map->PatrolRoute = { TEXT("C"), TEXT("B") };   // 경비: C(3) ↔ B(2) 왕복
    Map->MaxTurns = 4;

    // 1. 순찰과 예고: 예고한 방으로 실제로 이동하는지
    USSExplorationSession* Session = NewObject<USSExplorationSession>();
    TestTrue(TEXT("Initialize guard map"), Session->Initialize(Map));
    TestEqual(TEXT("Guard starts at first patrol room"), Session->GetGuardRoom(), 3);
    TestEqual(TEXT("Guard preview is next patrol room"), Session->GetGuardNextRoom(), 2);
    TestTrue(TEXT("Starts in progress"), Session->GetOutcome() == ESSExplorationOutcome::InProgress);

    TestTrue(TEXT("Player moves to A"), Session->MoveTo(1));
    TestEqual(TEXT("Guard moved to previewed room"), Session->GetGuardRoom(), 2);
    TestTrue(TEXT("Still in progress when rooms differ"), Session->GetOutcome() == ESSExplorationOutcome::InProgress);
    TestEqual(TEXT("Preview wraps to start of patrol"), Session->GetGuardNextRoom(), 3);

    // 2. 경비가 있는 방에 들어가면 발각, 경비는 그 자리에 남음
    TestTrue(TEXT("Player walks into guard room"), Session->MoveTo(2));
    TestTrue(TEXT("Caught when entering guard room"), Session->GetOutcome() == ESSExplorationOutcome::Caught);
    TestEqual(TEXT("Guard stays after catching"), Session->GetGuardRoom(), 2);
    TestFalse(TEXT("No moves after caught"), Session->CanMoveTo(1));
    TestFalse(TEXT("MoveTo rejected after caught"), Session->MoveTo(1));

    // 3. 경비를 피해 다니며 턴을 다 쓰면 시간 초과
    USSExplorationSession* Walker = NewObject<USSExplorationSession>();
    Walker->Initialize(Map);
    TestTrue(TEXT("Walker to A"), Walker->MoveTo(1));   // 플레이어 A, 경비 B
    TestTrue(TEXT("Walker back to entrance"), Walker->MoveTo(0));   // 플레이어 입구, 경비 C
    TestTrue(TEXT("Walker to A again"), Walker->MoveTo(1));   // 플레이어 A, 경비 B
    TestTrue(TEXT("Walker steps back again"), Walker->MoveTo(0));   // 플레이어 입구, 경비 C → 턴 0
    TestEqual(TEXT("All turns used"), Walker->GetRemainingTurns(), 0);
    TestTrue(TEXT("Time out after last turn"), Walker->GetOutcome() == ESSExplorationOutcome::TimeOut);
    TestFalse(TEXT("No moves after time out"), Walker->CanMoveTo(1));

    // 4. 경비가 플레이어 방으로 걸어 들어와도 발각
    Map->PatrolRoute = { TEXT("B"), TEXT("A") };   // 경비: B(2) ↔ A(1)
    Map->MaxTurns = 12;
    USSExplorationSession* Ambush = NewObject<USSExplorationSession>();
    TestTrue(TEXT("Initialize ambush map"), Ambush->Initialize(Map));
    TestTrue(TEXT("Player moves to A"), Ambush->MoveTo(1));   // 플레이어가 A로 가는 순간 경비는 B → 다음에 A
    TestTrue(TEXT("Caught when guard walks into player room"), Ambush->GetOutcome() == ESSExplorationOutcome::Caught);
    TestEqual(TEXT("Guard entered player room"), Ambush->GetGuardRoom(), 1);

    return true;
}
#endif
