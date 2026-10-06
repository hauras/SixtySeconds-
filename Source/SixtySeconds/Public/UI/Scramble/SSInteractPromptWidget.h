#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SSInteractPromptWidget.generated.h"

class UBorder;
class UTextBlock;

// ─────────────────────────────────────────────
// 가까운 아이템·동료 위에 뜨는 안내 ("식량 · [E] 줍기")
// 화면 전체 크기의 투명한 캔버스에 상자 하나를 두고, 대상 머리 위 화면 좌표로 옮김
// 클릭을 막지 않음. 스크램블 HUD가 만들고 매 프레임 위치·문장을 정함. 화면은 코드로 만듦
// ─────────────────────────────────────────────
UCLASS()
class SIXTYSECONDS_API USSInteractPromptWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 위젯 좌표(화면 배율 반영)에 안내를 띄움. 못 하는 상태면 흐린 색
	void ShowAt(const FVector2D& WidgetPosition, const FText& Prompt, bool bAvailable);

	void HidePrompt();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	// 글자를 감싸는 반투명 상자 (아래 가운데가 대상 위치에 오도록)
	UPROPERTY(Transient)
	TObjectPtr<UBorder> Box;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> Label;
};
