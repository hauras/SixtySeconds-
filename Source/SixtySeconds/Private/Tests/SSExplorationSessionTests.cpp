#include "Exploration/SSExplorationSession.h"
#include "Exploration/SSExplorationMapDefinition.h"
#include "Item/SSItemDefinition.h"
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
    const FSSExplorationResult CaughtResult = Session->MakeResult();
    TestTrue(TEXT("Caught result outcome"), CaughtResult.Outcome == ESSExplorationOutcome::Caught);
    TestEqual(TEXT("Caught result injury"), CaughtResult.Injury, Map->EmergencyInjury);
    TestEqual(TEXT("Caught result turns used"), CaughtResult.TurnsUsed, 2);

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
    const FSSExplorationResult TimeOutResult = Walker->MakeResult();
    TestTrue(TEXT("Time out result outcome"), TimeOutResult.Outcome == ESSExplorationOutcome::TimeOut);
    TestEqual(TEXT("Time out result injury"), TimeOutResult.Injury, Map->EmergencyInjury);
    TestEqual(TEXT("Time out result turns used"), TimeOutResult.TurnsUsed, 4);

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSExplorationLootTest, "SS.Exploration.SessionLoot",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSExplorationLootTest::RunTest(const FString& Parameters)
{
    // 아이템: 식량(1칸), 약품(2칸)
    USSItemDefinition* Food = NewObject<USSItemDefinition>();
    Food->ItemId = TEXT("Food");
    Food->CarryCost = 1;
    USSItemDefinition* Medkit = NewObject<USSItemDefinition>();
    Medkit->ItemId = TEXT("Medkit");
    Medkit->CarryCost = 2;

    // 지도: Entrance(0) - Storage(1) - Exit(2) - Far(3), 경비는 Far에 고정
    USSExplorationMapDefinition* Map = NewObject<USSExplorationMapDefinition>();
    for (const TCHAR* Id : { TEXT("Entrance"), TEXT("Storage"), TEXT("Exit"), TEXT("Far") })
    {
        Map->Rooms.AddDefaulted_GetRef().RoomId = Id;
    }
    FSSExplorationRoom& Storage = Map->Rooms[1];
    Storage.bSearchable = true;
    FSSItemStack FoodStack;   FoodStack.Item = Food;     FoodStack.Quantity = 3;
    FSSItemStack MedStack;    MedStack.Item = Medkit;    MedStack.Quantity = 1;
    Storage.Loot = { FoodStack, MedStack };

    const auto Connect = [Map](const TCHAR* A, const TCHAR* B)
    {
        FSSExplorationPassage& Passage = Map->Passages.AddDefaulted_GetRef();
        Passage.RoomA = A;
        Passage.RoomB = B;
    };
    Connect(TEXT("Entrance"), TEXT("Storage"));
    Connect(TEXT("Storage"), TEXT("Exit"));
    Connect(TEXT("Exit"), TEXT("Far"));
    Map->EntranceRoomId = TEXT("Entrance");
    Map->ExitRoomId = TEXT("Exit");
    Map->PatrolRoute = { TEXT("Far") };   // 경비가 움직이지 않아 수색 규칙만 확인
    Map->MaxTurns = 12;
    Map->CarryCapacity = 4;

    USSExplorationSession* Session = NewObject<USSExplorationSession>();
    TestTrue(TEXT("Initialize loot map"), Session->Initialize(Map));

    // 1. 수색 불가 방: 입구
    TestFalse(TEXT("Entrance is not searchable"), Session->CanSearch());
    TestFalse(TEXT("Search rejected at entrance"), Session->Search());
    TestEqual(TEXT("Rejected search uses no turn"), Session->GetRemainingTurns(), 12);
    TestFalse(TEXT("Cannot return from entrance"), Session->CanReturn());

    // 2. 대기: 턴만 1 줄어듦
    TestTrue(TEXT("Wait"), Session->Wait());
    TestEqual(TEXT("Wait uses one turn"), Session->GetRemainingTurns(), 11);
    TestEqual(TEXT("Wait keeps position"), Session->GetCurrentRoom(), 0);

    // 3. 수색 전에는 아무것도 담기지 않음
    TestTrue(TEXT("Move to storage"), Session->MoveTo(1));
    TestEqual(TEXT("Storage still has its loot before search"), Session->GetCurrentRoomLoot().Num(), 2);
    TestEqual(TEXT("Nothing carried before search"), Session->GetCarriedLoad(), 0);
    TestFalse(TEXT("Storage not searched yet"), Session->IsCurrentRoomSearched());

    // 4. 수색: 1턴, 들어가는 만큼 자동으로 담김
    //    식량 3개(1칸씩) → 3칸, 약품(2칸)은 3+2=5 > 4라 방에 남음
    TestTrue(TEXT("Can search storage"), Session->CanSearch());
    TestTrue(TEXT("Search storage"), Session->Search());
    TestEqual(TEXT("Search uses one turn"), Session->GetRemainingTurns(), 9);
    TestTrue(TEXT("Storage marked searched"), Session->IsCurrentRoomSearched());
    TestEqual(TEXT("Auto-collected up to capacity"), Session->GetCarriedLoad(), 3);
    TestEqual(TEXT("Same item stacks in one slot"), Session->GetCarried().Num(), 1);
    TestTrue(TEXT("Carried item is food"), Session->GetCarried()[0].Item == Food);
    TestEqual(TEXT("Carried food quantity"), Session->GetCarried()[0].Quantity, 3);
    TestEqual(TEXT("Only medkit left in room"), Session->GetCurrentRoomLoot().Num(), 1);
    TestTrue(TEXT("Leftover is medkit"), Session->GetCurrentRoomLoot()[0].Item == Medkit);

    // 5. 방마다 한 번만: 남은 물자가 있어도 다시 수색 불가
    TestFalse(TEXT("Cannot search twice"), Session->CanSearch());
    TestFalse(TEXT("Second search rejected"), Session->Search());
    TestEqual(TEXT("Rejected search uses no turn"), Session->GetRemainingTurns(), 9);
    TestEqual(TEXT("Load unchanged after rejected search"), Session->GetCarriedLoad(), 3);

    // 6. 귀환: 출구에서만, 턴을 안 쓰고, 이후 모든 행동 불가
    TestFalse(TEXT("Cannot return from storage"), Session->ReturnToShelter());
    TestTrue(TEXT("Move to exit"), Session->MoveTo(2));
    TestEqual(TEXT("Turns before return"), Session->GetRemainingTurns(), 8);
    TestTrue(TEXT("Can return at exit"), Session->CanReturn());
    TestTrue(TEXT("Return"), Session->ReturnToShelter());
    TestTrue(TEXT("Outcome is Returned"), Session->GetOutcome() == ESSExplorationOutcome::Returned);
    TestEqual(TEXT("Return uses no turn"), Session->GetRemainingTurns(), 8);
    TestEqual(TEXT("Carried items kept on return"), Session->GetCarriedLoad(), 3);
    TestFalse(TEXT("No acting after return"), Session->CanAct());
    TestFalse(TEXT("No moving after return"), Session->MoveTo(1));
    TestFalse(TEXT("No waiting after return"), Session->Wait());
    TestFalse(TEXT("No second return"), Session->ReturnToShelter());

    // 결과 요약: 귀환 성공이면 부상 없음, 운반함 그대로, 사용 턴 = 12 - 8
    const FSSExplorationResult Result = Session->MakeResult();
    TestTrue(TEXT("Result outcome Returned"), Result.Outcome == ESSExplorationOutcome::Returned);
    TestEqual(TEXT("Result carries food stack"), Result.Items.Num(), 1);
    TestEqual(TEXT("Result food quantity"), Result.Items.Num() > 0 ? Result.Items[0].Quantity : 0, 3);
    TestEqual(TEXT("No injury on return"), Result.Injury, 0.f);
    TestEqual(TEXT("Turns used"), Result.TurnsUsed, 4);

    // 7. 지도 에셋 원본은 그대로 (복사본만 줄어들었는지)
    TestEqual(TEXT("Map asset food untouched"), Map->Rooms[1].Loot[0].Quantity, 3);
    TestEqual(TEXT("Map asset loot count untouched"), Map->Rooms[1].Loot.Num(), 2);

    // 8. 턴 예산: 기본값은 지도 MaxTurns, 행동력으로 계산한 값을 넘기면 그걸 사용
    TestEqual(TEXT("Default budget is MaxTurns"), Session->GetTurnBudget(), 12);
    USSExplorationSession* Budgeted = NewObject<USSExplorationSession>();
    TestTrue(TEXT("Initialize with AP budget"), Budgeted->Initialize(Map, Map->GetTurnBudget(2)));
    TestEqual(TEXT("Budget from 2 AP"), Budgeted->GetTurnBudget(), 8);
    TestEqual(TEXT("Remaining starts at budget"), Budgeted->GetRemainingTurns(), 8);
    TestTrue(TEXT("Budgeted wait"), Budgeted->Wait());
    TestEqual(TEXT("Turns used counts from budget"), Budgeted->MakeResult().TurnsUsed, 1);

    return true;
}
#endif
