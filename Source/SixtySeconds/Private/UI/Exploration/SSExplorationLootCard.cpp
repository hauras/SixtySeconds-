#include "UI/Exploration/SSExplorationLootCard.h"
#include "Item/SSInventoryTypes.h"
#include "Item/SSItemDefinition.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void USSExplorationLootCard::Setup(int32 InLootIndex, const FSSItemStack& Stack)
{
	LootIndex = InLootIndex;

	const USSItemDefinition* Item = Stack.Item;
	if (!IsValid(Item)) return;

	if (ItemIcon)
	{
		ItemIcon->SetBrushFromTexture(Item->Icon, false);
		// 아이콘이 없는 아이템은 빈 흰 사각형이 뜨지 않게 숨김
		ItemIcon->SetVisibility(Item->Icon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
	if (NameText) NameText->SetText(Item->DisplayName);
	if (QuantityText) QuantityText->SetText(FText::Format(NSLOCTEXT("SSExploration", "LootQuantity", "×{0}"), Stack.Quantity));
	if (CostText) CostText->SetText(FText::Format(NSLOCTEXT("SSExploration", "LootCost", "{0}칸"), Item->CarryCost));
}

void USSExplorationLootCard::SetAvailable(bool bCanTake)
{
	SetRenderOpacity(bCanTake ? 1.f : 0.4f);   // 비활성 대신 어둡게: 클릭은 받아도 세션이 거절함
}

void USSExplorationLootCard::NativeConstruct()
{
	Super::NativeConstruct();
	if (CardButton) CardButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleClicked);
}

void USSExplorationLootCard::NativeDestruct()
{
	if (CardButton) CardButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleClicked);
	Super::NativeDestruct();
}

void USSExplorationLootCard::HandleClicked()
{
	if (LootIndex == INDEX_NONE) return;   // Setup 전 클릭은 무시
	OnCardClicked.Broadcast(LootIndex);
}
