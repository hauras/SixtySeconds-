#include "Exploration/SSExplorationRoomWidget.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void USSExplorationRoomWidget::Setup(int32 InRoomIndex, const FText& InDisplayName)
{
	RoomIndex = InRoomIndex;
	if (NameText) NameText->SetText(InDisplayName);   // 이름은 DataAsset의 DisplayName을 받아서 표시
}

void USSExplorationRoomWidget::SetState(bool bIsCurrent, bool bCanMove, bool bGuardHere, bool bGuardNext)
{
	if (HighlightBorder)
	{
		// 우선순위: 현재 방 > 경비 예고 > 이동 가능 > 기본
		const FLinearColor& Color = bIsCurrent ? CurrentColor
			: (bGuardHere || bGuardNext) ? DangerColor
			: bCanMove ? MovableColor
			: NormalColor;
		HighlightBorder->SetBrushColor(Color);
	}

	if (PlayerIcon)
	{
		// HitTestInvisible: 보이지만 클릭은 아래 버튼으로 통과시킴
		PlayerIcon->SetVisibility(bIsCurrent ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}

	if (GuardIcon)
	{
		GuardIcon->SetVisibility(bGuardHere ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}

	if (RoomStateText)
	{
		const FText Status = bGuardHere
			? NSLOCTEXT("SSExploration", "GuardHere", "경비 위치")
			: bGuardNext ? NSLOCTEXT("SSExploration", "GuardNext", "다음 순찰 · 위험")
			: bIsCurrent ? NSLOCTEXT("SSExploration", "PlayerHere", "현재 위치")
			: bCanMove ? NSLOCTEXT("SSExploration", "CanMove", "이동 가능")
			: FText::GetEmpty();
		RoomStateText->SetText(Status);
		RoomStateText->SetColorAndOpacity(FSlateColor(
			(bGuardHere || bGuardNext) ? DangerColor : (bIsCurrent ? CurrentColor : MovableColor)));
	}
}

void USSExplorationRoomWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (RoomButton) RoomButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleClicked);
}

void USSExplorationRoomWidget::NativeDestruct()
{
	if (RoomButton) RoomButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleClicked);
	Super::NativeDestruct();
}

void USSExplorationRoomWidget::HandleClicked()
{
	if (RoomIndex == INDEX_NONE) return;   // Setup 전 클릭은 무시
	OnRoomClicked.Broadcast(RoomIndex);
}
