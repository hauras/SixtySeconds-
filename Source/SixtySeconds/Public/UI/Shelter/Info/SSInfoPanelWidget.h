
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SSInfoPanelWidget.generated.h"

class UTexture2D;
class UImage;
class UTextBlock;
class UButton;
class UNamedSlot;
/**
 * 
 */
UCLASS()
class SIXTYSECONDS_API USSInfoPanelWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void SetPanelInfo(const FText& InTitle, UTexture2D* InImage);
	void SetPanelContent(UWidget* InContent);
	void SetPanelAction(UWidget* InAction);
    void SetPanelObservation(UWidget* InObservation);
    void SetPortraitRegion(FVector2D UVMin, FVector2D UVMax);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> TargetImage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CloseButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UNamedSlot> ContentSlot;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UNamedSlot> ActionSlot;
    UPROPERTY(meta=(BindWidgetOptional))
    TObjectPtr<UNamedSlot> ObservationSlot;

private:
	UFUNCTION()
	void OnCloseClicked();	
};
