#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Item/SSInventoryTypes.h"
#include "SSInventorySlotWidget.generated.h"

class UImage;
class UTextBlock;

UCLASS(Abstract)
class SIXTYSECONDS_API USSInventorySlotWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void SetItemStack(const FSSItemStack& InStack);
protected:
	virtual void NativeConstruct() override;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UImage> ItemIcon;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> QuantityText;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> ItemNameText;
private:
	void RefreshDisplay();
	UPROPERTY(Transient)
	FSSItemStack ItemStack;
};
