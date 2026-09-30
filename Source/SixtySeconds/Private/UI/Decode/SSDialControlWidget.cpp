#include "UI/Decode/SSDialControlWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

namespace SSDialControlStyle
{
	FLinearColor Color(const TCHAR* Hex)
	{
		return FLinearColor::FromSRGBColor(FColor::FromHex(Hex));
	}

	UTextBlock* Label(UWidgetTree* Tree, const FText& Text, int32 Size, const FLinearColor& TextColor)
	{
		UTextBlock* Widget = Tree->ConstructWidget<UTextBlock>();
		Widget->SetText(Text);
		FSlateFontInfo Font = Widget->GetFont();
		Font.Size = Size;
		Widget->SetFont(Font);
		Widget->SetColorAndOpacity(FSlateColor(TextColor));
		Widget->SetJustification(ETextJustify::Center);
		return Widget;
	}

	UButton* ArrowButton(UWidgetTree* Tree, const FText& Arrow)
	{
		UButton* Widget = Tree->ConstructWidget<UButton>();
		FButtonStyle Style = Widget->GetStyle();
		Style.Normal.TintColor = FSlateColor(Color(TEXT("2B3A36")));
		Style.Hovered.TintColor = FSlateColor(Color(TEXT("3F5A54")));
		Style.Pressed.TintColor = FSlateColor(Color(TEXT("1B2623")));
		Widget->SetStyle(Style);
		Widget->SetContent(Label(Tree, Arrow, 20, Color(TEXT("CFE9E4"))));
		return Widget;
	}
}

void USSDialControlWidget::Setup(int32 InDialIndex, const FLinearColor& InColor)
{
	DialIndex = InDialIndex;
	DialColor = InColor;

	// 화면이 이미 만들어졌으면 바로 반영
	if (NameText)
	{
		NameText->SetText(FText::Format(NSLOCTEXT("SSDecode", "DialName", "다이얼 {0}"), DialIndex + 1));
		NameText->SetColorAndOpacity(FSlateColor(DialColor));
	}
	if (StrengthBar) StrengthBar->SetFillColorAndOpacity(DialColor);
	if (Needle) Needle->SetBrushColor(DialColor);
}

void USSDialControlWidget::ShowState(int32 Value, float Strength, bool bLocked)
{
	if (ValueText) ValueText->SetText(FText::FromString(FString::Printf(TEXT("%02d"), Value)));
	if (Needle) Needle->SetRenderTransformAngle(Value * (360.f / 26.f));
	if (StrengthBar) StrengthBar->SetPercent(FMath::Clamp(Strength, 0.f, 1.f));
	if (LeftButton) LeftButton->SetIsEnabled(!bLocked);
	if (RightButton) RightButton->SetIsEnabled(!bLocked);
}

