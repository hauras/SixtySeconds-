#include "Item/SSRunSubsystem.h"
#include "Item/SSItemDefinition.h"
#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSAraBriefingTest, "SS.Ara.Briefing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSAraQuestionsTest, "SS.Ara.Questions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSAraQuestionsTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	USSRunSubsystem* Run = NewObject<USSRunSubsystem>(GameInstance);
	Run->InitializeShelterStats(100.f, 100.f, 100.f);
	FText Answer;
	TestTrue(TEXT("First question succeeds"), Run->AskAraQuestion(0, Answer));
	TestFalse(TEXT("Answer is not empty"), Answer.IsEmpty());
	TestEqual(TEXT("Question costs one action point"), Run->GetActionPoints(), 4);
	TestTrue(TEXT("Question is marked asked"), Run->HasAskedAraQuestionToday(0));
	TestTrue(TEXT("Repeated question returns answer"), Run->AskAraQuestion(0, Answer));
	TestEqual(TEXT("Repeated question is free"), Run->GetActionPoints(), 4);
	TestFalse(TEXT("Invalid question is rejected"), Run->AskAraQuestion(3, Answer));
	TestTrue(TEXT("Day advances"), Run->AdvanceDay(false, false));
	TestFalse(TEXT("Question resets next day"), Run->HasAskedAraQuestionToday(0));
	TestEqual(TEXT("Action points reset next day"), Run->GetActionPoints(), 5);
	return true;
}

bool FSSAraBriefingTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	USSRunSubsystem* Run = NewObject<USSRunSubsystem>(GameInstance);

	USSItemDefinition* Food = NewObject<USSItemDefinition>(Run);
	Food->ItemId = SSItemIds::Food;
	USSItemDefinition* Water = NewObject<USSItemDefinition>(Run);
	Water->ItemId = SSItemIds::Water;

	// 은신처 첫날: 보고가 비어 있지 않고, 아직 안 읽음
	Run->InitializeShelterStats(100.f, 100.f, 100.f);
	TestTrue(TEXT("Day 1 briefing is built on shelter start"), Run->GetAraBriefing().ToString().StartsWith(TEXT("Day 1.")));
	TestTrue(TEXT("Day 1 briefing starts unread"), Run->HasUnreadAraBriefing());

	Run->MarkAraBriefingRead();
	TestFalse(TEXT("Reading clears unread"), Run->HasUnreadAraBriefing());

	// 다음 날: 아침 물자 수량으로 새 보고, 다시 안 읽음
	FSSItemStack FoodStack;
	FoodStack.Item = Food;
	FoodStack.Quantity = 3;
	FSSItemStack WaterStack;
	WaterStack.Item = Water;
	WaterStack.Quantity = 2;
	Run->DepositItems({FoodStack, WaterStack});
	TestTrue(TEXT("Day advances without rations"), Run->AdvanceDay(false, false));

	const FString Morning = Run->GetAraBriefing().ToString();
	TestTrue(TEXT("Day 2 briefing has the day"), Morning.StartsWith(TEXT("Day 2.")));
	TestTrue(TEXT("Day 2 briefing reports food"), Morning.Contains(TEXT("식량 3개")));
	TestTrue(TEXT("Day 2 briefing reports water"), Morning.Contains(TEXT("물 2개")));
	TestTrue(TEXT("New day is unread again"), Run->HasUnreadAraBriefing());

	// 아침 이후 물자가 바뀌어도 보고 문장은 그대로
	TestTrue(TEXT("Food consumed after morning"), Run->RemoveStoredItemsById(SSItemIds::Food, 1));
	TestEqual(TEXT("Briefing stays fixed after morning"), Run->GetAraBriefing().ToString(), Morning);

	// 밤 사건으로 물자가 줄고 밤이 끝나면(HUD가 다시 만듦) 아침 보고에 반영
	TestTrue(TEXT("Night event takes water"), Run->RemoveStoredItemsById(SSItemIds::Water, 1));
	Run->BuildAraBriefing();
	TestTrue(TEXT("Rebuilt briefing reflects the night"), Run->GetAraBriefing().ToString().Contains(TEXT("물 1개")));

	// 새 판: Day 1 보고로 돌아가고 안 읽음
	Run->MarkAraBriefingRead();
	Run->ResetRun();
	TestTrue(TEXT("Reset rebuilds Day 1 briefing"), Run->GetAraBriefing().ToString().StartsWith(TEXT("Day 1.")));
	TestTrue(TEXT("Reset makes briefing unread"), Run->HasUnreadAraBriefing());

	return true;
}
#endif
