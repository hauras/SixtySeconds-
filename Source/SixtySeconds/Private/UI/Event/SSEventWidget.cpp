#include "UI/Event/SSEventWidget.h"
#include "Event/SSEventDirector.h"
#include "Item/SSRunSubsystem.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ButtonSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

namespace
{
	FLinearColor EventColor(const TCHAR* Hex)
	{
		return FLinearColor::FromSRGBColor(FColor::FromHex(Hex));
	}

	UTextBlock* EventLabel(UWidgetTree* Tree, const FText& Text, int32 Size, const TCHAR* Hex)
	{
		UTextBlock* Label = Tree->ConstructWidget<UTextBlock>();
		Label->SetText(Text);
		FSlateFontInfo Font = Label->GetFont();
		Font.Size = Size;
		Label->SetFont(Font);
		Label->SetColorAndOpacity(FSlateColor(EventColor(Hex)));
		Label->SetAutoWrapText(true);
		return Label;
	}
}

TSharedRef<SWidget> USSEventWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
		Canvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		WidgetTree->RootWidget = Canvas;

		UBorder* Frame = WidgetTree->ConstructWidget<UBorder>();
		Frame->SetBrushColor(EventColor(TEXT("4BB9C8")));
		Frame->SetPadding(FMargin(2));
		UCanvasPanelSlot* FrameSlot = Canvas->AddChildToCanvas(Frame);
		FrameSlot->SetAnchors(FAnchors(.5f, .5f));
		FrameSlot->SetAlignment(FVector2D(.5f, .5f));
		FrameSlot->SetSize(FVector2D(760, 540));

		UBorder* Body = WidgetTree->ConstructWidget<UBorder>();
		Body->SetBrushColor(EventColor(TEXT("101D2B")));
		Body->SetPadding(FMargin(30, 24));
		Frame->SetContent(Body);
		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
		Body->SetContent(Column);
		Column->AddChildToVerticalBox(EventLabel(WidgetTree,
			NSLOCTEXT("SSEvent", "NightEyebrow", "▲  밤 사건  /  제7연구소"), 17, TEXT("EBC085")))
			->SetPadding(FMargin(0, 0, 0, 12));

		TitleText = EventLabel(WidgetTree, FText::GetEmpty(), 32, TEXT("F3EBDE"));
		Column->AddChildToVerticalBox(TitleText)->SetPadding(FMargin(0, 0, 0, 22));
		BodyText = EventLabel(WidgetTree, FText::GetEmpty(), 23, TEXT("E3EAF0"));
		Column->AddChildToVerticalBox(BodyText)->SetPadding(FMargin(0, 0, 0, 26));

		TObjectPtr<UButton>* Buttons[] = { &ChoiceButton0, &ChoiceButton1, &ChoiceButton2 };
		TObjectPtr<UTextBlock>* Texts[] = { &ChoiceText0, &ChoiceText1, &ChoiceText2 };
		for (int32 Index = 0; Index < 3; ++Index)
		{
			UButton* Button = WidgetTree->ConstructWidget<UButton>();
			FButtonStyle Style = Button->GetStyle();
			Style.Normal.TintColor = FSlateColor(EventColor(TEXT("1C3A52")));
			Style.Hovered.TintColor = FSlateColor(EventColor(TEXT("2D667B")));
			Style.Pressed.TintColor = FSlateColor(EventColor(TEXT("143044")));
			Button->SetStyle(Style);
			Column->AddChildToVerticalBox(Button)->SetPadding(FMargin(0, 0, 0, 10));
			UTextBlock* ChoiceText = EventLabel(WidgetTree, FText::GetEmpty(), 22, TEXT("F3EBDE"));
			Button->SetContent(ChoiceText);
			CastChecked<UButtonSlot>(ChoiceText->Slot)->SetPadding(FMargin(16, 10));
			*Buttons[Index] = Button;
			*Texts[Index] = ChoiceText;
		}
	}
	return Super::RebuildWidget();
}

void USSEventWidget::ShowEvent(USSEventDirector* InDirector, USSRunSubsystem* InRun, FName InEventId)
{
	Director = InDirector;
	Run = InRun;
	EventId = InEventId;
	ChoiceIds.Reset();
	if (!IsValid(Director) || !IsValid(Run)) return;

	if (const FSSEventRow* Row = Director->FindEvent(EventId))
	{
		if (TitleText) TitleText->SetText(Row->Title);
		if (BodyText)  BodyText->SetText(Row->Body);
	}

	const TArray<FSSEventChoiceView> Choices = Director->GetChoices(EventId, *Run);
	UButton* Buttons[] = { ChoiceButton0, ChoiceButton1, ChoiceButton2 };
	UTextBlock* Texts[] = { ChoiceText0, ChoiceText1, ChoiceText2 };
	if (Choices.Num() > UE_ARRAY_COUNT(Buttons))
		UE_LOG(LogTemp, Warning, TEXT("[Event] '%s' has %d choices; only %d are shown"), *EventId.ToString(), Choices.Num(), (int32)UE_ARRAY_COUNT(Buttons));

	for (int32 i = 0; i < UE_ARRAY_COUNT(Buttons); ++i)
	{
		const bool bUsed = Choices.IsValidIndex(i);
		ChoiceIds.Add(bUsed ? Choices[i].ChoiceId : NAME_None);
		if (Buttons[i])
		{
			Buttons[i]->SetVisibility(bUsed ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
			Buttons[i]->SetIsEnabled(bUsed && Choices[i].bAvailable);   // 조건 안 맞는 선택지는 보이되 누를 수 없음
		}
		if (Texts[i] && bUsed) Texts[i]->SetText(Choices[i].Text);
	}
}

void USSEventWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (ChoiceButton0) ChoiceButton0->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleChoice0);
	if (ChoiceButton1) ChoiceButton1->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleChoice1);
	if (ChoiceButton2) ChoiceButton2->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleChoice2);
}

void USSEventWidget::NativeDestruct()
{
	if (ChoiceButton0) ChoiceButton0->OnClicked.RemoveDynamic(this, &ThisClass::HandleChoice0);
	if (ChoiceButton1) ChoiceButton1->OnClicked.RemoveDynamic(this, &ThisClass::HandleChoice1);
	if (ChoiceButton2) ChoiceButton2->OnClicked.RemoveDynamic(this, &ThisClass::HandleChoice2);
	Super::NativeDestruct();
}

void USSEventWidget::HandleChoice0() { Choose(0); }
void USSEventWidget::HandleChoice1() { Choose(1); }
void USSEventWidget::HandleChoice2() { Choose(2); }

void USSEventWidget::Choose(int32 Index)
{
	if (!ChoiceIds.IsValidIndex(Index) || ChoiceIds[Index].IsNone() || !IsValid(Director) || !IsValid(Run)) return;

	// 적용 여부 판단은 디렉터가 함. 성공했을 때만 창을 닫음
	if (Director->ApplyChoice(EventId, ChoiceIds[Index], *Run, Result))
	{
		OnEventFinished.Broadcast();
		RemoveFromParent();
	}
}
