#include "Item/SSCarryComponent.h"

USSCarryComponent::USSCarryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

int32 USSCarryComponent::GetUsedSlots() const
{
	int64 UsedSlots = 0;
	for (const FSSItemStack& Stack : Items)
	{
		// 비정상 데이터가 공간을 공짜로 만들지 않도록 추가 획득을 막는다.
		if (!IsValid(Stack.Item) || Stack.Quantity <= 0 || Stack.Item->CarryCost <= 0)
		{
			return MAX_int32;
		}
		UsedSlots += static_cast<int64>(Stack.Item->CarryCost) * Stack.Quantity;
		if (UsedSlots >= MAX_int32)
		{
			return MAX_int32;
		}
	}
	return static_cast<int32>(UsedSlots);
}

int32 USSCarryComponent::GetRemainingSlots() const
{
	return FMath::Max(0, GetCapacity() - GetUsedSlots());
}

int32 USSCarryComponent::GetQuantity(USSItemDefinition* Item) const
{
	if (!IsValid(Item)) return 0;
	for (const FSSItemStack& Stack : Items)
	{
		if (Stack.Item == Item) return Stack.Quantity;
	}
	return 0;
}

bool USSCarryComponent::CanAddItem(USSItemDefinition* Item, int32 Quantity) const
{
	if (!IsValid(Item) || Quantity <= 0 || Item->CarryCost <= 0) return false;
	const int64 RequiredSlots = static_cast<int64>(Item->CarryCost) * Quantity;
	return RequiredSlots <= GetRemainingSlots()
		&& static_cast<int64>(GetQuantity(Item)) + Quantity <= MAX_int32;
}

bool USSCarryComponent::TryAddItem(USSItemDefinition* Item, int32 Quantity)
{
	if (!CanAddItem(Item, Quantity)) return false;
	FSSItemStack* Existing = Items.FindByPredicate([Item](const FSSItemStack& Stack)
	{
		return Stack.Item == Item;
	});
	if (Existing)
	{
		Existing->Quantity += Quantity;
	}
	else
	{
		FSSItemStack& Stack = Items.AddDefaulted_GetRef();
		Stack.Item = Item;
		Stack.Quantity = Quantity;
	}
	OnCarryChanged.Broadcast();
	return true;
}

bool USSCarryComponent::TryRemoveItem(USSItemDefinition* Item, int32 Quantity)
{
	if (!IsValid(Item) || Quantity <= 0) return false;
	const int32 Index = Items.IndexOfByPredicate([Item](const FSSItemStack& Stack)
	{
		return Stack.Item == Item;
	});
	if (Index == INDEX_NONE || Items[Index].Quantity < Quantity) return false;
	Items[Index].Quantity -= Quantity;
	if (Items[Index].Quantity == 0) Items.RemoveAt(Index);
	OnCarryChanged.Broadcast();
	return true;
}

void USSCarryComponent::ClearItems()
{
	if (Items.IsEmpty()) return;
	Items.Reset();
	OnCarryChanged.Broadcast();
}
