#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TimerManager.h"
#include "Event/SSEventTypes.h"
#include "SSShelterHUD.generated.h"

class UCheckBox;
class UImage;
class UTextBlock;
class UButton;
class USSRunSubsystem;
class USSExpeditionWidget;
class USSComputerWidget;
class USSTraceWidget;
class USSTraceConfig;
class USSInfoPanelWidget;
class UTexture2D;
class USSSurvivorInfoContentWidget;
class USSEventCatalog;
class USSEventWidget;
class USSAraWidget;
class UWidget;


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
	
	// 밤 레이어 (어두운 막 + 진행 표시). 평소엔 숨김
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> NightLayer;
	
	// "DAY 1 → NIGHT → DAY 2" 표시
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NightProgressText;

	
	// 밤 화면이 어두워지는 시간(초). 완료된 뒤 사건 단서 또는 계속 버튼 표시
	UPROPERTY(EditDefaultsOnly, Category="SS|Night", meta=(ClampMin="0.1"))
	float NightIntroSeconds = 1.3f;

	UPROPERTY(EditDefaultsOnly, Category="SS|Night", meta=(ClampMin="0.1"))
	float MorningFadeSeconds = 1.0f;

	// 밤 사건 발견 표시(!). 루트 Canvas의 직접 자식이어야 위치를 옮길 수 있음. 평소엔 Collapsed
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> NightEventCueButton;

	// 발견 표시 옆 글자 ("! 문 확인")
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NightEventCueText;

	// 조용한 밤에 누르는 "아침 맞이하기". 평소엔 Collapsed
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> NightContinueButton;

	// 사건 위치 → 화면 앵커(0~1). 배경 그림이 바뀌면 여기만 고치면 됨
	UPROPERTY(EditDefaultsOnly, Category="SS|Night")
	TMap<ESSEventSpot, FVector2D> NightSpotAnchors = {
		{ ESSEventSpot::Monitor,   FVector2D(0.69f, 0.51f) },
		{ ESSEventSpot::Door,      FVector2D(0.87f, 0.49f) },
		{ ESSEventSpot::Vent,      FVector2D(0.51f, 0.19f) },
		{ ESSEventSpot::Shelf,     FVector2D(0.49f, 0.32f) },
		{ ESSEventSpot::Equipment, FVector2D(0.65f, 0.69f) },
		{ ESSEventSpot::Terminal,  FVector2D(0.11f, 0.57f) },
		{ ESSEventSpot::Bed,       FVector2D(0.50f, 0.70f) },
	};

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
    bool TryShowDailyEvent();          // 선택된 밤 사건을 띄움
    void PlaceNightEventCue(FName EventId);   // 사건 데이터의 Spot으로 발견 표시 위치와 글자를 정함
    UFUNCTION()
    void OnNightEventCueClicked();
    UFUNCTION()
    void OnNightContinueClicked();

    // 밤: 하루가 넘어간 뒤 사건을 처리하는 동안. 낮 행동을 잠그고, 끝나면 아침 브리핑
    void BeginNight();
    void ContinueNightAfterIntro();
    void StartNightFade(bool bFadeToNight);
    void UpdateNightFade();
    UFUNCTION()
    void EndNight();                   // 사건 선택을 마치거나 조용한 밤을 넘기면 아침으로
    void SetDayControlsEnabled(bool bEnabled);
    bool bNight = false;
    FTimerHandle NightFadeTimer;
    float NightFadeStartedAt = 0.f;
    bool bFadingToNight = false;
    FName PendingNightEventId = NAME_None;
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

	// 지정하지 않으면 C++에서 구성하는 기본 통신 화면을 사용한다.
	UPROPERTY(EditDefaultsOnly, Category="SS|UI")
	TSubclassOf<USSTraceWidget> TraceWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="SS|UI")
	TObjectPtr<USSTraceConfig> TraceConfig;

	// 컴퓨터가 기록 창과 탐사 창의 수명을 관리한다.
	UPROPERTY(Transient) TObjectPtr<USSComputerWidget> ComputerWidget;

	int32 CurrentDay = 1;
	float CachedHealth = 0.f;
	float CachedSatiety = 0.f;
	float CachedHydration = 0.f;
	
};
