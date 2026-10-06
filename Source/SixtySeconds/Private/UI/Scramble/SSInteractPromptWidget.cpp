#include "UI/Scramble/SSInteractPromptWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"

namespace SSPromptStyle
{
	FLinearColor Color(const TCHAR* Hex)
	{
		return FLinearColor::FromSRGBColor(FColor::FromHex(Hex));
	}

	// 할 수 있음: 밝은 글자 / 못 함(가방 꽉 참 등): 흐린 글자
	const TCHAR* Available = TEXT("F2E4D0");
	const TCHAR* Blocked = TEXT("9A8F82");
}

TSharedRef<SWidget> USSInteractPromptWidget::RebuildWidget()
{
	using namespace SSPromptStyle;
	if (!WidgetTree->RootWidget)
	{
		// 화면 전체를 덮는 투명 캔버스 (클릭은 통과)
		UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
		Canvas->SetVisibility(ESlateVisibility::HitTestInvisible);
		WidgetTree->RootWidget = Canvas;

		Box = WidgetTree->ConstructWidget<UBorder>();
		Box->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.6f));
		Box->SetPadding(FMargin(12, 6));
		Box->SetVisibility(ESlateVisibility::Collapsed);

		Label = WidgetTree->ConstructWidget<UTextBlock>();
		FSlateFontInfo Font = Label->GetFont();
		Font.Size = 18;
		Label->SetFont(Font);
		Label->SetColorAndOpacity(FSlateColor(Color(Available)));
		Box->SetContent(Label);

		// 글자 길이에 맞춰 크기가 정해지고, 상자의 아래 가운데가 지정한 위치에 옴
		UCanvasPanelSlot* BoxSlot = Canvas->AddChildToCanvas(Box);
		BoxSlot->SetAutoSize(true);
		BoxSlot->SetAlignment(FVector2D(0.5f, 1.f));
	}
	return Super::RebuildWidget();
}

void USSInteractPromptWidget::ShowAt(const FVector2D& WidgetPosition, const FText& Prompt, bool bAvailable)
{
	using namespace SSPromptStyle;
	if (!Box || !Label) return;

	Label->SetText(Prompt);
	Label->SetColorAndOpacity(FSlateColor(Color(bAvailable ? Available : Blocked)));
	if (UCanvasPanelSlot* BoxSlot = Cast<UCanvasPanelSlot>(Box->Slot))
	{
		BoxSlot->SetPosition(WidgetPosition);
	}
	Box->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void USSInteractPromptWidget::HidePrompt()
{
	if (Box) Box->SetVisibility(ESlateVisibility::Collapsed);
}
