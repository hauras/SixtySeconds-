#include "UI/Exploration/SSExplorationResultWidget.h"
#include "UI/Exploration/SSExplorationLootCard.h"
#include "Exploration/SSExplorationTypes.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"

void USSExplorationResultWidget::ShowResult(const FSSExplorationResult& Result, int32 MaxTurns)
{
	const bool bReturned = Result.Outcome == ESSExplorationOutcome::Returned;

	// 제목과 설명
	if (TitleText)
	{
		TitleText->SetText(bReturned
				? NSLOCTEXT("SSExploration", "ResultTitleReturned", "탐사 귀환")
				: NSLOCTEXT("SSExploration", "ResultTitleEmergency", "비상 귀환"));
		TitleText->SetColorAndOpacity(FSlateColor(bReturned ? SuccessColor : FailureColor));
	}
	if (SubtitleText)
	{
		FText Subtitle;
		switch (Result.Outcome)
		{
		case ESSExplorationOutcome::Returned:
			Subtitle = NSLOCTEXT("SSExploration", "ResultSubReturned", "무사 귀환 · 보관함에 물품을 옮깁니다");
			break;
		case ESSExplorationOutcome::Caught:
			Subtitle = NSLOCTEXT("SSExploration", "ResultSubCaught", "경비 로봇에게 발각되어 물품을 모두 잃었습니다");
			break;
		case ESSExplorationOutcome::TimeOut:
			Subtitle = NSLOCTEXT("SSExploration", "ResultSubTimeOut", "시간이 다 되어 물품을 버리고 도망쳤습니다");
			break;
		default:
			break;
		}
		SubtitleText->SetText(Subtitle);
	}

	// 물품 카드: 실패면 잃은 물품을 어둡게 보여줌
	int32 ItemCount = 0;
	if (ItemContainer)
	{
		ItemContainer->ClearChildren();
		if (CardClass)
		{
			for (int32 i = 0; i < Result.Items.Num(); ++i)
			{
				USSExplorationLootCard* Card = CreateWidget<USSExplorationLootCard>(this, CardClass);
				if (!Card) continue;
				ItemContainer->AddChild(Card);
				Card->Setup(i, Result.Items[i]);
				Card->SetAvailable(bReturned);
				Card->SetVisibility(ESlateVisibility::HitTestInvisible); // 결과창에서는 보여주기만
				ItemCount += Result.Items[i].Quantity;
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[Exploration] CardClass not assigned in WBP_ExplorationResult Class Defaults"));
		}
	}
	if (EmptyText)
	{
		EmptyText->SetVisibility(Result.Items.IsEmpty() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	// 요약
	if (TurnsUsedText)
	{
		TurnsUsedText->SetText(FText::Format(NSLOCTEXT("SSExploration", "ResultTurns", "{0} / {1}"), Result.TurnsUsed, MaxTurns));
	}
	if (ItemCountText)
	{
		ItemCountText->SetText(FText::Format(NSLOCTEXT("SSExploration", "ResultItems", "{0}개"), bReturned ? ItemCount : 0));
	}
	if (InjuryText)
	{
		InjuryText->SetText(Result.Injury > 0.f
				? FText::Format(NSLOCTEXT("SSExploration", "ResultInjury", "체력 -{0}"), FMath::RoundToInt(Result.Injury))
				: NSLOCTEXT("SSExploration", "ResultNoInjury", "없음"));
	}
}

void USSExplorationResultWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (ConfirmButton) ConfirmButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleConfirmClicked);
}

void USSExplorationResultWidget::NativeDestruct()
{
	if (ConfirmButton) ConfirmButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleConfirmClicked);
	Super::NativeDestruct();
}

void USSExplorationResultWidget::HandleConfirmClicked()
{
	OnConfirmed.Broadcast(); // 받은 쪽(탐사 위젯)이 정산과 닫기를 처리
	RemoveFromParent();
}
