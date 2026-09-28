#include "UI/Event/SSEventWidget.h"
#include "Event/SSEventDirector.h"
#include "Item/SSRunSubsystem.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

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
	if (Director->ApplyChoice(EventId, ChoiceIds[Index], *Run))
	{
		RemoveFromParent();
	}
}
