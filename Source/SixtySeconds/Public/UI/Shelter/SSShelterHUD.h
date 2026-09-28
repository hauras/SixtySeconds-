#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TimerManager.h"
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
class USSEventCatalog;
class USSEventWidget;
class USSAraWidget;

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

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UButton> AraButton;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> AraUnreadText;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UImage> AraIconImage;

	UPROPERTY(EditDefaultsOnly, Category="SS|ARA")
	TObjectPtr<UTexture2D> AraIdleIconTexture;

	UPROPERTY(EditDefaultsOnly, Category="SS|ARA")
	TObjectPtr<UTexture2D> AraUnreadIconTexture;

	UPROPERTY(EditDefaultsOnly, Category="SS|ARA")
	TObjectPtr<UTexture2D> AraWarningIconTexture;

	UPROPERTY(EditDefaultsOnly, Category="SS|ARA")
	TObjectPtr<UTexture2D> AraBlinkIconTexture;

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

	// 다음 날로 넘어가지 못한 이유 표시 (배급 수량 부족 등)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NextDayMessageText;

	// 하루 사건 데이터와 사건 창 WBP — Class Defaults에서 지정
	UPROPERTY(EditDefaultsOnly, Category="SS|Event")
	TObjectPtr<USSEventCatalog> EventCatalog;

	UPROPERTY(EditDefaultsOnly, Category="SS|Event")
	TSubclassOf<USSEventWidget> EventWidgetClass;

private:
    UFUNCTION()
    void OnSurvivorSelected(FName SurvivorId);
    UFUNCTION()
    void OnSurvivorsUpdated();
    UFUNCTION()
    void HandleDayAdvanced();   // 다음 날 버튼·직접 탐사 정산 어느 쪽이든 하루가 지나면 날짜·스탯 갱신과 사망 확인
    UFUNCTION()
    void HandlePlayerStatsChanged();   // 사건 효과 등으로 스탯이 바뀜 → 다시 그리고 사망 확인
    void CheckPlayerDeath();
    void TryShowDailyEvent();          // 하루가 시작될 때 사건 하나 (없을 수도 있음)
    void RefreshAraIndicator();
    void ScheduleAraBlink();
    void BlinkAra();
    void RestoreAraBlink();
    bool IsAraWarning() const;
    FTimerHandle AraBlinkTimer;
    FTimerHandle AraBlinkRestoreTimer;

    UFUNCTION()
    void OnAraClicked();

    UPROPERTY(Transient)
    TObjectPtr<USSAraWidget> AraPanelWidget;

    UPROPERTY(Transient)
    TObjectPtr<USSEventWidget> EventWidget;
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
