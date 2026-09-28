
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SSSurvivorInfoContentWidget.generated.h"


class UTextBlock;
class UProgressBar;
class UCheckBox;
class USSRunSubsystem;
class UBorder;
class UButton;
/**
 * 
 */
UCLASS()
class SIXTYSECONDS_API USSSurvivorInfoContentWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 어떤 동료의 정보인지
	void InitSurvivor(FName InSurvivorId);
    UWidget* GetObservationWidget() const;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
    UPROPERTY(meta=(BindWidgetOptional))
    TObjectPtr<UBorder> ObservationBorder;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> HealthText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SatietyText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> HydrationText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> HealthBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> SatietyBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> HydrationBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ObservationText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> FoodRationCheckBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> WaterRationCheckBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> HealButton;

private:
	UFUNCTION()
	void RefreshDisplay();

	UFUNCTION()
	void OnFoodRationChanged(bool bIsChecked);

	UFUNCTION()
	void OnWaterRationChanged(bool bIsChecked);

	UFUNCTION()
	void OnHealClicked();
	
	UPROPERTY(Transient)
	TObjectPtr<USSRunSubsystem> RunSubsystem;

	FName SurvivorId = NAME_None;	
};
