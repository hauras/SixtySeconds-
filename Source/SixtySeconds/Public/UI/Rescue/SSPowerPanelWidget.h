#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Rescue/SSRescueSession.h"
#include "TimerManager.h"
#include "SSPowerPanelWidget.generated.h"

class USSRescueSession;
class USSSurvivorDefinition;
class USSPowerPanelWidget;
class UButton;
class UImage;
class UTextBlock;
class UProgressBar;
class UUniformGridPanel;
class UHorizontalBox;
class UVerticalBox;
class UTexture2D;

// 배선 타일 그림 종류. 그림은 기본 방향으로 그려 두고 회전으로 4방향을 만든다
// 기본 방향: 끝 = 북, 직선 = 북+남, 꺾임 = 북+동, T자 = 북+동+남, 십자 = 전부
UENUM()
enum class ESSPowerTileShape : uint8
{
	None,
	End,
	Straight,
	Corner,
	Tee,
	Cross,
};

// 퍼즐이 끝나고 연출이 지나면 (HUD가 결과를 한 번 확정해서 ShowResult로 돌려줌)
DECLARE_MULTICAST_DELEGATE(FSSOnPowerPanelFinished);

// [돌아가기]로 닫힐 때 (HUD가 창을 치움)
DECLARE_MULTICAST_DELEGATE(FSSOnPowerPanelClosed);

// 배선 타일 하나 (클릭하면 패널에 칸 번호를 알려줌). 코드로 만드는 작은 위젯
UCLASS()
class SIXTYSECONDS_API USSPowerTileWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 만들기 전에 칸 번호와 주인 패널을 정해줌
	void Setup(USSPowerPanelWidget* InOwner, int32 InCell, float InSize);

	// 그림·회전·색 바꾸기 (Texture가 없으면 빈 칸)
	void Show(UTexture2D* Texture, float AngleDegrees, const FLinearColor& Tint);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void OnClicked();

	int32 Cell = INDEX_NONE;
	float Size = 96.f;

	UPROPERTY(Transient)
	TObjectPtr<USSPowerPanelWidget> OwnerPanel;

	UPROPERTY(Transient)
	TObjectPtr<UButton> Button;

	UPROPERTY(Transient)
	TObjectPtr<UImage> Image;
};

