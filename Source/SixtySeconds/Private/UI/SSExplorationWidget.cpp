

#include "UI/SSExplorationWidget.h"
#include "Exploration/SSExplorationSession.h"
#include "Exploration/SSExplorationRoomWidget.h"
#include "Exploration/SSExplorationMapDefinition.h"
#include "Blueprint/WidgetTree.h"      // WidgetTree->GetAllWidgets
#include "Components/Button.h"
#include "Components/TextBlock.h"

bool USSExplorationWidget::StartExploration(USSExplorationMapDefinition* Map)
{
	Session = NewObject<USSExplorationSession>(this);
	
	if (!Session->Initialize(Map)) return false;

	Session->OnExplorationChanged.AddUniqueDynamic(this, &ThisClass::Refresh);

	TArray<UWidget*> AllWidgets;
	WidgetTree->GetAllWidgets(AllWidgets);

	RoomWidgets.Reset();
	for (UWidget* Widget : AllWidgets)
	{
		USSExplorationRoomWidget* Room = Cast<USSExplorationRoomWidget>(Widget);
		if (!Room) continue;
		
		const int32 Index = Map->FindRoomIndex(Room->GetRoomId());   // 번호를 먼저 받아둠
		if (Index == INDEX_NONE)                                       // 못 찾았으면
		{
			UE_LOG(LogTemp, Warning, TEXT("[Exploration] 지도에 없는 방 위젯: %s"), *Room->GetRoomId().ToString());
			continue;                                                  // 이 위젯은 건너뛰고 다음 위젯으로
		}

		Room->Setup(Index, Map->Rooms[Index].DisplayName);
		Room->OnRoomClicked.AddUniqueDynamic(this, &ThisClass::HandleRoomClicked);
		RoomWidgets.Add(Room);
	}

	for (int32 i = 0; i < Map->Rooms.Num(); ++i)   // 지도의 방 0번부터 끝까지
	{
		const bool bHasWidget = RoomWidgets.ContainsByPredicate(
			[i](const USSExplorationRoomWidget* Room)      // 방 위젯 하나씩 받아서
			{
				return Room->GetRoomIndex() == i;           // 번호가 i면 "있다"
			});

		if (!bHasWidget)   // 끝까지 못 찾았으면
		{
			UE_LOG(LogTemp, Warning, TEXT("[Exploration] 위젯이 없는 방: %s"), *Map->Rooms[i].RoomId.ToString());
		}
	}

	// 6. 첫 화면 그리고 성공 반환
	Refresh();
	return true;
}

void USSExplorationWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (CloseButton)   // 버튼이 WBP에 있을 때만
	{
		CloseButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleCloseClicked);
	}
}

void USSExplorationWidget::NativeDestruct()
{
	if (IsValid(Session))
	{
		Session->OnExplorationChanged.RemoveDynamic(this, &ThisClass::Refresh);
	}

	for (USSExplorationRoomWidget* Room : RoomWidgets)
	{
		if (Room)
		{
			Room->OnRoomClicked.RemoveDynamic(this, &ThisClass::HandleRoomClicked);
		}
	}

	if (CloseButton)
	{
		CloseButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleCloseClicked);
	}
	Super::NativeDestruct();
}

void USSExplorationWidget::Refresh()
{
	if (!IsValid(Session)) return;   // 세션은 따로 만든 오브젝트라 IsValid

	const ESSExplorationOutcome Outcome = Session->GetOutcome();
	const bool bInProgress = Outcome == ESSExplorationOutcome::InProgress;

	for (USSExplorationRoomWidget* Room : RoomWidgets)
	{
		const int32 Index = Room->GetRoomIndex();
		Room->SetState(
			Index == Session->GetCurrentRoom(),
			Session->CanMoveTo(Index),
			Index == Session->GetGuardRoom(),
			bInProgress && Index == Session->GetGuardNextRoom());   // 끝난 뒤엔 예고 표시 안 함
	}   // ← 방 반복은 여기서 끝

	if (TurnsText)   // 반복문 밖, 한 번만
	{
		TurnsText->SetText(FText::Format(
			NSLOCTEXT("SS", "ExploreTurns", "남은 턴 {0} / {1}"),
			Session->GetRemainingTurns(), Session->GetMap()->MaxTurns));
	}

	if (ResultText)
	{
		switch (Outcome)
		{
		case ESSExplorationOutcome::Caught:
			ResultText->SetText(NSLOCTEXT("SS", "ExploreCaught", "발각! 비상 귀환"));
			break;
		case ESSExplorationOutcome::TimeOut:
			ResultText->SetText(NSLOCTEXT("SS", "ExploreTimeOut", "시간 초과! 비상 귀환"));
			break;
		case ESSExplorationOutcome::Returned:
			ResultText->SetText(NSLOCTEXT("SS", "ExploreReturned", "귀환 성공"));
			break;
		default:
			ResultText->SetText(FText::GetEmpty());
			break;
		}
	}
}

void USSExplorationWidget::HandleRoomClicked(int32 RoomIndex)
{
	if (IsValid(Session)) Session->MoveTo(RoomIndex);
}

void USSExplorationWidget::HandleCloseClicked()
{
	RemoveFromParent();  
}
