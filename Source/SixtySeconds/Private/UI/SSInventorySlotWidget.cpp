#include "UI/SSInventorySlotWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

void USSInventorySlotWidget::SetItemStack(const FSSItemStack& InStack)
{
	ItemStack = InStack;
	RefreshDisplay();
}

void USSInventorySlotWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshDisplay();
}

void USSInventorySlotWidget::RefreshDisplay()
{
	const bool bValid = IsValid(ItemStack.Item) && ItemStack.Quantity > 0;
	if (ItemNameText) ItemNameText->SetText(bValid ? ItemStack.Item->DisplayName : FText::GetEmpty());
	if (QuantityText) QuantityText->SetText(bValid ? FText::Format(NSLOCTEXT("SS", "SlotQty", "x{0}"), ItemStack.Quantity) : FText::GetEmpty());
	if (ItemIcon)
	{
		UTexture2D* Icon = bValid ? ItemStack.Item->Icon.Get() : nullptr;
		ItemIcon->SetBrushFromTexture(Icon);
		ItemIcon->SetVisibility(Icon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
}