// ─────────────────────────────────────────────
// B2 정비 패널 (전력 우회 퍼즐) 화면
// 판·타일·램프는 코드가 채우고, 틀·글자·꾸밈은 WBP_PowerPanel에서 편집
// 세션(USSRescueSession)이 바뀔 때마다 다시 그림
// ─────────────────────────────────────────────
UCLASS(Abstract)
class SIXTYSECONDS_API USSPowerPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 보여줄 패널 작업과 구출 대상 (AddToViewport 전에 부름)
	void SetSession(USSRescueSession* InSession, const USSSurvivorDefinition* InTarget);

	// 타일이 눌렸을 때 (USSPowerTileWidget이 부름)
	void OnTileClicked(int32 Cell);

	// 비트 모양 → 그림 종류와 시계 방향 90° 회전 수 (0~3). 모양이 없으면 false
	static bool ResolveTileShape(uint8 Mask, ESSPowerTileShape& OutShape, int32& OutQuarterTurns);

	// 확정된 결과를 카드로 보여줌 (HUD가 결과를 확정한 뒤에 부름). 배선판·중단 버튼은 잠김
	void ShowResult(const FSSRescueReport& Report);

	// 퍼즐 끝 + 연출 끝
	FSSOnPowerPanelFinished OnFinished;

	// [돌아가기]로 닫힘
	FSSOnPowerPanelClosed OnClosed;

	// 성공하면 잠금이 켜진 모습을 이만큼 보여준 뒤 결과 카드 (실패·중단은 짧게)
	static constexpr float UnlockRevealSeconds = 0.6f;
	static constexpr float FailRevealSeconds = 0.15f;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// 결과 카드 등장 연출 (카드가 커지며 나타나고, 성공이면 파동이 한 번 퍼짐)
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// ── 그림 (WBP 기본값에서 지정) ──

	// 타일 그림: 기본 방향으로 그린 것 (위 ESSPowerTileShape 설명)
	UPROPERTY(EditAnywhere, Category="SS|Panel|Tiles")
	TObjectPtr<UTexture2D> TileEnd;

	UPROPERTY(EditAnywhere, Category="SS|Panel|Tiles")
	TObjectPtr<UTexture2D> TileStraight;

	UPROPERTY(EditAnywhere, Category="SS|Panel|Tiles")
	TObjectPtr<UTexture2D> TileCorner;

	UPROPERTY(EditAnywhere, Category="SS|Panel|Tiles")
	TObjectPtr<UTexture2D> TileTee;

	UPROPERTY(EditAnywhere, Category="SS|Panel|Tiles")
	TObjectPtr<UTexture2D> TileCross;

	// 타일 한 칸 크기 (px)
	UPROPERTY(EditAnywhere, Category="SS|Panel|Tiles", meta=(ClampMin="16"))
	float TileSize = 96.f;

	// 전력이 흐르는 타일 / 안 흐르는 타일 색 (그림에 곱함)
	UPROPERTY(EditAnywhere, Category="SS|Panel|Tiles")
	FLinearColor PoweredTint = FLinearColor(0.3f, 0.95f, 1.f);

	UPROPERTY(EditAnywhere, Category="SS|Panel|Tiles")
	FLinearColor IdleTint = FLinearColor(0.85f, 0.6f, 0.35f);

	// 경보 램프: 꺼짐 / 켜짐
	UPROPERTY(EditAnywhere, Category="SS|Panel|Markers")
	TObjectPtr<UTexture2D> AlarmLampIdle;

	UPROPERTY(EditAnywhere, Category="SS|Panel|Markers")
	TObjectPtr<UTexture2D> AlarmLampHot;

	// 왼쪽 전원 / 오른쪽 잠금 표시 (판의 해당 행 옆에 놓임)
	UPROPERTY(EditAnywhere, Category="SS|Panel|Markers")
	TObjectPtr<UTexture2D> SourceIcon;

	UPROPERTY(EditAnywhere, Category="SS|Panel|Markers")
	TObjectPtr<UTexture2D> LockIcon;

	// 패널 기록 한 줄 (숨은 진실 단서 1)
	UPROPERTY(EditAnywhere, Category="SS|Panel|Text", meta=(MultiLine=true))
	FText PanelLogLine = NSLOCTEXT("SSPowerPanel", "PanelLog", "정화 프로토콜 전력 예약 · 대상 구역: 제7연구소 전 층 · 상태: 대기");

	// ── WBP에 배치하는 위젯 ──

	// 타일 판 (빈 Uniform Grid Panel. 타일은 코드가 채움)
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UUniformGridPanel> TileGrid;

	// 남은 회전 "24 / 40"
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> MovesText;

	// [작업 중단] / [돌아가기]
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> ActionButton;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> ActionButtonText;

	// 남은 회전 막대
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UProgressBar> MovesBar;

	// 판 위·아래 경보 램프 줄 (빈 Horizontal Box. 칸마다 자리를 코드가 채움)
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UHorizontalBox> TopAlarmRow;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UHorizontalBox> BottomAlarmRow;

	// 판 왼쪽·오른쪽 표시 줄 (빈 Vertical Box. 전원·잠금 행에 아이콘)
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UVerticalBox> SourceColumn;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UVerticalBox> LockColumn;

	// 구출 대상 카드
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TargetNameText;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UImage> TargetPortrait;

	// 경보 카드 (0회면 "정상" 안내)
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UWidget> AlarmPanel;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> AlarmText;

	// 아래 띠: 패널 기록 / 상태 안내
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> PanelLogText;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText;

	// ── 결과 카드 (모두 선택. 없으면 상태 글자와 기존 버튼으로 결과를 보여주고 닫음) ──

	// 판 위를 덮는 막 (평소엔 숨김)
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UWidget> ResultOverlay;

	// 카드 틀 (등장 연출로 커지며 나타남)
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UWidget> ResultCardFrame;

	// 성공 때 한 번 퍼지는 파동
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UImage> ResultPulse;

	// 큰 글자: "구출 성공" / "구출 실패" / "작업 중단"
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> ResultTitleText;

	// 작은 글자: "격리실 잠금 해제" / "배선 연결 실패" / "덕트로 철수"
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> ResultSubtitleText;

	// 이름 줄: "서하린 귀환" / "서하린 · B2 격리"
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> ResultBodyText;

	// 설명: "동료가 은신처로 돌아왔습니다." / "더 손댈 시간이 없었다."
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> ResultDescriptionText;

	// 확정된 바꿔치기 사실 한 줄 (그 외엔 숨김)
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> ResultNoticeText;

	// 이동 "B2 격리 → B1 은신처" / 위치 "B1 은신처 복귀"
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> ResultTransferText;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> ResultLocationText;

	// 배지: "행동력 5 소모" / "경보 1회"
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> ResultCostText;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> ResultAlarmCountText;

	// 상세 줄: 배지가 있으면 경보 기록 문장만(경보 0회면 숨김), 없으면 대가 전부
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> ResultDetailText;

	// 인원·격리 상태 그림 (성공 / 실패 그림)
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UImage> ResultPortrait;

	UPROPERTY(EditDefaultsOnly, Category="SS|Panel|Result")
	TObjectPtr<UTexture2D> ResultSuccessTexture;

	UPROPERTY(EditDefaultsOnly, Category="SS|Panel|Result")
	TObjectPtr<UTexture2D> ResultFailureTexture;

	// [돌아가기]
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UButton> ResultButton;

	// 실패 색 (머리글)
	UPROPERTY(EditAnywhere, Category="SS|Panel|Result")
	FLinearColor FailTint = FLinearColor(0.95f, 0.6f, 0.25f);

	// 바꿔치기 한 줄 색
	UPROPERTY(EditAnywhere, Category="SS|Panel|Result")
	FLinearColor NoticeTint = FLinearColor(0.95f, 0.3f, 0.25f);

