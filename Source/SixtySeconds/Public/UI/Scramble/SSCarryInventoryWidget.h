#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SSCarryInventoryWidget.generated.h"

class USSCarryComponent;
class USSInventorySlotWidget;
class UHorizontalBox;
class UTextBlock;

UCLASS(Abstract)
class SIXTYSECONDS_API USSCarryInventoryWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void InitializeInventory(USSCarryComponent* InCarry);
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UHorizontalBox> SlotContainer;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> CapacityText;
	UPROPERTY(EditDefaultsOnly, Category="SS|UI")
	TSubclassOf<USSInventorySlotWidget> SlotWidgetClass;
private:
	void BindCarry();
	void UnbindCarry();
	UFUNCTION()
	void RefreshSlots();
	UPROPERTY(Transient)
	TObjectPtr<USSCarryComponent> CarryComponent;
	bool bConstructed = false;
};
