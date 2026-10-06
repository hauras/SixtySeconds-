
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

	// [검사] 행동력 1 + 배터리 1 → 관찰 칸에 결과
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> InspectButton;

	// [격리] 되돌릴 수 없음. 한 번 누르면 확인 문구로 바뀌고, 한 번 더 눌러야 실행
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> IsolateButton;

private:
	UFUNCTION()
	void RefreshDisplay();

	UFUNCTION()
	void OnFoodRationChanged(bool bIsChecked);

	UFUNCTION()
	void OnWaterRationChanged(bool bIsChecked);

	UFUNCTION()
	void OnHealClicked();

	UFUNCTION()
	void OnInspectClicked();

	UFUNCTION()
	void OnIsolateClicked();

	// 격리 버튼을 한 번 눌러 확인을 기다리는 중인지
	bool bIsolateArmed = false;
	
	UPROPERTY(Transient)
	TObjectPtr<USSRunSubsystem> RunSubsystem;

	FName SurvivorId = NAME_None;	
};
