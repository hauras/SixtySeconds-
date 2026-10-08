#include "Item/SSRunSubsystem.h"
#include "Item/SSItemDefinition.h"
#include "Exploration/SSExplorationTypes.h"
#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSExplorationSettleTest, "SS.Exploration.Settle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSExplorationSettleTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>();

	// 보관함에 식량 2, 물 1이 있는 새 런
	USSItemDefinition* Food = nullptr;
	const auto MakeRun = [GameInstance, &Food]()
	{
		USSRunSubsystem* Run = NewObject<USSRunSubsystem>(GameInstance);
		Food = NewObject<USSItemDefinition>(Run);
		Food->ItemId = SSItemIds::Food;
		Food->UseEffect = ESSItemUseEffect::RestoreSatiety;
		Food->EffectAmount = 40.f;
		USSItemDefinition* Water = NewObject<USSItemDefinition>(Run);
		Water->ItemId = SSItemIds::Water;
		Water->UseEffect = ESSItemUseEffect::RestoreHydration;
		Water->EffectAmount = 50.f;
		FSSItemStack FoodStack;
		FoodStack.Item = Food;
		FoodStack.Quantity = 2;
		FSSItemStack WaterStack;
		WaterStack.Item = Water;
		WaterStack.Quantity = 1;
		Run->DepositItems({FoodStack, WaterStack});
		return Run;
	};
	const auto MakeResult = [&Food](ESSExplorationOutcome Outcome, int32 FoodCount, float Injury)
	{
		FSSExplorationResult Result;
		Result.Outcome = Outcome;
		FSSItemStack Carried;
		Carried.Item = Food;
		Carried.Quantity = FoodCount;
		Result.Items.Add(Carried);
		Result.Injury = Injury;
		Result.TurnsUsed = 5;
		return Result;
	};
	const auto HasJournal = [](const USSRunSubsystem* Run, int32 Day, ESSJournalEvent Event)
	{
		return Run->GetJournalEntries().ContainsByPredicate([Day, Event](const FSSJournalEntry& Entry)
		{
			return Entry.Day == Day && Entry.Event == Event;
		});
	};

	// 1. 귀환 성공: 하루 경과(출발 전 재고로 배급) → 식량 입고
	{
		USSRunSubsystem* Run = MakeRun();
		TestTrue(TEXT("Rations affordable"), Run->HasRationsFor(true, true));
		TestTrue(TEXT("Settle returned"), Run->ApplyExplorationResult(MakeResult(ESSExplorationOutcome::Returned, 3, 0.f), true, true));
		TestEqual(TEXT("Day advanced once"), Run->GetCurrentDay(), 2);
		TestEqual(TEXT("Food: 2 - 1 ration + 3 brought"), Run->GetStoredQuantityById(SSItemIds::Food), 4);
		TestEqual(TEXT("Water: 1 - 1 ration"), Run->GetStoredQuantityById(SSItemIds::Water), 0);
		TestEqual(TEXT("No injury on return"), Run->GetHealth(), 100.f);
		TestEqual(TEXT("Action points refilled for new day"), Run->GetActionPoints(), USSRunSubsystem::MaxActionPoints);
		TestTrue(TEXT("Exploration logged on departure day"), HasJournal(Run, 1, ESSJournalEvent::Expedition));
	}

	// 2. 발각: 입고 없음, 하루 경과 뒤 부상
	{
		USSRunSubsystem* Run = MakeRun();
		TestTrue(TEXT("Settle caught"), Run->ApplyExplorationResult(MakeResult(ESSExplorationOutcome::Caught, 3, 30.f), true, true));
		TestEqual(TEXT("Day advanced once when caught"), Run->GetCurrentDay(), 2);
		TestEqual(TEXT("No food deposited when caught"), Run->GetStoredQuantityById(SSItemIds::Food), 1);
		TestEqual(TEXT("Injury applied"), Run->GetHealth(), 70.f);
		TestTrue(TEXT("Injury logged"), HasJournal(Run, 2, ESSJournalEvent::DayEnd));
	}

	// 3. 배급 부족: 아무것도 바뀌지 않음
	{
		USSRunSubsystem* Run = MakeRun();
		TestTrue(TEXT("No rations always affordable"), Run->HasRationsFor(false, false));
		TestTrue(TEXT("First settle uses the only water"),
			Run->ApplyExplorationResult(MakeResult(ESSExplorationOutcome::Returned, 1, 0.f), true, true));
		TestFalse(TEXT("Water now short"), Run->HasRationsFor(false, true));
		const int32 DayBefore = Run->GetCurrentDay();
		const int32 FoodBefore = Run->GetStoredQuantityById(SSItemIds::Food);
		TestFalse(TEXT("Settle rejected when rations short"),
			Run->ApplyExplorationResult(MakeResult(ESSExplorationOutcome::Returned, 3, 0.f), false, true));
		TestEqual(TEXT("Day unchanged on rejection"), Run->GetCurrentDay(), DayBefore);
		TestEqual(TEXT("Nothing deposited on rejection"), Run->GetStoredQuantityById(SSItemIds::Food), FoodBefore);
	}

	// 4. 진행 중 결과는 정산 불가
	{
		USSRunSubsystem* Run = MakeRun();
		TestFalse(TEXT("InProgress result rejected"),
			Run->ApplyExplorationResult(MakeResult(ESSExplorationOutcome::InProgress, 3, 0.f), false, false));
		TestEqual(TEXT("Day unchanged for InProgress"), Run->GetCurrentDay(), 1);
	}

	// 5. 부상으로 사망: 기록 남고, 이후 정산 불가
	{
		USSRunSubsystem* Run = MakeRun();
		Run->InitializeShelterStats(20.f, 100.f, 100.f);
		TestTrue(TEXT("Settle fatal time out"), Run->ApplyExplorationResult(MakeResult(ESSExplorationOutcome::TimeOut, 3, 30.f), false, false));
		TestEqual(TEXT("Health reaches zero"), Run->GetHealth(), 0.f);
		TestTrue(TEXT("Death logged"), HasJournal(Run, 2, ESSJournalEvent::Death));
		TestFalse(TEXT("Dead player cannot settle again"),
			Run->ApplyExplorationResult(MakeResult(ESSExplorationOutcome::Returned, 1, 0.f), false, false));
	}

	return true;
}
#endif
