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
class USSComputerWidget;
class USSInfoPanelWidget;
class UTexture2D;
class USSSurvivorInfoContentWidget;

UCLASS(Abstract)
class SIXTYSECONDS_API USSShelterHUD : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitHUD(int32 InDay);
	void RefreshStats(float Health, float Satiety, float Hydration);
    USSInfoPanelWidget* GetOpenInfoPanel() const { return InfoPanelWidget; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UButton> RobotButton;

	UPROPERTY(EditDefaultsOnly, Category="SS|UI")
	TSubclassOf<USSInfoPanelWidget> InfoPanelWidgetClass;
    UPROPERTY(EditDefaultsOnly, Category="SS|UI")
    TSubclassOf<USSSurvivorInfoContentWidget> SurvivorInfoContentClass;

	UPROPERTY(EditDefaultsOnly, Category="SS|UI")
	TObjectPtr<UTexture2D> RobotInfoTexture;

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

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> ActionPointsText;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> ComputerButton;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> NextDayButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> FoodRationCheckBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> WaterRationCheckBox;
	
private:
    UFUNCTION()
    void OnSurvivorSelected(FName SurvivorId);
    UFUNCTION()
    void OnSurvivorsUpdated();
    FName InspectedSurvivorId = NAME_None;
    bool bShowingRobotInfo = false;
	UFUNCTION()
	void OnRobotClicked();

	UFUNCTION()
	void RefreshRobotDisplay();

	UPROPERTY(Transient)
	TObjectPtr<USSInfoPanelWidget> InfoPanelWidget;

	UFUNCTION()
	void RefreshDisplay();

	UFUNCTION()
	void OnComputerClicked();

	UFUNCTION()
	void OnNextDayClicked();

	UPROPERTY(Transient)
	TObjectPtr<USSRunSubsystem> RunSubsystem;

	// 탐사 위젯 클래스 — BP에서 지정
	UPROPERTY(EditDefaultsOnly, Category="SS|UI")
	TSubclassOf<USSExpeditionWidget> ExpeditionWidgetClass;

	// 컴퓨터가 기록 창과 탐사 창의 수명을 관리한다.
	UPROPERTY(Transient) TObjectPtr<USSComputerWidget> ComputerWidget;

	int32 CurrentDay = 1;
	float CachedHealth = 0.f;
	float CachedSatiety = 0.f;
	float CachedHydration = 0.f;
	
};
