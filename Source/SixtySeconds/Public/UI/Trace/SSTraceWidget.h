#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Trace/SSTraceSession.h"
#include "SSTraceWidget.generated.h"

class UButton;
class UTextBlock;
class UProgressBar;
class USSTraceMapWidget;
class USSTraceConfig;
class USSRunSubsystem;

// 판이 끝남 (결과를 RunSubsystem에 넘긴 뒤). HUD가 듣고 들킴이면 밤 습격을 예약할 수 있음
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSSOnTraceFinished, ESSTraceOutcome, Outcome);

// ─────────────────────────────────────────────
// 외부 통신 창
// 판을 시작하고(RunSubsystem.BeginTrace), 버튼을 세션 행동으로 바꾸고, 끝나면 결과를 넘김(FinishTrace)
// 지도는 USSTraceMapWidget이 그림. WBP에서 이름을 맞춰 배치
// ─────────────────────────────────────────────
UCLASS()
class SIXTYSECONDS_API USSTraceWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 판 시작. 오늘 이미 했거나 행동력이 없으면 false (창은 열지 말 것)
	bool StartTrace(USSTraceConfig* InConfig);

	UPROPERTY(BlueprintAssignable, Category="SS|Trace")
	FSSOnTraceFinished OnTraceFinished;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// ── WBP에서 이름을 맞춰 배치 ──

	// 지도 (WBP에 SSTraceMapWidget을 부모로 한 위젯을 넣고 이름을 MapView로)
	UPROPERTY(Transient)
	TObjectPtr<USSTraceMapWidget> MapView;

	UPROPERTY(Transient)
	TObjectPtr<UButton> SendDirectButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> SendRelayButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> EndButton;

	// 판이 끝난 뒤 창 닫기
	UPROPERTY(Transient)
	TObjectPtr<UButton> CloseButton;

	// 수신 게이지 (받은 횟수 ÷ 목표)
	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> ReceiveBar;

	// "수신 60%"
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ReceiveText;

	// "남은 송신 3 / 6"
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TurnText;

	// "경유: 단자 2 · 차단 1곳"
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> RelayText;

	// "적 경계 1 / 3"
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> AlertText;

	// 안내와 결과 문구
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> LogText;

private:
	UFUNCTION() void HandleSendDirect();
	UFUNCTION() void HandleSendRelay();
	UFUNCTION() void HandleEnd();
	UFUNCTION() void HandleClose();
	UFUNCTION() void HandleMapClicked(FVector2D MapPosition);
	UFUNCTION() void Refresh();

	void SetStatus(const FText& Text);

	// 판을 끝까지 받았을 때 해독 대기함에 들어간 메시지와 기한을 기록 칸과 안내 문구에 표시
	void ShowReceivedMessage();

	UPROPERTY(Transient)
	TObjectPtr<USSTraceSession> Session;

	UPROPERTY(Transient)
	TObjectPtr<USSTraceConfig> Config;

	UPROPERTY(Transient)
	TObjectPtr<USSRunSubsystem> Run;

	// 결과를 RunSubsystem에 한 번만 넘기려고
	bool bFinished = false;
};
