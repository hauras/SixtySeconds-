#include "UI/SSStockItemWidget.h"
#include "Components/Image.h"
#include "Engine/GameInstance.h"
#include "Item/SSItemDefinition.h"
#include "Item/SSRunSubsystem.h"

void USSStockItemWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	if (IsDesignTime()) SetItemAndQuantity(ItemDefinition, PreviewQuantity);
}

void USSStockItemWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (UGameInstance* Instance = GetGameInstance())
		RunSubsystem = Instance->GetSubsystem<USSRunSubsystem>();
	if (IsValid(RunSubsystem))
		RunSubsystem->OnStoredItemsChanged.AddUniqueDynamic(this, &USSStockItemWidget::RefreshFromStorage);
	RefreshFromStorage();
}

void USSStockItemWidget::NativeDestruct()
{
	if (IsValid(RunSubsystem))
		RunSubsystem->OnStoredItemsChanged.RemoveDynamic(this, &USSStockItemWidget::RefreshFromStorage);
	RunSubsystem = nullptr;
	Super::NativeDestruct();
}

void USSStockItemWidget::RefreshFromStorage()
{
	const int32 Quantity = IsValid(RunSubsystem) && IsValid(ItemDefinition)
		? RunSubsystem->GetStoredQuantityById(ItemDefinition->ItemId) : 0;
	SetItemAndQuantity(ItemDefinition, Quantity);
}

void USSStockItemWidget::SetItemAndQuantity(USSItemDefinition* Item, int32 Quantity)
{
	ItemDefinition = Item;
	if (!StockImage) return;
	UTexture2D* Texture = IsValid(Item) ? Item->GetStockTexture(Quantity) : nullptr;
	StockImage->SetBrushFromTexture(Texture, false);
	StockImage->SetVisibility(Texture ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
}
