#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SSTruthDecodeWidget.generated.h"

class UButton;
class UTextBlock;
class UProgressBar;
class UBorder;
class UWrapBox;
class UCanvasPanel;
class USSTruthSession;
class USSCipherCellWidget;
class USSRunSubsystem;

// ─────────────────────────────────────────────
// 감청 해독 창 (진실 통신, 치환 암호)
// 1단계: [자동 해독 시작] → 해독기가 글자를 맞바꾸며 문장이 점점 영어다워지는 과정을 보여줌
// 2단계: 해독기가 멈추면 빈칸(보라)을 눌러 키보드로 글자 입력. 80% 맞으면 잠금 해제
// 판단은 세션(USSTruthSession)이 함. 화면은 코드로 만듦. 시간제한 없음, 닫아도 진행 저장
// 여는 순서: CreateWidget → AddToViewport → StartDecode
// ─────────────────────────────────────────────
UCLASS()
class SIXTYSECONDS_API USSTruthDecodeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 대기함 PendingIndex번째 진실 통신 해독 시작. 잘못된 번호거나 진실 통신이 아니면 false
	bool StartDecode(int32 PendingIndex);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// 선택한 칸에 글자 입력 (A~Z), 지우기 (Backspace·Delete)
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	UFUNCTION() void HandleSolve();
	UFUNCTION() void HandleClose();
	UFUNCTION() void HandleCellClicked(int32 CipherLetter);
	UFUNCTION() void Refresh();

	// 잠금 해제 후 한글 해석 표시
	void ShowResult();

	// 긴 문장을 단어 단위로 줄바꿈
	static FString FormatText(const FString& Text);

	// 자동 해독 연출 시간과 한 프레임에 걷는 걸음 수
	static constexpr float SolverDuration = 2.5f;
	static constexpr int32 StepsPerFrame = 6;

	UPROPERTY(Transient)
	TObjectPtr<USSRunSubsystem> Run;

	UPROPERTY(Transient)
	TObjectPtr<USSTruthSession> Session;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USSCipherCellWidget>> Cells;

	UPROPERTY(Transient, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> LockStateText;

	UPROPERTY(Transient, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> CipherText;

	UPROPERTY(Transient, meta=(BindWidgetOptional))
	TObjectPtr<UWrapBox> CellBox;

	UPROPERTY(Transient, meta=(BindWidgetOptional))
	TObjectPtr<UProgressBar> FitnessBar;

	UPROPERTY(Transient, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> FitnessText;

	UPROPERTY(Transient, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> SolverText;

	UPROPERTY(Transient, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> SolverLogText;

	UPROPERTY(Transient, meta=(BindWidgetOptional))
	TObjectPtr<UCanvasPanel> FitnessChart;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UBorder>> FitnessChartBars;

	UPROPERTY(Transient, meta=(BindWidgetOptional))
	TObjectPtr<UBorder> ResultPanel;

	UPROPERTY(Transient, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> ResultText;

	UPROPERTY(Transient, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(Transient, meta=(BindWidgetOptional))
	TObjectPtr<UButton> SolveButton;

	UPROPERTY(Transient, meta=(BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

	// 선택한 암호 글자 칸 (없으면 INDEX_NONE)
	int32 SelectedLetter = INDEX_NONE;

	// 자동 해독 연출이 흐른 시간
	float SolverElapsed = 0.f;

	bool bResultShown = false;
	int32 LastChartTries = -1;
	int32 LastLoggedAccepted = 0;
	TArray<float> FitnessHistory;
	TArray<FString> SolverLogLines;
};