private:
	// 타일·램프·표시를 판 크기에 맞게 만듦 (한 번)
	void BuildBoard();

	// 세션 상태대로 다시 그림
	void Refresh();

	UFUNCTION()
	void OnActionClicked();

	UFUNCTION()
	void OnResultClicked();

	// 세션이 끝난 걸 처음 봤을 때: 연출 시간 뒤 OnFinished
	void BeginFinishReveal();
	void NotifyFinished();

	UTexture2D* GetTileTexture(ESSPowerTileShape Shape) const;

	// 칸 크기의 빈 자리 (램프·표시 줄용). Texture가 있으면 그림을 넣고 그 그림을 돌려줌
	UImage* AddMarkerSlot(class UPanelWidget* Parent, UTexture2D* Texture);

	UPROPERTY(Transient)
	TObjectPtr<USSRescueSession> Session;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USSPowerTileWidget>> Tiles;

	// 경보 단자 번호 순서대로 램프 그림
	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> AlarmLamps;

	UPROPERTY(Transient)
	TObjectPtr<UImage> LockImage;

	FDelegateHandle SessionChangedHandle;

	// 중단은 두 번 눌러야 (행동력을 이미 썼으므로)
	bool bAbortArmed = false;

	// 끝을 알렸나 / 결과 카드를 보여줬나 (닫기는 결과가 확정된 뒤에만)
	bool bFinishNotified = false;
	bool bResultShown = false;

	// 결과 카드에 쓸 대상 이름·그림 (SetSession 때 받아 둠)
	FText TargetName;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> TargetTexture;

	FTimerHandle RevealTimer;
	float ResultRevealElapsed = 0.f;
	bool bResultSuccess = false;
};
