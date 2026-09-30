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

private:
	UFUNCTION() void HandleDialTurn(int32 DialIndex, int32 Step);
	UFUNCTION() void HandleHint();
	UFUNCTION() void HandleClose();
	UFUNCTION() void Refresh();

	// 문장을 줄마다 나누고, 줄 밑에 글자마다 담당 다이얼 번호(1,2,3…)를 적음
	static FString FormatWithMarks(const FString& Text, int32 DialCount, bool bShowMarks);

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

	// 잠금 해제 결과를 한 번만 표시하려고
	bool bResultShown = false;
};
