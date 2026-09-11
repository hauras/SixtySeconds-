#include "Item/SSItemDefinition.h"

USSItemDefinition::USSItemDefinition() = default;

void USSItemDefinition::PostLoad()
{
	Super::PostLoad();
	if (bUseSettingsInitialized) return;
	bUseSettingsInitialized = true;
	// Preserve any explicit settings on newly authored assets.
	if (UseEffect != ESSItemUseEffect::None || EffectAmount != 0.f) return;
	if (ItemId == TEXT("Water"))
	{
		UseEffect = ESSItemUseEffect::RestoreHydration;
		EffectAmount = 50.f;
	}
	else if (ItemId == TEXT("Food"))
	{
		UseEffect = ESSItemUseEffect::RestoreSatiety;
		EffectAmount = 40.f;
	}
	else if (ItemId == TEXT("Medkit"))
	{
		UseEffect = ESSItemUseEffect::RestoreHealth;
		EffectAmount = 30.f;
	}
	else if (ItemId == TEXT("Battery"))
	{
		ItemType = ESSItemType::ExplorationResource;
	}
	else if (ItemId == TEXT("RepairKit"))
	{
		ItemType = ESSItemType::Tool;
		bConsumeOnUse = false;
	}
}
