#include "Item/SSRunSubsystem.h"
#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSItemUseTest, "SS.Items.UseAndRations",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSItemUseTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	USSRunSubsystem* Run = NewObject<USSRunSubsystem>(GameInstance);
	const auto AddItem = [Run](const TCHAR* Id, ESSItemUseEffect Effect, float Amount)
	{
		USSItemDefinition* Item = NewObject<USSItemDefinition>(Run);
		Item->ItemId = Id;
		Item->UseEffect = Effect;
		Item->EffectAmount = Amount;
		FSSItemStack Stack;
		Stack.Item = Item;
		Stack.Quantity = 1;
		Run->DepositItems({Stack});
		return Item;
	};
	AddItem(TEXT("Medkit"), ESSItemUseEffect::RestoreHealth, 30.f);
	TestFalse(TEXT("Full health rejects medkit"), Run->UseItem(TEXT("Medkit")));
	TestEqual(TEXT("Rejected medkit retained"), Run->GetStoredQuantityById(TEXT("Medkit")), 1);
	Run->InitializeShelterStats(85.f, 30.f, 20.f);
	TestTrue(TEXT("Medkit works"), Run->UseItem(TEXT("Medkit")));
	TestEqual(TEXT("Recovery capped"), Run->GetHealth(), 100.f);
	TestEqual(TEXT("Last medkit removed"), Run->GetStoredQuantityById(TEXT("Medkit")), 0);
	TestEqual(TEXT("Use does not advance day"), Run->GetCurrentDay(), 1);
	AddItem(TEXT("Food"), ESSItemUseEffect::RestoreSatiety, 40.f);
	TestFalse(TEXT("Missing water rejects whole ration request"), Run->AdvanceDay(true, true));
	TestEqual(TEXT("Food not partially spent"), Run->GetStoredQuantityById(TEXT("Food")), 1);
	TestEqual(TEXT("Failed day unchanged"), Run->GetCurrentDay(), 1);
	USSItemDefinition* Water = AddItem(TEXT("Water"), ESSItemUseEffect::RestoreHydration, 50.f);
	Water->EffectAmount = -1.f;
	TestFalse(TEXT("Invalid water settings reject whole request"), Run->AdvanceDay(true, true));
	TestEqual(TEXT("Food retained for invalid water"), Run->GetStoredQuantityById(TEXT("Food")), 1);
	Water->EffectAmount = 50.f;
	TestTrue(TEXT("Rations advance day"), Run->AdvanceDay(true, true));
	TestEqual(TEXT("Food recovery then daily loss"), Run->GetSatiety(), 50.f);
	TestEqual(TEXT("Water recovery then daily loss"), Run->GetHydration(), 45.f);
	TestEqual(TEXT("Water spent"), Run->GetStoredQuantityById(TEXT("Water")), 0);
	USSItemDefinition* Reusable = AddItem(TEXT("Reusable"), ESSItemUseEffect::RestoreHealth, 10.f);
	Reusable->bConsumeOnUse = false;
	Run->InitializeShelterStats(50.f, 50.f, 50.f);
	TestTrue(TEXT("Reusable effect applied"), Run->UseItem(TEXT("Reusable")));
	TestEqual(TEXT("Reusable retained"), Run->GetStoredQuantityById(TEXT("Reusable")), 1);
	AddItem(TEXT("Battery"), ESSItemUseEffect::None, 0.f);
	TestFalse(TEXT("Resource has no direct effect"), Run->UseItem(TEXT("Battery")));
	Run->InitializeShelterStats(0.f, 0.f, 0.f);
	TestFalse(TEXT("Items cannot revive dead player"), Run->UseItem(TEXT("Reusable")));
	return true;
}
#endif
