#include "Event/SSEventCatalog.h"
#include "Event/SSEventDirector.h"
#include "Event/SSEventTypes.h"
#include "Item/SSRunSubsystem.h"
#include "Item/SSItemDefinition.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace SSEventTest
{
    USSItemDefinition* MakeItem(UObject* Outer, FName Id, ESSItemUseEffect Effect)
    {
        USSItemDefinition* Item = NewObject<USSItemDefinition>(Outer);
        Item->ItemId = Id;
        Item->UseEffect = Effect;
        Item->EffectAmount = 40.f;
        return Item;
    }

    UDataTable* MakeTable(UScriptStruct* RowStruct)
    {
        UDataTable* Table = NewObject<UDataTable>();
        Table->RowStruct = RowStruct;
        return Table;
    }

    FSSEventChoiceRow Choice(FName EventId, int32 Order, ESSEventCondition Condition = ESSEventCondition::None, FName Target = NAME_None)
    {
        FSSEventChoiceRow Row;
        Row.EventId = EventId;
        Row.Order = Order;
        Row.Text = FText::FromName(EventId);
        Row.Condition = Condition;
        Row.ConditionTarget = Target;
        return Row;
    }

    FSSEventEffectRow Effect(FName ChoiceId, ESSEventEffect Type, FName Target, int32 Amount, const TCHAR* Text = TEXT(""))
    {
        FSSEventEffectRow Row;
        Row.ChoiceId = ChoiceId;
        Row.Type = Type;
        Row.Target = Target;
        Row.Amount = Amount;
        Row.Text = FText::FromString(Text);
        return Row;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSEventDirectorTest, "SS.Event.Director",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSEventDirectorTest::RunTest(const FString& Parameters)
{
    using namespace SSEventTest;

    UGameInstance* GameInstance = NewObject<UGameInstance>();
    USSRunSubsystem* Run = NewObject<USSRunSubsystem>(GameInstance);
    USSItemDefinition* Food = MakeItem(Run, TEXT("Food"), ESSItemUseEffect::RestoreSatiety);
    USSItemDefinition* Water = MakeItem(Run, TEXT("Water"), ESSItemUseEffect::RestoreHydration);

    // 사건
    //  Knock:  Day 2부터, 한 번만. 열어주기(식량 필요) → 식량 -1, 기록, 2일 뒤 Reward 예약 / 무시 → 체력 -10
    //  Reward: 예약으로만 등장. 받기 → 물 +2
    //  Leak:   물이 있을 때, 쿨다운 2일. 고치기 → 행동력 -1
    USSEventCatalog* Catalog = NewObject<USSEventCatalog>();
    Catalog->Items = { Food, Water };
    Catalog->DailyEventChance = 1.f;   // 테스트는 매일 사건이 나오게

    UDataTable* Events = MakeTable(FSSEventRow::StaticStruct());
    FSSEventRow Knock;  Knock.MinDay = 2;  Knock.bOnceOnly = true;
    FSSEventRow Reward; Reward.bScheduledOnly = true;
    FSSEventRow Leak;   Leak.Condition = ESSEventCondition::HasItem; Leak.ConditionTarget = TEXT("Water"); Leak.Cooldown = 2;
    Events->AddRow(TEXT("Knock"), Knock);
    Events->AddRow(TEXT("Reward"), Reward);
    Events->AddRow(TEXT("Leak"), Leak);

    UDataTable* Choices = MakeTable(FSSEventChoiceRow::StaticStruct());
    Choices->AddRow(TEXT("Knock_Open"), Choice(TEXT("Knock"), 0, ESSEventCondition::HasItem, TEXT("Food")));
    Choices->AddRow(TEXT("Knock_Ignore"), Choice(TEXT("Knock"), 1));
    Choices->AddRow(TEXT("Reward_Take"), Choice(TEXT("Reward"), 0));
    Choices->AddRow(TEXT("Leak_Fix"), Choice(TEXT("Leak"), 0));

    UDataTable* Effects = MakeTable(FSSEventEffectRow::StaticStruct());
    Effects->AddRow(TEXT("E1"), Effect(TEXT("Knock_Open"), ESSEventEffect::Item, TEXT("Food"), -1));
    Effects->AddRow(TEXT("E2"), Effect(TEXT("Knock_Open"), ESSEventEffect::Journal, NAME_None, 0, TEXT("Shared food")));
    Effects->AddRow(TEXT("E3"), Effect(TEXT("Knock_Open"), ESSEventEffect::ScheduleEvent, TEXT("Reward"), 2));
    Effects->AddRow(TEXT("E4"), Effect(TEXT("Knock_Ignore"), ESSEventEffect::PlayerHealth, NAME_None, -10));
    Effects->AddRow(TEXT("E5"), Effect(TEXT("Reward_Take"), ESSEventEffect::Item, TEXT("Water"), 2));
    Effects->AddRow(TEXT("E6"), Effect(TEXT("Leak_Fix"), ESSEventEffect::ActionPoints, NAME_None, -1));

    Catalog->EventTable = Events;
    Catalog->ChoiceTable = Choices;
    Catalog->EffectTable = Effects;

    // 1. 카탈로그 검사
    TArray<FText> Errors;
    TestTrue(TEXT("Valid catalog passes"), Catalog->Validate(Errors));

    Effects->AddRow(TEXT("Bad"), Effect(TEXT("NoSuchChoice"), ESSEventEffect::Item, TEXT("Battery"), 1));
    Errors.Reset();
    TestFalse(TEXT("Effect pointing to missing choice / unknown item rejected"), Catalog->Validate(Errors));
    Effects->RemoveRow(TEXT("Bad"));

    Events->AddRow(TEXT("Lonely"), FSSEventRow());
    Errors.Reset();
    TestFalse(TEXT("Event without choices rejected"), Catalog->Validate(Errors));
    Events->RemoveRow(TEXT("Lonely"));

    // 2. 디렉터 연결
    USSEventDirector* Director = Run->GetEventDirector();
    Director->SetSeed(1234);
    TestTrue(TEXT("Director accepts valid catalog"), Director->SetCatalog(Catalog));

    FSSItemStack FoodStack;  FoodStack.Item = Food;   FoodStack.Quantity = 1;
    FSSItemStack WaterStack; WaterStack.Item = Water; WaterStack.Quantity = 1;
    Run->DepositItems({ FoodStack, WaterStack });

    // 3. Day 1: Knock은 Day 2부터, Reward는 예약 전용 → Leak만 가능
    TestEqual(TEXT("Day 1 picks Leak"), Director->PickEventForToday(*Run), FName(TEXT("Leak")));
    TestFalse(TEXT("Choice from another event rejected"), Director->ApplyChoice(TEXT("Leak"), TEXT("Knock_Open"), *Run));
    TestTrue(TEXT("Apply Leak_Fix"), Director->ApplyChoice(TEXT("Leak"), TEXT("Leak_Fix"), *Run));
    TestEqual(TEXT("Action point effect"), Run->GetActionPoints(), USSRunSubsystem::MaxActionPoints - 1);

    // 4. Day 2: Leak은 쿨다운 → Knock
    TestTrue(TEXT("Advance to day 2"), Run->AdvanceDay(false, false));
    TestEqual(TEXT("Day 2 picks Knock"), Director->PickEventForToday(*Run), FName(TEXT("Knock")));
    const TArray<FSSEventChoiceView> KnockChoices = Director->GetChoices(TEXT("Knock"), *Run);
    TestEqual(TEXT("Knock has two choices"), KnockChoices.Num(), 2);
    TestTrue(TEXT("Choices sorted by Order"), KnockChoices.Num() == 2 && KnockChoices[0].ChoiceId == TEXT("Knock_Open"));
    TestTrue(TEXT("Open available with food"), KnockChoices.Num() == 2 && KnockChoices[0].bAvailable);
    TestTrue(TEXT("Apply Knock_Open"), Director->ApplyChoice(TEXT("Knock"), TEXT("Knock_Open"), *Run));
    TestEqual(TEXT("Food spent"), Run->GetStoredQuantityById(TEXT("Food")), 0);
    TestTrue(TEXT("Journal entry recorded"), Run->GetJournalEntries().ContainsByPredicate(
        [](const FSSJournalEntry& Entry) { return Entry.Event == ESSJournalEvent::Event; }));

    // 5. Day 3: Knock 1회성, Leak 쿨다운, Reward는 Day 4 예약 → 없음
    TestTrue(TEXT("Advance to day 3"), Run->AdvanceDay(false, false));
    TestTrue(TEXT("Day 3 quiet"), Director->PickEventForToday(*Run).IsNone());

    // 6. Day 4: 예약된 Reward
    TestTrue(TEXT("Advance to day 4"), Run->AdvanceDay(false, false));
    TestEqual(TEXT("Day 4 picks scheduled Reward"), Director->PickEventForToday(*Run), FName(TEXT("Reward")));
    TestTrue(TEXT("Apply Reward_Take"), Director->ApplyChoice(TEXT("Reward"), TEXT("Reward_Take"), *Run));
    TestEqual(TEXT("Water gained from catalog item"), Run->GetStoredQuantityById(TEXT("Water")), 3);

    // 7. 조건이 안 맞는 선택지: 식량이 없으면 열어주기 불가
    const TArray<FSSEventChoiceView> NoFood = Director->GetChoices(TEXT("Knock"), *Run);
    TestFalse(TEXT("Open unavailable without food"), NoFood.Num() == 2 && NoFood[0].bAvailable);
    TestFalse(TEXT("Unavailable choice rejected"), Director->ApplyChoice(TEXT("Knock"), TEXT("Knock_Open"), *Run));
    TestTrue(TEXT("Ignore still possible"), Director->ApplyChoice(TEXT("Knock"), TEXT("Knock_Ignore"), *Run));
    TestEqual(TEXT("Health effect"), Run->GetHealth(), 90.f);

    // 8. 확률 0이면 예약 없이는 사건 없음
    Catalog->DailyEventChance = 0.f;
    TestTrue(TEXT("Advance to day 5"), Run->AdvanceDay(false, false));
    TestTrue(TEXT("No event when daily chance is zero"), Director->PickEventForToday(*Run).IsNone());

    // 9. 새 게임이면 1회성 기록 초기화
    Catalog->DailyEventChance = 1.f;
    Run->ResetRun();
    Run->DepositItems({ FoodStack });
    TestTrue(TEXT("Advance after reset"), Run->AdvanceDay(false, false));
    TestEqual(TEXT("Once-only event returns after reset"), Director->PickEventForToday(*Run), FName(TEXT("Knock")));

    return true;
}
#endif
