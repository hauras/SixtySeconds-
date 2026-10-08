#include "UI/Decode/SSCipherCellWidget.h"
#include "Decode/SSCipher.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/CoreStyle.h"

namespace SSCipherCellStyle
{
	FLinearColor Color(const TCHAR* Hex)
	{
		return FLinearColor::FromSRGBColor(FColor::FromHex(Hex));
	}

	UTextBlock* MonoLabel(UWidgetTree* Tree, int32 Size, const TCHAR* Hex)
	{
		UTextBlock* Widget = Tree->ConstructWidget<UTextBlock>();
		Widget->SetFont(FCoreStyle::GetDefaultFontStyle("Mono", Size));
		Widget->SetColorAndOpacity(FSlateColor(Color(Hex)));
		Widget->SetJustification(ETextJustify::Center);
		return Widget;
	}
}

void USSCipherCellWidget::Setup(int32 InCipherLetter, int32 InFrequency)
{
	CipherLetter = InCipherLetter;
	Frequency = InFrequency;
	if (CipherText) CipherText->SetText(FText::FromString(FString::Chr(FSSCipher::ToLetter(CipherLetter))));
	if (FrequencyText) FrequencyText->SetText(FText::Format(NSLOCTEXT("SSCipherCell", "Frequency", "×{0}"), Frequency));
}

void USSCipherCellWidget::ShowState(int32 GuessLetter, bool bSelected, bool bLocked)
{
	using namespace SSCipherCellStyle;

	if (GuessText)
	{
		GuessText->SetText(GuessLetter == INDEX_NONE
				? FText::FromString(TEXT("_"))
				: FText::FromString(FString::Chr(FSSCipher::ToLetter(GuessLetter))));
		GuessText->SetColorAndOpacity(FSlateColor(Color(GuessLetter == INDEX_NONE ? TEXT("C9A3F0") : TEXT("74E3EE"))));
	}

	if (CellButton)
	{
		// 선택 = 보라 테두리 느낌의 밝은 바탕, 빈칸 = 살짝 보라, 채움 = 어두운 초록
		FButtonStyle Style = CellButton->GetStyle();
		const TCHAR* Normal = bSelected ? TEXT("4A3566") : (GuessLetter == INDEX_NONE ? TEXT("2A2036") : TEXT("14201C"));
		Style.Normal.TintColor = FSlateColor(Color(Normal));
		Style.Hovered.TintColor = FSlateColor(Color(TEXT("3A2B50")));
		Style.Pressed.TintColor = FSlateColor(Color(TEXT("1D1628")));
		CellButton->SetStyle(Style);
		CellButton->SetIsEnabled(!bLocked);
	}
}

TSharedRef<SWidget> USSCipherCellWidget::RebuildWidget()
{
	using namespace SSCipherCellStyle;
	if (!WidgetTree->RootWidget)
	{
		USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
		Size->SetWidthOverride(46.f);
		Size->SetHeightOverride(66.f);
		WidgetTree->RootWidget = Size;

		CellButton = WidgetTree->ConstructWidget<UButton>();
		Size->SetContent(CellButton);

		// 위: 추측 글자(크게), 아래: 암호 글자(작게)
		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
		CellButton->SetContent(Column);
		GuessText = MonoLabel(WidgetTree, 20, TEXT("74E3EE"));
		Column->AddChildToVerticalBox(GuessText)->SetHorizontalAlignment(HAlign_Center);
		CipherText = MonoLabel(WidgetTree, 11, TEXT("8A6C45"));
		Column->AddChildToVerticalBox(CipherText)->SetHorizontalAlignment(HAlign_Center);
		FrequencyText = MonoLabel(WidgetTree, 10, TEXT("8F889C"));
		Column->AddChildToVerticalBox(FrequencyText)->SetHorizontalAlignment(HAlign_Center);

		Setup(CipherLetter);
	}
	return Super::RebuildWidget();
}

void USSCipherCellWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (CellButton) CellButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleClicked);
}

void USSCipherCellWidget::NativeDestruct()
{
	if (CellButton) CellButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleClicked);
	Super::NativeDestruct();
}

void USSCipherCellWidget::HandleClicked()
{
	OnCellClicked.Broadcast(CipherLetter);
}
