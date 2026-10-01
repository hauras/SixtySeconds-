#include "UI/Trace/SSEncryptedDataRewardWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

namespace SSEncryptedRewardStyle
{
	FLinearColor Color(const TCHAR* Hex)
	{
		return FLinearColor::FromSRGBColor(FColor::FromHex(Hex));
	}

	UTextBlock* Label(UWidgetTree* Tree, const FText& Value, int32 Size, const TCHAR* Hex)
	{
		UTextBlock* Widget = Tree->ConstructWidget<UTextBlock>();
		Widget->SetText(Value);
		FSlateFontInfo Font = Widget->GetFont();
		Font.Size = Size;
		Widget->SetFont(Font);
		Widget->SetColorAndOpacity(FSlateColor(Color(Hex)));
		Widget->SetJustification(ETextJustify::Center);
		Widget->SetAutoWrapText(true);
		return Widget;
	}
}

TSharedRef<SWidget> USSEncryptedDataRewardWidget::RebuildWidget()
{
	using namespace SSEncryptedRewardStyle;
	if (!WidgetTree->RootWidget)
	{
		UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
		WidgetTree->RootWidget = Canvas;

		UBorder* Blocker = WidgetTree->ConstructWidget<UBorder>();
		Blocker->SetBrushColor(FLinearColor(0.01f, 0.03f, 0.05f, .85f));
		UCanvasPanelSlot* BlockerSlot = Canvas->AddChildToCanvas(Blocker);
		BlockerSlot->SetAnchors(FAnchors(0, 0, 1, 1));
		BlockerSlot->SetOffsets(FMargin(0));

		UScaleBox* ScreenFit = WidgetTree->ConstructWidget<UScaleBox>();
		ScreenFit->SetStretch(EStretch::ScaleToFit);
		ScreenFit->SetStretchDirection(EStretchDirection::DownOnly);
		UCanvasPanelSlot* FitSlot = Canvas->AddChildToCanvas(ScreenFit);
		FitSlot->SetAnchors(FAnchors(0, 0, 1, 1));
		FitSlot->SetOffsets(FMargin(24));
		USizeBox* WindowSize = WidgetTree->ConstructWidget<USizeBox>();
		WindowSize->SetWidthOverride(800.f);
		WindowSize->SetHeightOverride(650.f);
		ScreenFit->SetContent(WindowSize);

		UBorder* Rim = WidgetTree->ConstructWidget<UBorder>();
		Rim->SetBrushColor(Color(TEXT("56DCE8")));
		Rim->SetPadding(FMargin(2));
		WindowSize->SetContent(Rim);

		UBorder* Body = WidgetTree->ConstructWidget<UBorder>();
		Body->SetBrushColor(Color(TEXT("0A1920")));
		Body->SetPadding(FMargin(38, 30));
		Rim->SetContent(Body);
		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
		Body->SetContent(Column);

		Column->AddChildToVerticalBox(Label(WidgetTree,
			NSLOCTEXT("SSTraceReward", "Source", "제7연구소  /  외부 통신 수신"), 17, TEXT("83BFC4")));
		Column->AddChildToVerticalBox(Label(WidgetTree,
			NSLOCTEXT("SSTraceReward", "Title", "암호문 획득!"), 42, TEXT("D8FFFF")))
			->SetPadding(FMargin(0, 12, 0, 10));

		UBorder* DataRim = WidgetTree->ConstructWidget<UBorder>();
		DataRim->SetBrushColor(Color(TEXT("369AA7")));
		DataRim->SetPadding(FMargin(2));
		Column->AddChildToVerticalBox(DataRim)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		UBorder* DataFrame = WidgetTree->ConstructWidget<UBorder>();
		DataFrame->SetBrushColor(Color(TEXT("11313A")));
		DataFrame->SetPadding(FMargin(24, 20));
		DataRim->SetContent(DataFrame);
		UVerticalBox* Data = WidgetTree->ConstructWidget<UVerticalBox>();
		DataFrame->SetContent(Data);
		Data->AddChildToVerticalBox(Label(WidgetTree,
			NSLOCTEXT("SSTraceReward", "Digital", "◇  ENCRYPTED TRANSMISSION  ◇"), 18, TEXT("7CE6EF")));

		UHorizontalBox* Hologram = WidgetTree->ConstructWidget<UHorizontalBox>();
		Data->AddChildToVerticalBox(Hologram)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		UTextBlock* LeftBytes = Label(WidgetTree,
			NSLOCTEXT("SSTraceReward", "LeftBytes", "A3 7F 2C 9D\n5E 01 A6 FF\nC4 3B 9E 02"), 16, TEXT("6FC9D3"));
		Hologram->AddChildToHorizontalBox(LeftBytes)->SetVerticalAlignment(VAlign_Center);
		UBorder* DataCoreRim = WidgetTree->ConstructWidget<UBorder>();
		DataCoreRim->SetBrushColor(Color(TEXT("56DCE8")));
		DataCoreRim->SetPadding(FMargin(2));
		UHorizontalBoxSlot* CoreSlot = Hologram->AddChildToHorizontalBox(DataCoreRim);
		CoreSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		CoreSlot->SetPadding(FMargin(24, 24));
		UBorder* DataCore = WidgetTree->ConstructWidget<UBorder>();
		DataCore->SetBrushColor(Color(TEXT("0B2630")));
		DataCore->SetPadding(FMargin(18));
		DataCoreRim->SetContent(DataCore);
		UVerticalBox* CoreColumn = WidgetTree->ConstructWidget<UVerticalBox>();
		DataCore->SetContent(CoreColumn);
		CoreColumn->AddChildToVerticalBox(Label(WidgetTree,
			NSLOCTEXT("SSTraceReward", "CoreMark", "◇"), 56, TEXT("A4F7FF")))
			->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		CoreColumn->AddChildToVerticalBox(Label(WidgetTree,
			NSLOCTEXT("SSTraceReward", "CoreLocked", "DATA LOCKED"), 20, TEXT("D8FFFF")));
		UTextBlock* RightBytes = Label(WidgetTree,
			NSLOCTEXT("SSTraceReward", "RightBytes", "0D F4 7A 21\n9C 6E 83 10\nE8 27 4D C5"), 16, TEXT("6FC9D3"));
		Hologram->AddChildToHorizontalBox(RightBytes)->SetVerticalAlignment(VAlign_Center);

		Data->AddChildToVerticalBox(Label(WidgetTree,
			NSLOCTEXT("SSTraceReward", "Code", "■■■■  /  외부 기록 패킷 수신 완료  /  복호화 필요"),
			15, TEXT("75C7D0")));

		Column->AddChildToVerticalBox(Label(WidgetTree,
			NSLOCTEXT("SSTraceReward", "ItemName", "암호화된 외부 기록"), 27, TEXT("D8FFFF")))
			->SetPadding(FMargin(0, 16, 0, 4));
		Column->AddChildToVerticalBox(Label(WidgetTree,
			NSLOCTEXT("SSTraceReward", "Stored", "해독 대기함에 추가되었습니다."), 18, TEXT("D4E3E4")));
		DetailText = Label(WidgetTree, FText::GetEmpty(), 15, TEXT("89BEC4"));
		Column->AddChildToVerticalBox(DetailText)->SetPadding(FMargin(0, 8, 0, 12));

		ConfirmButton = WidgetTree->ConstructWidget<UButton>();
		FButtonStyle ButtonStyle = ConfirmButton->GetStyle();
		ButtonStyle.Normal.TintColor = FSlateColor(Color(TEXT("17606A")));
		ButtonStyle.Hovered.TintColor = FSlateColor(Color(TEXT("238999")));
		ButtonStyle.Pressed.TintColor = FSlateColor(Color(TEXT("10434B")));
		ConfirmButton->SetStyle(ButtonStyle);
		ConfirmButton->SetContent(Label(WidgetTree, NSLOCTEXT("SSTraceReward", "Confirm", "확인"), 22, TEXT("F3FFFF")));
		CastChecked<UButtonSlot>(ConfirmButton->GetContent()->Slot)->SetPadding(FMargin(60, 9));
		Column->AddChildToVerticalBox(ConfirmButton)->SetHorizontalAlignment(HAlign_Center);
	}
	return Super::RebuildWidget();
}

void USSEncryptedDataRewardWidget::ShowReward(int32 PendingCount, const FText& Deadline)
{
	if (DetailText)
	{
		DetailText->SetText(FText::Format(
			NSLOCTEXT("SSTraceReward", "Detail", "해독 대기 {0}건  ·  {1}"), PendingCount, Deadline));
	}
}

void USSEncryptedDataRewardWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (ConfirmButton) ConfirmButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleConfirm);
}

void USSEncryptedDataRewardWidget::NativeDestruct()
{
	if (ConfirmButton) ConfirmButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleConfirm);
	Super::NativeDestruct();
}

void USSEncryptedDataRewardWidget::HandleConfirm()
{
	RemoveFromParent();
}
