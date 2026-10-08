#include "UI/Event/SSEventWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ButtonSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WrapBox.h"
#include "Components/ScrollBox.h"
#include "Event/SSEventDirector.h"
#include "Item/SSRunSubsystem.h"

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
		FrameSlot->SetSize(FVector2D(760, 620));

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

		TObjectPtr<UButton>* Buttons[] = {&ChoiceButton0, &ChoiceButton1, &ChoiceButton2};
		TObjectPtr<UTextBlock>* Texts[] = {&ChoiceText0, &ChoiceText1, &ChoiceText2};
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

		ResultBox = WidgetTree->ConstructWidget<UVerticalBox>();
		Column->AddChildToVerticalBox(ResultBox)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		PickedText = EventLabel(WidgetTree, FText::GetEmpty(), 18, TEXT("8EA3B5"));
		ResultBox->AddChildToVerticalBox(PickedText)->SetPadding(FMargin(0, 0, 0, 18));

		// 긴 결과도 확인 버튼을 밀어내지 않도록 안쪽에서 읽는다.
		UScrollBox* ResultScroll = WidgetTree->ConstructWidget<UScrollBox>();
		UVerticalBox* ResultContent = WidgetTree->ConstructWidget<UVerticalBox>();
		ResultScroll->AddChild(ResultContent);
		OutcomeText = EventLabel(WidgetTree, FText::GetEmpty(), 23, TEXT("E3EAF0"));
		ResultContent->AddChildToVerticalBox(OutcomeText)->SetPadding(FMargin(0, 0, 0, 20));
		ChangeList = WidgetTree->ConstructWidget<UWrapBox>();
		ChangeList->SetInnerSlotPadding(FVector2D(12, 10));
		ResultContent->AddChildToVerticalBox(ChangeList);
		ResultBox->AddChildToVerticalBox(ResultScroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

		ConfirmButton = WidgetTree->ConstructWidget<UButton>();
		FButtonStyle ConfirmStyle = ConfirmButton->GetStyle();
		ConfirmStyle.Normal.TintColor = FSlateColor(EventColor(TEXT("1C3A52")));
		ConfirmStyle.Hovered.TintColor = FSlateColor(EventColor(TEXT("2D667B")));
		ConfirmStyle.Pressed.TintColor = FSlateColor(EventColor(TEXT("143044")));
		ConfirmButton->SetStyle(ConfirmStyle);
		UTextBlock* ConfirmText = EventLabel(
			WidgetTree,
			NSLOCTEXT("SSEvent", "ConfirmResult", "확인"),
			22,
			TEXT("F3EBDE"));
		ConfirmButton->SetContent(ConfirmText);
		CastChecked<UButtonSlot>(ConfirmText->Slot)->SetPadding(FMargin(36, 12));
		UVerticalBoxSlot* ConfirmSlot = ResultBox->AddChildToVerticalBox(ConfirmButton);
		ConfirmSlot->SetHorizontalAlignment(HAlign_Right);
		ConfirmSlot->SetPadding(FMargin(0, 20, 0, 0));
		ResultBox->SetVisibility(ESlateVisibility::Collapsed);
		ConfirmButton->SetVisibility(ESlateVisibility::Collapsed);
	}
	return Super::RebuildWidget();
}

