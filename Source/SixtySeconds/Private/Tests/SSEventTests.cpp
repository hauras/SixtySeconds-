#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/WrapBox.h"
#include "Event/SSEventCatalog.h"
#include "Event/SSEventDirector.h"
#include "Event/SSEventTypes.h"
#include "Item/SSRunSubsystem.h"
#include "Item/SSItemDefinition.h"
#include "UI/Event/SSEventWidget.h"
#include "SSEventWidgetTestListener.h"
#include "Ara/SSAraDirector.h"
#include "Character/SSSurvivorDefinition.h"
#include "Companion/SSCompanionState.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace SSEventTest
{
    USSItemDefinition* MakeItem(UObject* Outer, FName Id, ESSItemUseEffect Effect)
    {
        USSItemDefinition* Item = NewObject<USSItemDefinition>(Outer);
        Item->ItemId = Id;
        Item->UseEffect = Effect;
        Item->EffectAmount = 40.f;
        Item->DisplayName = FText::FromName(Id);   // 결과 문장에 "Food -1"처럼 이름이 나오게
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
    Effects->AddRow(TEXT("E7"), Effect(TEXT("Knock_Ignore"), ESSEventEffect::Item, TEXT("Water"), -5));   // 가진 것보다 많이 잃기
    FSSEventEffectRow Never = Effect(TEXT("Knock_Ignore"), ESSEventEffect::ActionPoints, NAME_None, -1);
    Never.Chance = 0.f;                                                                                  // 절대 안 일어나는 효과
    Effects->AddRow(TEXT("E8"), Never);

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

    FSSEventResult Result;   // 선택 결과 (선택할 때마다 새로 채워짐)
    const auto HasChange = [&Result](ESSEventEffect Type, FName Target, int32 Amount)
    {
        return Result.Changes.ContainsByPredicate([&](const FSSEventChange& Change)
        {
            return Change.Type == Type && Change.Target == Target && Change.Amount == Amount;
        });
    };

    // 3. Day 1: Knock은 Day 2부터, Reward는 예약 전용 → Leak만 가능
    TestEqual(TEXT("Day 1 picks Leak"), Director->PickEventForToday(*Run), FName(TEXT("Leak")));
    TestFalse(TEXT("Choice from another event rejected"), Director->ApplyChoice(TEXT("Leak"), TEXT("Knock_Open"), *Run, Result));
    TestTrue(TEXT("Apply Leak_Fix"), Director->ApplyChoice(TEXT("Leak"), TEXT("Leak_Fix"), *Run, Result));
    TestEqual(TEXT("Action point effect"), Run->GetActionPoints(), USSRunSubsystem::MaxActionPoints - 1);
    TestTrue(TEXT("Result records action point change"), HasChange(ESSEventEffect::ActionPoints, NAME_None, -1));

    // 4. Day 2: Leak은 쿨다운 → Knock
    TestTrue(TEXT("Advance to day 2"), Run->AdvanceDay(false, false));
    TestEqual(TEXT("Day 2 picks Knock"), Director->PickEventForToday(*Run), FName(TEXT("Knock")));
    const TArray<FSSEventChoiceView> KnockChoices = Director->GetChoices(TEXT("Knock"), *Run);
    TestEqual(TEXT("Knock has two choices"), KnockChoices.Num(), 2);
    TestTrue(TEXT("Choices sorted by Order"), KnockChoices.Num() == 2 && KnockChoices[0].ChoiceId == TEXT("Knock_Open"));
    TestTrue(TEXT("Open available with food"), KnockChoices.Num() == 2 && KnockChoices[0].bAvailable);
    TestTrue(TEXT("Apply Knock_Open"), Director->ApplyChoice(TEXT("Knock"), TEXT("Knock_Open"), *Run, Result));
    TestEqual(TEXT("Food spent"), Run->GetStoredQuantityById(TEXT("Food")), 0);
    TestEqual(TEXT("Result keeps chosen text"), Result.ChoiceText.ToString(), FString(TEXT("Knock")));
    TestTrue(TEXT("Result keeps journal line"), Result.Lines.Num() == 1 && Result.Lines[0].ToString() == TEXT("Shared food"));
    TestTrue(TEXT("Result records food loss"), HasChange(ESSEventEffect::Item, TEXT("Food"), -1));
    TestEqual(TEXT("Scheduled follow-up is hidden from result"), Result.Changes.Num(), 1);
    TestEqual(TEXT("Changes described"), Director->DescribeChanges(Result).ToString(), FString(TEXT("Food -1")));
    const TArray<FSSJournalEntry>& Journal = Run->GetJournalEntries();
    const int32 EventIndex = Journal.FindLastByPredicate(   // 방금 고른 Knock의 기록 (앞에 Leak 기록이 있음)
        [](const FSSJournalEntry& Entry) { return Entry.Event == ESSJournalEvent::Event; });
    const FSSJournalEntry* EventEntry = Journal.IsValidIndex(EventIndex) ? &Journal[EventIndex] : nullptr;
    TestTrue(TEXT("One journal line with result and change"), EventEntry
        && EventEntry->Message.ToString().Contains(TEXT("Shared food"))
        && EventEntry->Message.ToString().Contains(TEXT("(Food -1)")));

    // 5. Day 3: Knock 1회성, Leak 쿨다운, Reward는 Day 4 예약 → 없음
    TestTrue(TEXT("Advance to day 3"), Run->AdvanceDay(false, false));
    TestTrue(TEXT("Day 3 quiet"), Director->PickEventForToday(*Run).IsNone());

    // 6. Day 4: 예약된 Reward
    TestTrue(TEXT("Advance to day 4"), Run->AdvanceDay(false, false));
    TestEqual(TEXT("Day 4 picks scheduled Reward"), Director->PickEventForToday(*Run), FName(TEXT("Reward")));
    TestTrue(TEXT("Apply Reward_Take"), Director->ApplyChoice(TEXT("Reward"), TEXT("Reward_Take"), *Run, Result));
    TestEqual(TEXT("Water gained from catalog item"), Run->GetStoredQuantityById(TEXT("Water")), 3);

    // 7. 조건이 안 맞는 선택지: 식량이 없으면 열어주기 불가
    const TArray<FSSEventChoiceView> NoFood = Director->GetChoices(TEXT("Knock"), *Run);
    TestFalse(TEXT("Open unavailable without food"), NoFood.Num() == 2 && NoFood[0].bAvailable);
    TestFalse(TEXT("Unavailable choice rejected"), Director->ApplyChoice(TEXT("Knock"), TEXT("Knock_Open"), *Run, Result));
    TestTrue(TEXT("Ignore still possible"), Director->ApplyChoice(TEXT("Knock"), TEXT("Knock_Ignore"), *Run, Result));
    TestEqual(TEXT("Health effect"), Run->GetHealth(), 90.f);
    TestTrue(TEXT("Result records health change"), HasChange(ESSEventEffect::PlayerHealth, NAME_None, -10));
    TestTrue(TEXT("Loss capped to what was held"), HasChange(ESSEventEffect::Item, TEXT("Water"), -3));
    TestEqual(TEXT("Water gone"), Run->GetStoredQuantityById(TEXT("Water")), 0);
    TestFalse(TEXT("Failed chance not in result"), HasChange(ESSEventEffect::ActionPoints, NAME_None, -1));

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSEventChangeDescriptionTest, "SS.Event.ChangeDescription",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSEventChangeDescriptionTest::RunTest(const FString& Parameters)
{
    using namespace SSEventTest;
    USSEventCatalog* Catalog = NewObject<USSEventCatalog>();
    USSItemDefinition* Food = MakeItem(Catalog, TEXT("Food"), ESSItemUseEffect::RestoreSatiety);
    Food->DisplayName = FText::FromString(TEXT("식량"));
    Catalog->Items = { Food };
    Catalog->EventTable = MakeTable(FSSEventRow::StaticStruct());
    Catalog->EventTable->AddRow(TEXT("Sample"), FSSEventRow());
    Catalog->ChoiceTable = MakeTable(FSSEventChoiceRow::StaticStruct());
    Catalog->ChoiceTable->AddRow(TEXT("Pick"), Choice(TEXT("Sample"), 0));
    Catalog->EffectTable = MakeTable(FSSEventEffectRow::StaticStruct());
    USSEventDirector* Director = NewObject<USSEventDirector>();
    Director->SetSeed(1234);
    if (!TestTrue(TEXT("Description catalog is valid"), Director->SetCatalog(Catalog))) return false;

    const FSSEventChange Gain{ ESSEventEffect::Item, TEXT("Food"), 2 };
    const FSSEventChange Cost{ ESSEventEffect::ActionPoints, NAME_None, -1 };
    const FSSEventChange Journal{ ESSEventEffect::Journal, NAME_None, 0 };
    const FSSEventChange Scheduled{ ESSEventEffect::ScheduleEvent, TEXT("Sample"), 2 };
    TestEqual(TEXT("Item uses display name and positive sign"), Director->DescribeChange(Gain).ToString(), FString(TEXT("식량 +2")));
    TestEqual(TEXT("Action points keep negative sign"), Director->DescribeChange(Cost).ToString(), FString(TEXT("행동력 -1")));
    TestTrue(TEXT("Journal is not a visible change"), Director->DescribeChange(Journal).IsEmpty());
    TestTrue(TEXT("Scheduled event is not revealed"), Director->DescribeChange(Scheduled).IsEmpty());
    TestEqual(TEXT("Unknown item falls back to ID"), Director->DescribeChange(
        { ESSEventEffect::Item, TEXT("Unknown"), -2 }).ToString(), FString(TEXT("Unknown -2")));
    FSSEventResult Result;
    Result.Changes = { Gain, Journal, Cost, Scheduled };
    TestEqual(TEXT("Joined description keeps previous formatting"), Director->DescribeChanges(Result).ToString(), FString(TEXT("식량 +2, 행동력 -1")));
    Result.Changes.Reset();
    TestTrue(TEXT("Empty changes keep empty journal description"), Director->DescribeChanges(Result).IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSEventWidgetResultTest, "SS.Event.WidgetResultStep",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSEventWidgetResultTest::RunTest(const FString& Parameters)
{
    using namespace SSEventTest;
    UGameInstance* Game = NewObject<UGameInstance>(GEngine);
    Game->InitializeStandalone();
    USSRunSubsystem* Run = Game->GetSubsystem<USSRunSubsystem>();
    Run->ResetRun();
    USSEventCatalog* Catalog = NewObject<USSEventCatalog>(Run);
    USSItemDefinition* Food = MakeItem(Catalog, TEXT("Food"), ESSItemUseEffect::RestoreSatiety);
    Food->DisplayName = FText::FromString(TEXT("식량"));
    Catalog->Items = { Food };
    Catalog->EventTable = MakeTable(FSSEventRow::StaticStruct());
    FSSEventRow Event;
    Event.Title = FText::FromString(TEXT("밤의 소리"));
    Event.Body = FText::FromString(TEXT("문밖에서 소리가 들린다."));
    Catalog->EventTable->AddRow(TEXT("Sample"), Event);
    Catalog->EventTable->AddRow(TEXT("Quiet"), Event);
    Catalog->ChoiceTable = MakeTable(FSSEventChoiceRow::StaticStruct());
    FSSEventChoiceRow Selected = Choice(TEXT("Sample"), 0);
    Selected.Text = FText::FromString(TEXT("불을 끈다"));
    Catalog->ChoiceTable->AddRow(TEXT("Pick"), Selected);
    Catalog->ChoiceTable->AddRow(TEXT("Blocked"), Choice(
        TEXT("Sample"),
        1,
        ESSEventCondition::HasItem,
        TEXT("Food")));
    Catalog->ChoiceTable->AddRow(TEXT("QuietPick"), Choice(TEXT("Quiet"), 0));
    Catalog->EffectTable = MakeTable(FSSEventEffectRow::StaticStruct());
    Catalog->EffectTable->AddRow(TEXT("Cost"), Effect(TEXT("Pick"), ESSEventEffect::ActionPoints, NAME_None, -1));
    Catalog->EffectTable->AddRow(TEXT("Food"), Effect(TEXT("Pick"), ESSEventEffect::Item, TEXT("Food"), 2));
    Catalog->EffectTable->AddRow(TEXT("Line1"), Effect(TEXT("Pick"), ESSEventEffect::Journal, NAME_None, 0, TEXT("빛이 지나갔다.")));
    Catalog->EffectTable->AddRow(TEXT("Line2"), Effect(TEXT("Pick"), ESSEventEffect::Journal, NAME_None, 0, TEXT("소리가 멎었다.")));
    USSEventDirector* Director = Run->GetEventDirector();
    Director->SetSeed(1234);
    if (!TestTrue(TEXT("Widget catalog is valid"), Director->SetCatalog(Catalog)))
    {
        Game->Shutdown();
        return false;
    }

    USSEventWidget* Widget = CreateWidget<USSEventWidget>(Game);
    USSEventWidgetTestListener* Listener = NewObject<USSEventWidgetTestListener>(Game);
    Widget->OnEventFinished.AddUniqueDynamic(Listener, &USSEventWidgetTestListener::HandleFinished);
    // 명령줄 테스트에는 게임 화면이 없으므로 부모 위젯에 붙여 닫힘을 확인한다.
    UVerticalBox* Host = NewObject<UVerticalBox>(Game);
    Host->AddChildToVerticalBox(Widget);
    Host->TakeWidget();
    Widget->ShowEvent(Director, Run, TEXT("Sample"));
    Widget->HandleConfirm();
    TestEqual(TEXT("Confirm before choosing does nothing"), Listener->FinishedCount, 0);
    Widget->Choose(1);
    TestFalse(TEXT("Failed choice keeps selection step"), Widget->bShowingResult);
    TestTrue(TEXT("Failed choice keeps body visible"), Widget->BodyText->GetVisibility() == ESlateVisibility::Visible);
    Widget->ChoiceButton0->OnClicked.Broadcast();
    TestTrue(TEXT("Successful choice keeps window attached"), Widget->GetParent() == Host);
    TestEqual(TEXT("Choosing does not finish night"), Listener->FinishedCount, 0);
    TestTrue(TEXT("Result step is visible"), Widget->ResultBox->GetVisibility() == ESlateVisibility::Visible);
    TestTrue(TEXT("Body and choices are hidden"), Widget->BodyText->GetVisibility() == ESlateVisibility::Collapsed
        && Widget->ChoiceButton0->GetVisibility() == ESlateVisibility::Collapsed
        && Widget->ChoiceButton1->GetVisibility() == ESlateVisibility::Collapsed
        && Widget->ChoiceButton2->GetVisibility() == ESlateVisibility::Collapsed);
    TestEqual(TEXT("Selected text is shown"), Widget->PickedText->GetText().ToString(), FString(TEXT("선택 · 불을 끈다")));
    TestTrue(TEXT("Outcome joins both lines"), Widget->OutcomeText->GetText().ToString().Contains(TEXT("빛이 지나갔다."))
        && Widget->OutcomeText->GetText().ToString().Contains(TEXT("소리가 멎었다.")));
    TestEqual(TEXT("Both actual changes have chips"), Widget->ChangeList->GetChildrenCount(), 2);
    bool bRed = false;
    bool bGreen = false;
    for (UWidget* Child : Widget->ChangeList->GetAllChildren())
    {
        const UBorder* Chip = Cast<UBorder>(Child);
        const UTextBlock* Text = Chip ? Cast<UTextBlock>(Chip->GetContent()) : nullptr;
        if (!Text) continue;
        const FLinearColor Color = Text->GetColorAndOpacity().GetSpecifiedColor();
        bRed |= Color.Equals(FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("F08A7A"))));
        bGreen |= Color.Equals(FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("8FE0A0"))));
    }
    TestTrue(TEXT("Loss and gain use specified colors"), bRed && bGreen);
    const int32 APAfterChoice = Run->GetActionPoints();
    Widget->Choose(0);
    TestEqual(TEXT("Repeated choice cannot apply effects again"), Run->GetActionPoints(), APAfterChoice);
    Widget->ConfirmButton->OnClicked.Broadcast();
    Widget->HandleConfirm();
    TestEqual(TEXT("Confirm finishes exactly once"), Listener->FinishedCount, 1);
    TestNull(TEXT("Confirm removes window"), Widget->GetParent());

    Host->AddChildToVerticalBox(Widget);
    Host->TakeWidget();
    Widget->ShowEvent(Director, Run, TEXT("Quiet"));
    TestTrue(TEXT("Reusing window restores choices"), Widget->ChoiceButton0->GetVisibility() == ESlateVisibility::Visible);
    TestTrue(TEXT("Reusing window hides result"), Widget->ResultBox->GetVisibility() == ESlateVisibility::Collapsed);
    Widget->Choose(0);
    TestTrue(TEXT("Empty outcome is hidden"), Widget->OutcomeText->GetVisibility() == ESlateVisibility::Collapsed);
    const UTextBlock* NoChange = Cast<UTextBlock>(Widget->ChangeList->GetChildAt(0));
    TestTrue(TEXT("Empty changes show fallback"), NoChange && NoChange->GetText().ToString() == TEXT("변화 없음"));
    Widget->HandleConfirm();
    TestEqual(TEXT("Reused event has its own confirmation"), Listener->FinishedCount, 2);
    Game->Shutdown();
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSEventAraEffectsTest, "SS.Event.AraEffects",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSEventAraEffectsTest::RunTest(const FString& Parameters)
{
    using namespace SSEventTest;
    UGameInstance* GameInstance = NewObject<UGameInstance>();
    USSRunSubsystem* Run = NewObject<USSRunSubsystem>(GameInstance);
    Run->InitializeShelterStats(100.f, 100.f, 100.f);

    // 동료 한 명 (이름 "하린")
    USSSurvivorDefinition* Researcher = NewObject<USSSurvivorDefinition>(Run);
    Researcher->SurvivorId = TEXT("Researcher");
    Researcher->DisplayName = FText::FromString(TEXT("하린"));
    Researcher->AraInfluence = 1.2f;
    Run->RecruitSurvivor(Researcher);
    Run->RescueFollowingSurvivors();

    // 아라 제안 사건: 보내면 식량 +2 · 교체 · 결과 문장, 거절하면 아라가 다음 단계 예약
    USSEventCatalog* Catalog = NewObject<USSEventCatalog>(Run);
    Catalog->Items = { MakeItem(Catalog, TEXT("Food"), ESSItemUseEffect::RestoreSatiety) };
    Catalog->EventTable = MakeTable(FSSEventRow::StaticStruct());
    FSSEventRow Offer;
    Offer.Title = FText::FromString(TEXT("{Target} 검진"));
    Offer.Body = FText::FromString(TEXT("{Target}의 상태"));
    Offer.Weight = 0;
    Offer.bScheduledOnly = true;
    Catalog->EventTable->AddRow(TEXT("Offer"), Offer);
    Catalog->ChoiceTable = MakeTable(FSSEventChoiceRow::StaticStruct());
    Catalog->ChoiceTable->AddRow(TEXT("Send"), Choice(TEXT("Offer"), 0, ESSEventCondition::AraHasTarget));
    Catalog->ChoiceTable->AddRow(TEXT("Refuse"), Choice(TEXT("Offer"), 1, ESSEventCondition::AraHasTarget));
    Catalog->EffectTable = MakeTable(FSSEventEffectRow::StaticStruct());
    Catalog->EffectTable->AddRow(TEXT("Send_1"), Effect(TEXT("Send"), ESSEventEffect::Item, TEXT("Food"), 2));
    Catalog->EffectTable->AddRow(TEXT("Send_2"), Effect(TEXT("Send"), ESSEventEffect::AraSwapTarget, NAME_None, 0));
    Catalog->EffectTable->AddRow(TEXT("Send_3"), Effect(TEXT("Send"), ESSEventEffect::Journal, NAME_None, 0, TEXT("{Target} 돌아옴")));
    Catalog->EffectTable->AddRow(TEXT("Refuse_1"), Effect(TEXT("Refuse"), ESSEventEffect::AraRefused, NAME_None, 0));

    USSEventDirector* Director = Run->GetEventDirector();
    Director->SetSeed(7);
    if (!TestTrue(TEXT("Ara catalog is valid"), Director->SetCatalog(Catalog))) return false;

    // 표적이 없으면 선택지가 닫혀 있고, {Target}은 "동료"
    TestFalse(TEXT("No target closes the offer"), Director->GetChoices(TEXT("Offer"), *Run)[0].bAvailable);
    TestEqual(TEXT("Unknown target name"), USSEventDirector::FillText(Offer.Title, *Run).ToString(), FString(TEXT("동료 검진")));

    // 표적이 생기면 선택지가 열리고 이름이 채워짐
    USSAraDirector* Ara = Run->GetAra();
    Ara->ObserveNight(TEXT("Researcher"), ESSInvestigationSpot::TerminalLog, true);
    Ara->ObserveEavesdrop(TEXT("Researcher"), true);
    Ara->UpdateTarget();
    if (!TestTrue(TEXT("Researcher targeted"), Ara->HasTarget())) return false;
    TestTrue(TEXT("Target opens the offer"), Director->GetChoices(TEXT("Offer"), *Run)[0].bAvailable);
    TestEqual(TEXT("Target name filled"), USSEventDirector::FillText(Offer.Title, *Run).ToString(), FString(TEXT("하린 검진")));

    // 거절: 아라가 재제안을 예약
    FSSEventResult Result;
    TestTrue(TEXT("Refuse applies"), Director->ApplyChoice(TEXT("Offer"), TEXT("Refuse"), *Run, Result));
    TestEqual(TEXT("Refusal counted"), Ara->GetRefusalCount(), 1);
    TestTrue(TEXT("Repeat offer scheduled"), Director->FindScheduledDay(FName(USSAraDirector::RepeatOfferEventId)) > 0);
    TestEqual(TEXT("Refusal is not a visible change"), Result.Changes.Num(), 0);

    // 보냄: 보상만 보이고 교체는 비밀. 이름은 교체 뒤에도 그대로 채워짐
    TestTrue(TEXT("Send applies"), Director->ApplyChoice(TEXT("Offer"), TEXT("Send"), *Run, Result));
    TestTrue(TEXT("Researcher swapped"), Run->GetCompanions()->IsAndroid(TEXT("Researcher")));
    TestEqual(TEXT("Title keeps the name"), Result.Title.ToString(), FString(TEXT("하린 검진")));
    TestTrue(TEXT("Line keeps the name"), Result.Lines.Num() == 1 && Result.Lines[0].ToString() == TEXT("하린 돌아옴"));
    TestTrue(TEXT("Only the reward is a visible change"), Result.Changes.Num() == 1 && Result.Changes[0].Type == ESSEventEffect::Item);
    TestTrue(TEXT("Swap is never described"), Director->DescribeChange({ ESSEventEffect::AraSwapTarget, NAME_None, 0 }).IsEmpty());
    TestTrue(TEXT("Refusal is never described"), Director->DescribeChange({ ESSEventEffect::AraRefused, NAME_None, 0 }).IsEmpty());

    // 교체가 끝나면 제안 선택지는 다시 닫힘
    TestFalse(TEXT("Offer closed after swap"), Director->GetChoices(TEXT("Offer"), *Run)[0].bAvailable);
    return true;
}
#endif
