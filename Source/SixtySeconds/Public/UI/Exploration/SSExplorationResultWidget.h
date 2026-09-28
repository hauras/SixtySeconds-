#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SSExplorationResultWidget.generated.h"

class UButton;
class UPanelWidget;
class UTextBlock;
class USSExplorationLootCard;
struct FSSExplorationResult;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSSOnExplorationResultConfirmed);

// 탐사 귀환 결과창. 가져온(또는 잃은) 물품 카드와 요약을 보여주고, 확인을 누르면 알린다.
UCLASS(Abstract)
class SIXTYSECONDS_API USSExplorationResultWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 탐사 위젯이 창을 띄운 뒤 부름. MaxTurns는 "사용 턴 8 / 12"의 뒷자리용
	void ShowResult(const FSSExplorationResult& Result, int32 MaxTurns);

	FSSOnExplorationResultConfirmed OnConfirmed;   // "은신처로 돌아가기"를 누르면 방송

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UPanelWidget> ItemContainer;   // 카드가 들어갈 칸 (Wrap Box 등)

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> ConfirmButton;         // 은신처로 돌아가기

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;          // 탐사 귀환 / 비상 귀환

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> SubtitleText;       // 무사 귀환 · … / 발각되어 …

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TurnsUsedText;      // 8 / 12

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> ItemCountText;      // 3개

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> InjuryText;         // 없음 / 체력 -30

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> EmptyText;          // 운반함이 비었을 때만 보임

	// 카드로 쓸 WBP (WBP_ExplorationLootCard) — Class Defaults에서 지정
	UPROPERTY(EditDefaultsOnly, Category="SS|Exploration")
	TSubclassOf<USSExplorationLootCard> CardClass;

	// 성공·실패 제목 색
	UPROPERTY(EditAnywhere, Category="SS|Exploration|Style")
	FLinearColor SuccessColor = FLinearColor(0.35f, 0.85f, 0.75f);

	UPROPERTY(EditAnywhere, Category="SS|Exploration|Style")
	FLinearColor FailureColor = FLinearColor(0.9f, 0.3f, 0.25f);

private:
	UFUNCTION()
	void HandleConfirmClicked();
};
