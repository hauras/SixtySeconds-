#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SSStockItemWidget.generated.h"

class UImage;
class USSItemDefinition;
class USSRunSubsystem;

// Place one instance per item on the shelter shelf. No Tick or Blueprint graph needed.
UCLASS(Abstract)
class SIXTYSECONDS_API USSStockItemWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetItemAndQuantity(USSItemDefinition* Item, int32 Quantity);

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UImage> StockImage;

	// Override on each instance placed inside the shelter HUD.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Stock")
	TObjectPtr<USSItemDefinition> ItemDefinition;

	UPROPERTY(EditAnywhere, Category="SS|Stock", meta=(ClampMin="0"))
	int32 PreviewQuantity = 1;

private:
	UFUNCTION()
	void RefreshFromStorage();

	UPROPERTY(Transient)
	TObjectPtr<USSRunSubsystem> RunSubsystem;
};
