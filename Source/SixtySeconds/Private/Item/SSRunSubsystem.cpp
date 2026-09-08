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
}
