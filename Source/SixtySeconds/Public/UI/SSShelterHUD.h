#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SSShelterHUD.generated.h"

class UCheckBox;
class UImage;
class UTextBlock;
class UButton;
class USSRunSubsystem;
class USSExpeditionWidget;

UCLASS(Abstract)
class SIXTYSECONDS_API USSShelterHUD : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitHUD(int32 InDay);
	void RefreshStats(float Health, float Satiety, float Hydration);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UImage> BackgroundImage;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> DayText;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> HealthText;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> SatietyText;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> HydrationText;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> WaterCountText;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> FoodCountText;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> BatteryCountText;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> StorageButton;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> ComputerButton;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> NextDayButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> FoodRationCheckBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> WaterRationCheckBox;
	
private:
	void RefreshDisplay();

	UFUNCTION()
	void OnStorageClicked();

	UFUNCTION()
	void OnComputerClicked();

	UFUNCTION()
	void OnNextDayClicked();

	UPROPERTY(Transient)
	TObjectPtr<USSRunSubsystem> RunSubsystem;

	// 탐사 위젯 클래스 — BP에서 지정
	UPROPERTY(EditDefaultsOnly, Category="SS|UI")
	TSubclassOf<USSExpeditionWidget> ExpeditionWidgetClass;

	// 현재 열린 탐사 위젯 참조
	UPROPERTY(Transient)
	TObjectPtr<USSExpeditionWidget> ExpeditionWidget;

	int32 CurrentDay = 1;
	float CachedHealth = 0.f;
	float CachedSatiety = 0.f;
	float CachedHydration = 0.f;
	
};
