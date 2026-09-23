
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SSSurvivorInfoContentWidget.generated.h"


class UTextBlock;
class UProgressBar;
class USSRunSubsystem;
class UBorder;
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

private:
	UFUNCTION()
	void RefreshDisplay();

	UPROPERTY(Transient)
	TObjectPtr<USSRunSubsystem> RunSubsystem;

	FName SurvivorId = NAME_None;	
};
