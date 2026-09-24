#include "Item/SSItemDefinition.h"
#include "Engine/Texture2D.h"

USSItemDefinition::USSItemDefinition() = default;

UTexture2D* USSItemDefinition::GetStockTexture(int32 Quantity) const
{
	UTexture2D* Selected = nullptr;
	int32 BestThreshold = 0;
	for (const FSSStockVisual& Visual : ShelterVisuals)
	{
		if (Visual.MinQuantity > BestThreshold && Visual.MinQuantity <= Quantity
			&& IsValid(Visual.Texture))
		{
			BestThreshold = Visual.MinQuantity;
			Selected = Visual.Texture.Get();
		}
	}
	return Selected;
}

void USSItemDefinition::PostLoad()
{
	Super::PostLoad();
	if (bUseSettingsInitialized) return;
	bUseSettingsInitialized = true;
	// Preserve any explicit settings on newly authored assets.
	if (UseEffect != ESSItemUseEffect::None || EffectAmount != 0.f) return;
	if (ItemId == SSItemIds::Water)
	{
		UseEffect = ESSItemUseEffect::RestoreHydration;
		EffectAmount = 50.f;
	}
	else if (ItemId == SSItemIds::Food)
	{
		UseEffect = ESSItemUseEffect::RestoreSatiety;
		EffectAmount = 40.f;
	}
	else if (ItemId == SSItemIds::Medkit)
	{
		UseEffect = ESSItemUseEffect::RestoreHealth;
		EffectAmount = 30.f;
	}
	else if (ItemId == SSItemIds::Battery)
	{
		ItemType = ESSItemType::ExplorationResource;
	}
	else if (ItemId == SSItemIds::RepairKit)
	{
		ItemType = ESSItemType::Tool;
		bConsumeOnUse = false;
	}
}