void USSEventWidget::ShowEvent(USSEventDirector* InDirector, USSRunSubsystem* InRun, FName InEventId)
{
	Director = InDirector;
	Run = InRun;
	EventId = InEventId;
	ChoiceIds.Reset();
	Result = FSSEventResult();
	bShowingResult = false;
	bFinished = false;
	if (ResultBox) ResultBox->SetVisibility(ESlateVisibility::Collapsed);
	if (PickedText) PickedText->SetVisibility(ESlateVisibility::Collapsed);
	if (OutcomeText) OutcomeText->SetVisibility(ESlateVisibility::Collapsed);
	if (ChangeList)
	{
		ChangeList->ClearChildren();
		ChangeList->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (ConfirmButton) ConfirmButton->SetVisibility(ESlateVisibility::Collapsed);
	if (BodyText)
	{
		BodyText->SetVisibility(ESlateVisibility::Visible);
		BodyText->SetText(FText::GetEmpty());
	}
	if (TitleText) TitleText->SetText(FText::GetEmpty());
	UButton* ResetButtons[] = {ChoiceButton0, ChoiceButton1, ChoiceButton2};
	for (UButton* Button : ResetButtons)
		if (Button) Button->SetVisibility(ESlateVisibility::Collapsed);
	if (!IsValid(Director) || !IsValid(Run)) return;

	if (const FSSEventRow* Row = Director->FindEvent(EventId))
	{
		// {Target}을 아라 표적 이름으로 채움
		if (TitleText) TitleText->SetText(USSEventDirector::FillText(Row->Title, *Run));
		if (BodyText) BodyText->SetText(USSEventDirector::FillText(Row->Body, *Run));
	}

	const TArray<FSSEventChoiceView> Choices = Director->GetChoices(EventId, *Run);
	UButton* Buttons[] = {ChoiceButton0, ChoiceButton1, ChoiceButton2};
	UTextBlock* Texts[] = {ChoiceText0, ChoiceText1, ChoiceText2};
	if (Choices.Num() > UE_ARRAY_COUNT(Buttons))
		UE_LOG(LogTemp, Warning, TEXT("[Event] '%s' has %d choices; only %d are shown"), *EventId.ToString(), Choices.Num(), (int32)UE_ARRAY_COUNT(Buttons));

	for (int32 i = 0; i < UE_ARRAY_COUNT(Buttons); ++i)
	{
		const bool bUsed = Choices.IsValidIndex(i);
		ChoiceIds.Add(bUsed ? Choices[i].ChoiceId : NAME_None);
		if (Buttons[i])
		{
			Buttons[i]->SetVisibility(bUsed ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
			Buttons[i]->SetIsEnabled(bUsed && Choices[i].bAvailable); // 조건 안 맞는 선택지는 보이되 누를 수 없음
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
	if (ConfirmButton) ConfirmButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleConfirm);
	// 화면을 만들기 전에 사건을 받았어도 현재 단계를 표시한다.
	if (bShowingResult)
		ShowResult();
	else if (IsValid(Director) && IsValid(Run))
		ShowEvent(Director, Run, EventId);
}

void USSEventWidget::NativeDestruct()
{
	if (ChoiceButton0) ChoiceButton0->OnClicked.RemoveDynamic(this, &ThisClass::HandleChoice0);
	if (ChoiceButton1) ChoiceButton1->OnClicked.RemoveDynamic(this, &ThisClass::HandleChoice1);
	if (ChoiceButton2) ChoiceButton2->OnClicked.RemoveDynamic(this, &ThisClass::HandleChoice2);
	if (ConfirmButton) ConfirmButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleConfirm);
	Super::NativeDestruct();
}

void USSEventWidget::HandleChoice0()
{
	Choose(0);
}
void USSEventWidget::HandleChoice1()
{
	Choose(1);
}
void USSEventWidget::HandleChoice2()
{
	Choose(2);
}

void USSEventWidget::Choose(int32 Index)
{
	if (bShowingResult || bFinished || !ChoiceIds.IsValidIndex(Index) || ChoiceIds[Index].IsNone() || !IsValid(Director) || !IsValid(Run)) return;

	// 효과 적용에 성공했을 때만 결과 단계로 바꾼다.
	if (Director->ApplyChoice(EventId, ChoiceIds[Index], *Run, Result))
	{
		ShowResult();
	}
}

void USSEventWidget::ShowResult()
{
	bShowingResult = true;
	if (BodyText) BodyText->SetVisibility(ESlateVisibility::Collapsed);
	UButton* Buttons[] = {ChoiceButton0, ChoiceButton1, ChoiceButton2};
	for (UButton* Button : Buttons)
		if (Button) Button->SetVisibility(ESlateVisibility::Collapsed);
	if (ResultBox) ResultBox->SetVisibility(ESlateVisibility::Visible);
	if (PickedText)
	{
		PickedText->SetText(FText::Format(
			NSLOCTEXT("SSEvent", "PickedResult", "선택 · {0}"),
			Result.ChoiceText));
		PickedText->SetVisibility(ESlateVisibility::Visible);
	}
	if (OutcomeText)
	{
		const FText Outcome = FText::Join(FText::FromString(TEXT(" ")), Result.Lines);
		OutcomeText->SetText(Outcome);
		OutcomeText->SetVisibility(Outcome.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
	if (ChangeList)
	{
		ChangeList->ClearChildren();
		ChangeList->SetVisibility(ESlateVisibility::Visible);
		if (IsValid(Director))
		{
			for (const FSSEventChange& Change : Result.Changes)
			{
				const FText Description = Director->DescribeChange(Change);
				if (Description.IsEmpty()) continue;
				const TCHAR* Color = Change.Amount < 0 ? TEXT("F08A7A")
					: Change.Amount > 0                ? TEXT("8FE0A0")
													   : TEXT("8EA3B5");
				UBorder* Chip = WidgetTree->ConstructWidget<UBorder>();
				Chip->SetBrushColor(EventColor(TEXT("1B2C3B")));
				Chip->SetPadding(FMargin(12, 6));
				Chip->SetContent(EventLabel(WidgetTree, Description, 19, Color));
				ChangeList->AddChildToWrapBox(Chip);
			}
		}
		if (ChangeList->GetChildrenCount() == 0)
			ChangeList->AddChildToWrapBox(EventLabel(
				WidgetTree,
				NSLOCTEXT("SSEvent", "NoChanges", "변화 없음"),
				19,
				TEXT("8EA3B5")));
	}
	if (ConfirmButton)
	{
		ConfirmButton->SetVisibility(ESlateVisibility::Visible);
		ConfirmButton->SetIsEnabled(true);
	}
}

void USSEventWidget::HandleConfirm()
{
	if (!bShowingResult || bFinished) return;
	// 확인을 여러 번 눌러도 밤 종료 알림은 한 번만 보낸다.
	bFinished = true;
	if (ConfirmButton) ConfirmButton->SetIsEnabled(false);
	OnEventFinished.Broadcast();
	RemoveFromParent();
}
