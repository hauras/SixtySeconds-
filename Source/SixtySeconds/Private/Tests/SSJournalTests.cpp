#include "Item/SSRunSubsystem.h"
#include "Item/SSExpeditionDefinition.h"
#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSJournalTest, "SS.Journal.ChronologyAndReset",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSJournalTest::RunTest(const FString& Parameters)
{
    UGameInstance* TestInstance = NewObject<UGameInstance>();
    USSRunSubsystem* Run = NewObject<USSRunSubsystem>(TestInstance);
    Run->ResetRun();
    TestTrue(TEXT("Fresh run has no old records"), Run->GetJournalEntries().IsEmpty());
    TestFalse(TEXT("Missing ration rejects advance"), Run->AdvanceDay(true, false));
    TestTrue(TEXT("Rejected day produces no records"), Run->GetJournalEntries().IsEmpty());
    USSItemDefinition* Food = NewObject<USSItemDefinition>();
    Food->ItemId = TEXT("Food"); Food->DisplayName = FText::FromString(TEXT("Food"));
    Food->UseEffect = ESSItemUseEffect::RestoreSatiety; Food->EffectAmount = 40;
    USSItemDefinition* Kit = NewObject<USSItemDefinition>();
    Kit->ItemId = TEXT("RepairKit"); Kit->DisplayName = FText::FromString(TEXT("Kit"));
    FSSItemStack FoodStack; FoodStack.Item = Food; FoodStack.Quantity = 1;
    FSSItemStack KitStack; KitStack.Item = Kit; KitStack.Quantity = 1;
    Run->DepositItems({FoodStack, KitStack});
    const FText OriginalDeposit = Run->GetJournalEntries()[0].Message;
    Food->DisplayName = FText::FromString(TEXT("Renamed"));
    TestEqual(TEXT("Old text is a snapshot"), Run->GetJournalEntries()[0].Message.ToString(), OriginalDeposit.ToString());

    USSExpeditionDefinition* Expedition = NewObject<USSExpeditionDefinition>();
    Expedition->RegionName = FText::FromString(TEXT("Depot"));
    Expedition->DurationDays = 1; Expedition->SuccessRate = 1; Expedition->BreakdownChance = 1;
    FSSItemStackRange Reward; Reward.Item = Food; Reward.MinQuantity = 2; Reward.MaxQuantity = 2;
    Expedition->Rewards.Add(Reward);
    TestTrue(TEXT("Dispatch succeeds"), Run->StartExpedition(Expedition) == ESSExpeditionStartResult::Success);
    const int32 AfterDispatch = Run->GetJournalEntries().Num();
    TestTrue(TEXT("Repeated dispatch rejected"), Run->StartExpedition(Expedition) == ESSExpeditionStartResult::RobotBusy);
    TestEqual(TEXT("Rejected dispatch not logged"), Run->GetJournalEntries().Num(), AfterDispatch);
    TestTrue(TEXT("Ration and return"), Run->AdvanceDay(true, false));
    TestEqual(TEXT("Current day advances"), Run->GetCurrentDay(), 2);
    TestEqual(TEXT("Rewards really deposited"), Run->GetStoredQuantityById(TEXT("Food")), 2);
    int32 DayOneRations = 0, DayOneEnd = 0, DayTwoRobot = 0, DayTwoExpedition = 0;
    for (const FSSJournalEntry& Entry : Run->GetJournalEntries())
    {
        if (Entry.Day == 1 && Entry.Event == ESSJournalEvent::Rations) ++DayOneRations;
        if (Entry.Day == 1 && Entry.Event == ESSJournalEvent::DayEnd) ++DayOneEnd;
        if (Entry.Day == 2 && Entry.Event == ESSJournalEvent::Robot) ++DayTwoRobot;
        if (Entry.Day == 2 && Entry.Event == ESSJournalEvent::Expedition) ++DayTwoExpedition;
    }
    TestEqual(TEXT("Rations belong to finished day"), DayOneRations, 1);
    TestEqual(TEXT("Stats belong to finished day"), DayOneEnd, 1);
    TestEqual(TEXT("Breakdown belongs to return day"), DayTwoRobot, 1);
    TestEqual(TEXT("Return and actual reward recorded"), DayTwoExpedition, 2);
    TestTrue(TEXT("Repair succeeds"), Run->RepairRobot());
    TestEqual(TEXT("Repair starts on day 2"), Run->GetJournalEntries().Last().Day, 2);
    TestTrue(TEXT("Repair day advances"), Run->AdvanceDay(false, false));
    TestTrue(TEXT("Repair completes"), Run->GetRobotState() == ESSRobotState::Idle);
    TestEqual(TEXT("Completion on day 3"), Run->GetJournalEntries().Last().Day, 3);

    Expedition->SuccessRate = 0; Expedition->BreakdownChance = 0;
    TestTrue(TEXT("Second dispatch"), Run->StartExpedition(Expedition) == ESSExpeditionStartResult::Success);
    Run->AdvanceDay(false, false);
    TestTrue(TEXT("Failed return replaces old successful result"), Run->GetLastExpeditionResult().ReceivedItems.IsEmpty());
    TestEqual(TEXT("Failed return date"), Run->GetLastExpeditionResult().ReturnDay, 4);
    Run->InitializeShelterStats(5, 0, 0);
    Run->AdvanceDay(false, false);
    TestTrue(TEXT("Death recorded"), Run->GetJournalEntries().Last().Event == ESSJournalEvent::Death);
    const int32 DeathCount = Run->GetJournalEntries().Num();
    TestFalse(TEXT("Dead day cannot repeat"), Run->AdvanceDay(false, false));
    TestEqual(TEXT("No duplicate death"), Run->GetJournalEntries().Num(), DeathCount);
    Run->ResetRun();
    TestTrue(TEXT("Reset clears all history"), Run->GetJournalEntries().IsEmpty());
    return true;
}
#endif
