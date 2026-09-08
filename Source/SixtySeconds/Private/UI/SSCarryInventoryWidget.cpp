#include "UI/SSCarryInventoryWidget.h"
#include "UI/SSInventorySlotWidget.h"
#include "Item/SSCarryComponent.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"

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
		CapacityText->SetText(FText::Format(NSLOCTEXT("SS", "CarryCapacity", "{0} / {1}"),
			FText::AsNumber(bValid ? CarryComponent->GetUsedSlots() : 0),
			FText::AsNumber(bValid ? CarryComponent->GetCapacity() : 0)));
	}
	if (!SlotContainer) return;
	SlotContainer->ClearChildren();
	if (!bValid || !SlotWidgetClass) return;
	for (const FSSItemStack& Stack : CarryComponent->GetItems())
	{
		if (!IsValid(Stack.Item) || Stack.Quantity <= 0) continue;
		USSInventorySlotWidget* ItemWidget = CreateWidget<USSInventorySlotWidget>(this, SlotWidgetClass);
		if (!ItemWidget) continue;
		ItemWidget->SetItemStack(Stack);
		SlotContainer->AddChildToHorizontalBox(ItemWidget);
	}
}
