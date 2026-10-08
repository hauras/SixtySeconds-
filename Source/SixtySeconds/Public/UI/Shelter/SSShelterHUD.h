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
class USSRadioWidget;
class USSCompanionTalkWidget;
class USSTruthDecodeWidget;
class USSTraceWidget;
class USSTraceConfig;
class USSInfoPanelWidget;
class UTexture2D;
class USSSurvivorInfoContentWidget;
class USSEventCatalog;
class USSEventWidget;
class USSAraWidget;
class USSPowerPanelWidget;
class USSEndingWidget;
class UWidget;
class UDataTable;

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

	// "마지막 밤까지 N일" (없으면 날짜 글자 뒤에 붙임)
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> DaysLeftText;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> ComputerButton;

	// 무전기 (B1 비상 수신기): 외부 통신·해독. WBP에서 무전기 그림 위에 투명 버튼으로 배치
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UButton> RadioButton;

	// B2 정비 패널 (구출 퍼즐). 붙잡힌 사람이 있고 B2 덕트를 알 때만 보임. WBP에서 환풍구 그림 위에 배치
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UButton> RescueButton;

	// 엔딩 카드 (WBP_Ending) — Class Defaults에서 지정
	UPROPERTY(EditDefaultsOnly, Category="SS|Ending")
	TSubclassOf<USSEndingWidget> EndingWidgetClass;

	// B2 정비 패널 창 (WBP_PowerPanel) — Class Defaults에서 지정
	UPROPERTY(EditDefaultsOnly, Category="SS|Rescue")
	TSubclassOf<USSPowerPanelWidget> PowerPanelWidgetClass;

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

	// 동료 조사 단서 표 (DT_Clues, 줄 형식 FSSClueRow) — Class Defaults에서 지정
	UPROPERTY(EditDefaultsOnly, Category="SS|Companion")
	TObjectPtr<UDataTable> ClueTable;

	// 동료 대사 표 (DT_CompanionLines, 줄 형식 FSSCompanionLineRow) — Class Defaults에서 지정
	UPROPERTY(EditDefaultsOnly, Category="SS|Companion")
	TObjectPtr<UDataTable> CompanionLineTable;

	// 아라 대사 표 (DT_AraLines, 줄 형식 FSSAraLineRow) — Class Defaults에서 지정
	UPROPERTY(EditDefaultsOnly, Category="SS|Ara")
	TObjectPtr<UDataTable> AraLineTable;

	UPROPERTY(EditDefaultsOnly, Category="SS|Companion")
	TSubclassOf<USSCompanionTalkWidget> CompanionTalkWidgetClass;

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
		{ESSEventSpot::Monitor, FVector2D(0.69f, 0.51f)},
		{ESSEventSpot::Door, FVector2D(0.87f, 0.49f)},
		{ESSEventSpot::Vent, FVector2D(0.51f, 0.19f)},
		{ESSEventSpot::Shelf, FVector2D(0.49f, 0.32f)},
		{ESSEventSpot::Equipment, FVector2D(0.65f, 0.69f)},
		{ESSEventSpot::Terminal, FVector2D(0.11f, 0.57f)},
		{ESSEventSpot::Bed, FVector2D(0.50f, 0.70f)},
	};

private:
	UFUNCTION()
	void OnSurvivorSelected(FName SurvivorId);
	UFUNCTION()
	void OnSurvivorsUpdated();
	UFUNCTION()
	void HandleDayAdvanced(); // 다음 날 버튼·직접 탐사 정산 어느 쪽이든 하루가 지나면 날짜·스탯 갱신과 사망 확인
	UFUNCTION()
	void HandlePlayerStatsChanged(); // 사건 효과 등으로 스탯이 바뀜 → 다시 그리고 사망 확인
	void CheckPlayerDeath();
	bool TryShowDailyEvent();               // 선택된 밤 사건을 띄움
	void PlaceNightEventCue(FName EventId); // 사건 데이터의 Spot으로 발견 표시 위치와 글자를 정함
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
	void EndNight(); // 사건 선택을 마치거나 조용한 밤을 넘기면 아침으로
	void SetDayControlsEnabled(bool bEnabled);
	bool bNight = false;

	// 낮 행동 버튼이 켜져 있나 (밤·아침 페이드 동안 false). 구출 버튼이 알림으로 다시 켜지지 않게
	bool bDayControlsEnabled = true;
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
	void OnRadioClicked();

	// 컴퓨터·무전기에서 연 창이 하나라도 떠 있는지 (둘이 동시에 열리지 않게)
	bool HasOpenTerminalWindow() const;

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
	TSubclassOf<USSTruthDecodeWidget> TruthDecodeWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="SS|UI")
	TObjectPtr<USSTraceConfig> TraceConfig;

	// 컴퓨터가 기록 창과 탐사 창의 수명을 관리한다.
	UPROPERTY(Transient) TObjectPtr<USSComputerWidget> ComputerWidget;

	// 무전기가 역추적 창·해독 창의 수명을 관리한다.
	UPROPERTY(Transient) TObjectPtr<USSRadioWidget> RadioWidget;

	// 정보창의 [대화하기]: 지금 보고 있는 동료와 대화창을 엶
	UFUNCTION()
	void OnTalkClicked();

	// 동료 대화창
	UPROPERTY(Transient)
	TObjectPtr<USSCompanionTalkWidget> TalkWidget;

	// [B2 정비 패널]: 붙잡힌 첫 동료를 대상으로 패널 작업 시작
	UFUNCTION()
	void OnRescueClicked();

	// 퍼즐이 끝남: 결과를 여기서 한 번만 확정하고 패널에 결과 카드로 보여줌
	void OnPowerPanelFinished();

	// 패널 [돌아가기]: 창 닫기 (결과는 이미 확정됨)
	void OnPowerPanelClosed();

	// 패널 버튼 보이기·켜기·툴팁
	void RefreshRescueButton();

	UPROPERTY(Transient)
	TObjectPtr<USSPowerPanelWidget> PowerPanel;

	// 마지막 밤 서버실 선택으로 엔딩이 정해졌으면 아침 대신 엔딩 카드. 띄웠으면 true
	bool TryShowEnding();

	UPROPERTY(Transient)
	TObjectPtr<USSEndingWidget> EndingWidget;

	int32 CurrentDay = 1;
	float CachedHealth = 0.f;
	float CachedSatiety = 0.f;
	float CachedHydration = 0.f;
};
