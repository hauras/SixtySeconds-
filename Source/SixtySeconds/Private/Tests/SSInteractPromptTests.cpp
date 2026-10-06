#include "Character/SSSurvivorDefinition.h"
#include "Character/SSSurvivorPickup.h"
#include "Item/SSCarryComponent.h"
#include "Item/SSInteractable.h"
#include "Item/SSItemDefinition.h"
#include "Item/SSPickupActor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSInteractPromptTest, "SS.Scramble.InteractPrompt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSInteractPromptTest::RunTest(const FString& Parameters)
{
	// 액터를 놓을 임시 월드
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	Context.SetCurrentWorld(World);

	// 식량 (가방 1칸)
	USSItemDefinition* Food = NewObject<USSItemDefinition>();
	Food->ItemId = TEXT("Food");
	Food->DisplayName = FText::FromString(TEXT("식량"));
	Food->CarryCost = 1;

	ASSPickupActor* Pickup = World->SpawnActor<ASSPickupActor>();
	FSSItemStack Stack;
	Stack.Item = Food;
	Stack.Quantity = 1;
	Pickup->SetItemStack(Stack);

	// 상호작용 대상으로 인식됨
	TestTrue(TEXT("Pickup is interactable"), Pickup->Implements<USSInteractable>());

	// 가방에 자리가 있으면 "식량 · [E] 줍기"
	USSCarryComponent* Carry = NewObject<USSCarryComponent>();
	TestTrue(TEXT("Empty bag can pick up"), Pickup->CanPickupWith(Carry));
	TestEqual(TEXT("Pickup prompt"), Pickup->GetPickupPrompt(Carry).ToString(), FString(TEXT("식량 · [E] 줍기")));

	// 두 개면 개수도
	Stack.Quantity = 2;
	Pickup->SetItemStack(Stack);
	TestTrue(TEXT("Count shown"), Pickup->GetPickupPrompt(Carry).ToString().StartsWith(TEXT("식량 ×2")));

	// 가방이 꽉 차면 못 줍고, 이유를 보여줌
	TestTrue(TEXT("Fill the bag"), Carry->TryAddItem(Food, Carry->GetCapacity()));
	TestFalse(TEXT("Full bag cannot pick up"), Pickup->CanPickupWith(Carry));
	TestTrue(TEXT("Full bag prompt"), Pickup->GetPickupPrompt(Carry).ToString().Contains(TEXT("가방이 가득 찼다")));

	// 가방이 없으면(사람이 아니면) 상호작용 불가
	TestFalse(TEXT("No pawn, no pickup"), Pickup->CanInteract(nullptr));

	// 동료: "서하린 · [E] 데려가기"
	USSSurvivorDefinition* Harin = NewObject<USSSurvivorDefinition>();
	Harin->SurvivorId = TEXT("TestResearcher");
	Harin->DisplayName = FText::FromString(TEXT("서하린"));
	ASSSurvivorPickup* Survivor = World->SpawnActor<ASSSurvivorPickup>();
	Survivor->Definition = Harin;
	TestTrue(TEXT("Survivor is interactable"), Survivor->Implements<USSInteractable>());
	TestEqual(TEXT("Survivor prompt"), Survivor->GetInteractPrompt(nullptr).ToString(), FString(TEXT("서하린 · [E] 데려가기")));
	TestFalse(TEXT("No pawn, no rescue"), Survivor->CanInteract(nullptr));

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}
#endif
