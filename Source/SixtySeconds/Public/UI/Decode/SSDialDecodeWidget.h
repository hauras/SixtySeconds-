#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SSDialDecodeWidget.generated.h"

class UButton;
class UTextBlock;
class UProgressBar;
class UBorder;
class UHorizontalBox;
class USSDialSession;
class USSDialControlWidget;
class USSRunSubsystem;
class USoundBase;

// ─────────────────────────────────────────────
// 감청 해독 창 (다이얼 방식)
// 대기함의 메시지 하나를 세션(USSDialSession)으로 열고, 다이얼 칸들과 문장·신호 세기를 그림
// 판단은 세션이 함. 화면은 코드로 만듦 (WBP 없이 써도 됨)
// 여는 순서: CreateWidget → AddToViewport → StartDecode (화면이 먼저 있어야 다이얼 칸을 만들 수 있음)
// ─────────────────────────────────────────────
UCLASS()
class SIXTYSECONDS_API USSDialDecodeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 대기함 PendingIndex번째 메시지 해독 시작. 번호가 잘못됐으면 false (창을 닫을 것)
	bool StartDecode(int32 PendingIndex);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UFUNCTION() void HandleDialTurn(int32 DialIndex, int32 Step);
	UFUNCTION() void HandleHint();
	UFUNCTION() void HandleClose();
	UFUNCTION() void Refresh();
	void UpdateTimerDisplay();

	// 긴 암호문을 단어 단위로 줄바꿈한다.
	static FString FormatCipherText(const FString& Text);

	// 잠금 해제 후 한글 해석과 바뀐 것 표시
	void ShowResult();

	UPROPERTY(Transient)
	TObjectPtr<USSRunSubsystem> Run;

	UPROPERTY(Transient)
	TObjectPtr<USSDialSession> Session;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USSDialControlWidget>> DialControls;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> InfoText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> LockStateText;

	// "신호 유지 00:34" (시간제한 있는 해독만)
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TimerText;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> TimerPanel;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> TimerBar;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> ResultPanel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CipherText;

	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> DialRow;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> SignalBar;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SignalText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ResultText;

	UPROPERTY(Transient)
	TObjectPtr<UButton> HintButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> CloseButton;

	// 다이얼을 한 칸 돌릴 때 나는 소리 (/Game/Assets/Audio/Decode/SS_DialTurn_Click, 없으면 무음)
	UPROPERTY(Transient)
	TObjectPtr<USoundBase> DialTickSound;

	// 잠금 해제 결과를 한 번만 표시하려고
	bool bResultShown = false;
};
