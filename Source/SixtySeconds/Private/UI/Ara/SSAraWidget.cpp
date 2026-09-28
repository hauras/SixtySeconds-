#include "UI/Ara/SSAraWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "Item/SSRunSubsystem.h"

namespace
{
	FLinearColor AraColor(const TCHAR* Hex)
	{
		return FLinearColor::FromSRGBColor(FColor::FromHex(Hex));
	}

	UTextBlock* AraLabel(UWidgetTree* Tree, const FText& Text, int32 Size, const TCHAR* Hex)
	{
		UTextBlock* Label = Tree->ConstructWidget<UTextBlock>();
		Label->SetText(Text);
		FSlateFontInfo Font = Label->GetFont();
		Font.Size = Size;
		Label->SetFont(Font);
		Label->SetColorAndOpacity(FSlateColor(AraColor(Hex)));
		Label->SetAutoWrapText(true);
		return Label;
	}
}

TSharedRef<SWidget> USSAraWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
		Canvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		WidgetTree->RootWidget = Canvas;

		UBorder* Frame = WidgetTree->ConstructWidget<UBorder>();
		Frame->SetBrushColor(AraColor(TEXT("45B8C7")));
		Frame->SetPadding(FMargin(2));
		UCanvasPanelSlot* FrameSlot = Canvas->AddChildToCanvas(Frame);
		FrameSlot->SetAnchors(FAnchors(0, 1));
		FrameSlot->SetAlignment(FVector2D(0, 1));
		FrameSlot->SetPosition(FVector2D(120, -145));
		FrameSlot->SetSize(FVector2D(690, 535));

		UBorder* Body = WidgetTree->ConstructWidget<UBorder>();
		Body->SetBrushColor(AraColor(TEXT("101D2B")));
		Body->SetPadding(FMargin(22, 18));
		Frame->SetContent(Body);
		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
		Body->SetContent(Column);
		Column->AddChildToVerticalBox(AraLabel(WidgetTree,
			NSLOCTEXT("SSAra", "Header", "아라  /  제7연구소 관리 AI"), 22, TEXT("76DFEA")))
			->SetPadding(FMargin(0, 0, 0, 12));

		Column->AddChildToVerticalBox(AraLabel(WidgetTree,
			NSLOCTEXT("SSAra", "MorningReport", "오늘의 브리핑"), 17, TEXT("8CBCCB")))
			->SetPadding(FMargin(0, 0, 0, 5));
		BriefingText = AraLabel(WidgetTree, Briefing, 20, TEXT("ECF5F8"));
		Column->AddChildToVerticalBox(BriefingText)->SetPadding(FMargin(0, 0, 0, 18));

		ActionPointText = AraLabel(WidgetTree, FText::GetEmpty(), 17, TEXT("8CBCCB"));
		Column->AddChildToVerticalBox(ActionPointText)->SetPadding(FMargin(0, 0, 0, 8));
		static const FText Questions[] = {
			NSLOCTEXT("SSAra", "OutsideQuestion", "밖은 안전한가?"),
			NSLOCTEXT("SSAra", "PatrolQuestion", "경비 로봇은 왜 순찰하나?"),
			NSLOCTEXT("SSAra", "SurvivorQuestion", "다른 생존자는 어디 있나?")
		};
		for (const FText& Question : Questions)
		{
			UButton* QuestionButton = WidgetTree->ConstructWidget<UButton>();
			FButtonStyle QuestionStyle = QuestionButton->GetStyle();
			QuestionStyle.Normal.TintColor = FSlateColor(AraColor(TEXT("1D3B52")));
			QuestionStyle.Hovered.TintColor = FSlateColor(AraColor(TEXT("28647B")));
			QuestionButton->SetStyle(QuestionStyle);
			QuestionButton->SetContent(AraLabel(WidgetTree, Question, 20, TEXT("ECF5F8")));
			CastChecked<UButtonSlot>(QuestionButton->GetContent()->Slot)->SetPadding(FMargin(12, 7));
			Column->AddChildToVerticalBox(QuestionButton)->SetPadding(FMargin(0, 0, 0, 6));
			QuestionButtons.Add(QuestionButton);
		}

		Column->AddChildToVerticalBox(AraLabel(WidgetTree,
			NSLOCTEXT("SSAra", "AnswerHeader", "아라의 답변"), 17, TEXT("76DFEA")))
			->SetPadding(FMargin(0, 8, 0, 5));
		AnswerText = AraLabel(WidgetTree,
			NSLOCTEXT("SSAra", "AnswerPlaceholder", "질문을 선택하세요."), 19, TEXT("ECF5F8"));
		Column->AddChildToVerticalBox(AnswerText)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

		CloseButton = WidgetTree->ConstructWidget<UButton>();
		FButtonStyle Style = CloseButton->GetStyle();
		Style.Normal.TintColor = FSlateColor(AraColor(TEXT("1D3B52")));
		Style.Hovered.TintColor = FSlateColor(AraColor(TEXT("28647B")));
		CloseButton->SetStyle(Style);
		CloseButton->SetContent(AraLabel(WidgetTree,
			NSLOCTEXT("SSAra", "Close", "닫기"), 19, TEXT("ECF5F8")));
		CastChecked<UButtonSlot>(CloseButton->GetContent()->Slot)->SetPadding(FMargin(24, 7));
		Column->AddChildToVerticalBox(CloseButton)->SetHorizontalAlignment(HAlign_Right);
	}
	return Super::RebuildWidget();
}

