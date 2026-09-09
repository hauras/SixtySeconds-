#include "Item/SSRunSubsystem.h"
#include "Item/SSItemDefinition.h"

void USSRunSubsystem::DepositItems(const TArray<FSSItemStack>& CarriedItems)
{
	for (const FSSItemStack& Incoming : CarriedItems)
	{
		if (!IsValid(Incoming.Item) || Incoming.Quantity <= 0) continue;

		FSSItemStack* Existing = StoredItems.FindByPredicate([&Incoming](const FSSItemStack& S)
		{
			return S.Item == Incoming.Item;
		});

		if (Existing)
			Existing->Quantity += Incoming.Quantity;
		else
			StoredItems.Add(Incoming);
	}
}

int32 USSRunSubsystem::GetStoredQuantity(USSItemDefinition* Item) const
{
	if (!IsValid(Item)) return 0;
	for (const FSSItemStack& Stack : StoredItems)
	{
		if (Stack.Item == Item) return Stack.Quantity;
	}
	return 0;
}

void USSRunSubsystem::ResetRun()
{
	StoredItems.Reset();

	CurrentDay = 1;
	Health = 100.f;
	Satiety = 100.f;
	Hydration = 100.f;
	
}

void USSRunSubsystem::InitializeShelterStats(float InHealth, float InSatiety, float InHydration)
{
	Health = FMath::Clamp(InHealth, 0.f, 100.f);
	Satiety = FMath::Clamp(InSatiety, 0.f, 100.f);
	Hydration = FMath::Clamp(InHydration, 0.f, 100.f);
}

bool USSRunSubsystem::ConsumeItem(FName ItemID)
{
	if (ItemID.IsNone()) return false;

	for (int32 Index = 0; Index < StoredItems.Num(); ++Index)
	{
		FSSItemStack& Stack  = StoredItems[Index];
		if (!IsValid(Stack.Item) || Stack.Item->ItemId != ItemID || Stack.Quantity <= 0)
		{
			continue;
		}

		--Stack.Quantity;

		if (Stack.Quantity == 0)
		{
			StoredItems.RemoveAt(Index);
		}
		return true;
	}

	return false;
}


bool USSRunSubsystem::AdvanceDay(bool bGiveFood, bool bGiveWater)
{
	// 죽으면 종료
	if (Health <= 0.f)
	{
		return false;
	}

	static const FName FoodItemId(TEXT("Food"));
	static const FName WaterItemId(TEXT("Water"));

	if (bGiveFood && GetStoredQuantityById(FoodItemId) < 1)
	{
		return false;
	}

	if (bGiveWater && GetStoredQuantityById(WaterItemId) < 1)
	{
		return false;
	}

	if (bGiveFood)
	{
		ConsumeItem(FoodItemId);
		Satiety = FMath::Clamp(Satiety + 40.f, 0.f, 100.f);
	}

	if (bGiveWater)
	{
		ConsumeItem(WaterItemId);
		Hydration = FMath::Clamp(Hydration + 50.f, 0.f, 100.f);
	}

	Satiety = FMath::Clamp(Satiety - 20.f, 0.f, 100.f);
	Hydration = FMath::Clamp(Hydration - 25.f, 0.f, 100.f);

	// 3. 고갈 피해: 둘 다 0이면 총 30 피해
	float Damage = 0.f;

	if (Satiety <= 0.f)
	{
		Damage += 10.f;
	}

	if (Hydration <= 0.f)
	{
		Damage += 20.f;
	}

	Health = FMath::Clamp(Health - Damage, 0.f, 100.f);

	// 4. 날짜 증가
	++CurrentDay;

	return true;
}

int32 USSRunSubsystem::GetStoredQuantityById(FName ItemId) const
{
	if (ItemId.IsNone()) return 0;

	int32 TotalQuantity = 0;

	for (const FSSItemStack& Stack : StoredItems)
	{
		if (!IsValid(Stack.Item) || Stack.Item->ItemId != ItemId || Stack.Quantity <= 0)
		{
			continue;
		}

		TotalQuantity += Stack.Quantity;
	}
	return TotalQuantity;
}