TSharedRef<SWidget> USSDialControlWidget::RebuildWidget()
{
	using namespace SSDialControlStyle;
	if (!WidgetTree->RootWidget)
	{
		// 한 칸의 바탕
		UBorder* Card = WidgetTree->ConstructWidget<UBorder>();
		Card->SetBrushColor(Color(TEXT("101A17")));
		Card->SetPadding(FMargin(12, 9));
		WidgetTree->RootWidget = Card;

		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
		Card->SetContent(Column);

		// 이름 (다이얼 색)
		NameText = Label(WidgetTree, FText::GetEmpty(), 17, DialColor);
		Column->AddChildToVerticalBox(NameText);

		// 시계형 다이얼: 26개 눈금과 현재 값을 가리키는 바늘
		UCanvasPanel* Face = WidgetTree->ConstructWidget<UCanvasPanel>();
		constexpr float Center = 80.f;
		for (int32 Tick = 0; Tick < 26; ++Tick)
		{
			const float Angle = 2.f * PI * Tick / 26.f;
			UBorder* Mark = WidgetTree->ConstructWidget<UBorder>();
			Mark->SetBrushColor(Color(Tick % 5 == 0 ? TEXT("C29C65") : TEXT("665641")));
			UCanvasPanelSlot* MarkSlot = Face->AddChildToCanvas(Mark);
			MarkSlot->SetPosition(FVector2D(Center + FMath::Sin(Angle) * 68.f - 1.f,
				Center - FMath::Cos(Angle) * 68.f - 5.f));
			MarkSlot->SetSize(FVector2D(2.f, 10.f));
			Mark->SetRenderTransformPivot(FVector2D(.5f, .5f));
			Mark->SetRenderTransformAngle(Tick * (360.f / 26.f));
		}
		Needle = WidgetTree->ConstructWidget<UBorder>();
		Needle->SetBrushColor(DialColor);
		UCanvasPanelSlot* NeedleSlot = Face->AddChildToCanvas(Needle);
		NeedleSlot->SetPosition(FVector2D(Center - 2.f, Center - 55.f));
		NeedleSlot->SetSize(FVector2D(4.f, 55.f));
		Needle->SetRenderTransformPivot(FVector2D(.5f, 1.f));
		ValueText = Label(WidgetTree, FText::FromString(TEXT("00")), 28, Color(TEXT("F2E4D0")));
		UCanvasPanelSlot* ValueSlot = Face->AddChildToCanvas(ValueText);
		ValueSlot->SetAnchors(FAnchors(.5f, .5f));
		ValueSlot->SetAlignment(FVector2D(.5f, .5f));
		ValueSlot->SetAutoSize(true);
		USizeBox* FaceSize = WidgetTree->ConstructWidget<USizeBox>();
		FaceSize->SetWidthOverride(160.f);
		FaceSize->SetHeightOverride(160.f);
		FaceSize->SetContent(Face);
		Column->AddChildToVerticalBox(FaceSize)->SetHorizontalAlignment(HAlign_Center);

		// ◀ ▶ 버튼 (반씩 나눠 가짐). 시계 판 위에서는 마우스 휠로도 돌릴 수 있음
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
		LeftButton = ArrowButton(WidgetTree, NSLOCTEXT("SSDecode", "Left", "◀"));
		RightButton = ArrowButton(WidgetTree, NSLOCTEXT("SSDecode", "Right", "▶"));
		Row->AddChildToHorizontalBox(LeftButton)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		Row->AddChildToHorizontalBox(RightButton)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		Column->AddChildToVerticalBox(Row)->SetPadding(FMargin(0, 0, 0, 8));

		// 이 다이얼의 신호 세기
		StrengthBar = WidgetTree->ConstructWidget<UProgressBar>();
		StrengthBar->SetPercent(0.f);
		StrengthBar->SetFillColorAndOpacity(DialColor);
		Column->AddChildToVerticalBox(StrengthBar);

		// Setup이 먼저 불렸으면 이름·색 반영
		Setup(DialIndex, DialColor);
	}
	return Super::RebuildWidget();
}

void USSDialControlWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 버튼이 아닌 빈 곳(시계 판) 위에서도 휠을 받게
	SetVisibility(ESlateVisibility::Visible);

	if (LeftButton) LeftButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleLeft);
	if (RightButton) RightButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleRight);
}

void USSDialControlWidget::NativeDestruct()
{
	if (LeftButton) LeftButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleLeft);
	if (RightButton) RightButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleRight);
	Super::NativeDestruct();
}

FReply USSDialControlWidget::NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// 잠금이 풀려 버튼이 꺼졌으면 휠도 무시
	if (!LeftButton || !LeftButton->GetIsEnabled())
	{
		return Super::NativeOnMouseWheel(InGeometry, InMouseEvent);
	}

	const float Delta = InMouseEvent.GetWheelDelta();
	if (FMath::IsNearlyZero(Delta))
	{
		return Super::NativeOnMouseWheel(InGeometry, InMouseEvent);
	}

	OnDialTurn.Broadcast(DialIndex, Delta > 0.f ? +1 : -1);

	// 처리했다고 알려서 뒤쪽 스크롤 영역이 같이 움직이지 않게
	return FReply::Handled();
}

void USSDialControlWidget::HandleLeft()
{
	OnDialTurn.Broadcast(DialIndex, -1);
}

void USSDialControlWidget::HandleRight()
{
	OnDialTurn.Broadcast(DialIndex, +1);
}
