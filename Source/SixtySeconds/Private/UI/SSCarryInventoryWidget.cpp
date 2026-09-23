#include "UI/SSCarryInventoryWidget.h"
#include "UI/SSInventorySlotWidget.h"
#include "Item/SSCarryComponent.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"

void USSCarryInventoryWidget::InitializeInventory(USSCarryComponent* InCarry)
{
	UnbindCarry();
	CarryComponent = InCarry;
	if (bConstructed) BindCarry();
	RefreshSlots();
}

void USSCarryInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	bConstructed = true;
	BindCarry();
	RefreshSlots();
}

void USSCarryInventoryWidget::NativeDestruct()
{
	UnbindCarry();
	bConstructed = false;
	Super::NativeDestruct();
}

void USSCarryInventoryWidget::BindCarry()
{
	if (IsValid(CarryComponent))
		CarryComponent->OnCarryChanged.AddUniqueDynamic(this, &USSCarryInventoryWidget::RefreshSlots);
}

void USSCarryInventoryWidget::UnbindCarry()
{
	if (IsValid(CarryComponent))
		CarryComponent->OnCarryChanged.RemoveDynamic(this, &USSCarryInventoryWidget::RefreshSlots);
}

void USSCarryInventoryWidget::RefreshSlots()
{
	const bool bValid = IsValid(CarryComponent);
	if (CapacityText)
	{
		CapacityText->SetText(FText::Format(NSLOCTEXT("SS", "CarryCapacityLabel", "운반 중 {0} / {1}"),
			FText::AsNumber(bValid ? CarryComponent->GetUsedSlots() : 0),
			FText::AsNumber(bValid ? CarryComponent->GetCapacity() : 0)));
	}
	if (!SlotContainer) return;
	SlotContainer->ClearChildren();
	if (!bValid || !SlotWidgetClass) return;
	const auto AddSlot = [this](const FSSItemStack& Stack, int32 Cost)
	{
		USSInventorySlotWidget* ItemWidget = CreateWidget<USSInventorySlotWidget>(this, SlotWidgetClass);
		if (!ItemWidget) return;
		ItemWidget->SetItemStack(Stack);
		USizeBox* Cell = NewObject<USizeBox>(this);
		Cell->SetWidthOverride(108.f * Cost + 8.f * (Cost - 1));
		Cell->SetHeightOverride(112.f);
		Cell->SetContent(ItemWidget);
		SlotContainer->AddChildToHorizontalBox(Cell)->SetPadding(FMargin(4, 0));
	};
	// 한 물품당 한 카드. 공간을 여러 칸 쓰는 물품은 카드 폭도 늘린다.
	for (const FSSItemStack& Stack : CarryComponent->GetItems())
	{
		if (!IsValid(Stack.Item) || Stack.Quantity <= 0 || Stack.Item->CarryCost <= 0) continue;
		FSSItemStack SingleItem = Stack;
		SingleItem.Quantity = 1;
		for (int32 ItemIndex = 0; ItemIndex < Stack.Quantity; ++ItemIndex)
			AddSlot(SingleItem, Stack.Item->CarryCost);
	}
	for (int32 EmptyIndex = 0; EmptyIndex < CarryComponent->GetRemainingSlots(); ++EmptyIndex)
		AddSlot(FSSItemStack{}, 1);
}