void USSAraWidget::SetBriefing(const FText& InBriefing)
{
	Briefing = InBriefing;
	if (BriefingText) BriefingText->SetText(Briefing);
}

void USSAraWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (AnswerText)
		AnswerText->SetText(NSLOCTEXT("SSAra", "AnswerPlaceholder", "질문을 선택하세요."));
	if (UGameInstance* GameInstance = GetGameInstance())
		RunSubsystem = GameInstance->GetSubsystem<USSRunSubsystem>();
	if (CloseButton) CloseButton->OnClicked.AddUniqueDynamic(this, &ThisClass::CloseBriefing);
	if (QuestionButtons.Num() == 3)
	{
		QuestionButtons[0]->OnClicked.AddUniqueDynamic(this, &ThisClass::AskOutside);
		QuestionButtons[1]->OnClicked.AddUniqueDynamic(this, &ThisClass::AskPatrol);
		QuestionButtons[2]->OnClicked.AddUniqueDynamic(this, &ThisClass::AskSurvivors);
	}
	RefreshQuestions();
}

void USSAraWidget::NativeDestruct()
{
	if (CloseButton) CloseButton->OnClicked.RemoveDynamic(this, &ThisClass::CloseBriefing);
	if (QuestionButtons.Num() == 3)
	{
		QuestionButtons[0]->OnClicked.RemoveDynamic(this, &ThisClass::AskOutside);
		QuestionButtons[1]->OnClicked.RemoveDynamic(this, &ThisClass::AskPatrol);
		QuestionButtons[2]->OnClicked.RemoveDynamic(this, &ThisClass::AskSurvivors);
	}
	Super::NativeDestruct();
}

void USSAraWidget::AskOutside() { AskQuestion(0); }
void USSAraWidget::AskPatrol() { AskQuestion(1); }
void USSAraWidget::AskSurvivors() { AskQuestion(2); }

void USSAraWidget::AskQuestion(int32 QuestionIndex)
{
	if (!IsValid(RunSubsystem)) return;
	FText Answer;
	if (RunSubsystem->AskAraQuestion(QuestionIndex, Answer) && AnswerText)
		AnswerText->SetText(Answer);
	RefreshQuestions();
}

void USSAraWidget::RefreshQuestions()
{
	if (!IsValid(RunSubsystem)) return;
	if (ActionPointText)
		ActionPointText->SetText(FText::Format(
			NSLOCTEXT("SSAra", "QuestionCost", "질문하기 · 질문당 행동력 1  |  남은 행동력 {0}"),
			RunSubsystem->GetActionPoints()));
	for (int32 Index = 0; Index < QuestionButtons.Num(); ++Index)
		if (QuestionButtons[Index])
			QuestionButtons[Index]->SetIsEnabled(RunSubsystem->GetActionPoints() >= 1
				&& !RunSubsystem->HasAskedAraQuestionToday(Index));
}

void USSAraWidget::CloseBriefing()
{
	RemoveFromParent();
}
