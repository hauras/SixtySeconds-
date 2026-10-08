#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Ending/SSEndingTypes.h"
#include "SSEndingWidget.generated.h"

class UButton;
class UTextBlock;

// ─────────────────────────────────────────────
// 엔딩 카드 (마지막 밤 서버실 선택 뒤)
// 문장·기록은 코드가 채우고, 모양은 WBP_Ending에서 편집
// ─────────────────────────────────────────────
UCLASS(Abstract)
class SIXTYSECONDS_API USSEndingWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 보여줄 엔딩 (AddToViewport 전후 아무 때나)
	void ShowEnding(const FSSEndingReport& Report);

	// 엔딩 이름·문장 (테스트·다른 화면에서도 같은 문장을 쓰도록 static)
	static FText GetEndingTitle(const FSSEndingReport& Report);
	static FText GetEndingBody(const FSSEndingReport& Report);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// "엔딩 ① 해결" 같은 작은 머리글
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> EndingLabelText;

	// 큰 제목
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> EndingTitleText;

	// 엔딩 문장 (여러 줄)
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> EndingBodyText;

	// 기록: 버틴 날 · 구한 사람 · 경보
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> EndingStatsText;

	// [처음부터]
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UButton> RestartButton;

	// 처음부터 할 때 열 맵 (스크램블 맵)
	UPROPERTY(EditAnywhere, Category="SS|Ending")
	FName RestartLevel = TEXT("L_SSResearchLab");

private:
	UFUNCTION()
	void OnRestartClicked();

	FSSEndingReport Shown;
	bool bHasReport = false;
};
