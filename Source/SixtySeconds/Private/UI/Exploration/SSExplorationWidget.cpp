

#include "UI/Exploration/SSExplorationWidget.h"
#include "UI/Exploration/SSExplorationResultWidget.h"
#include "Exploration/SSExplorationSession.h"
#include "UI/Exploration/SSExplorationRoomWidget.h"
#include "Exploration/SSExplorationMapDefinition.h"
#include "Item/SSItemDefinition.h"
#include "Blueprint/WidgetTree.h"      // WidgetTree->GetAllWidgets
#include "Components/Button.h"
#include "Components/TextBlock.h"

bool USSExplorationWidget::StartExploration(USSExplorationMapDefinition* Map, int32 TurnBudget)
{
	Session = NewObject<USSExplorationSession>(this);

	if (!Session->Initialize(Map, TurnBudget)) return false;

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
	if (SearchButton) SearchButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleSearchClicked);
	if (WaitButton)   WaitButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleWaitClicked);
	if (ReturnButton) ReturnButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleReturnClicked);
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
	if (SearchButton) SearchButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleSearchClicked);
	if (WaitButton)   WaitButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleWaitClicked);
	if (ReturnButton) ReturnButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleReturnClicked);

	if (IsValid(ResultWidget))
	{
		ResultWidget->OnConfirmed.RemoveDynamic(this, &ThisClass::HandleResultConfirmed);
		ResultWidget->RemoveFromParent();   // 탐사 화면이 먼저 닫혀도 결과창이 혼자 남지 않게
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
			Session->GetRemainingTurns(), Session->GetTurnBudget()));
	}

	const USSExplorationMapDefinition* Map = Session->GetMap();
	const int32 CurrentRoom = Session->GetCurrentRoom();
	if (Map && Map->Rooms.IsValidIndex(CurrentRoom))
	{
		const FSSExplorationRoom& Room = Map->Rooms[CurrentRoom];
		if (CurrentRoomText) CurrentRoomText->SetText(Room.DisplayName);
		if (SearchStatusText)
		{
			SearchStatusText->SetText(Session->IsCurrentRoomSearched()
				? NSLOCTEXT("SS", "RoomSearched", "수색 완료")
				: Room.bSearchable
					? NSLOCTEXT("SS", "RoomNotSearched", "미수색 · 수색 가능")
					: NSLOCTEXT("SS", "RoomNotSearchable", "수색할 물품 없음"));
		}
	}

	if (RoomLootText)
	{
		FString LootSummary;
		if (Session->IsCurrentRoomSearched())
		{
			for (const FSSItemStack& Stack : Session->GetCurrentRoomLoot())
			{
				if (!IsValid(Stack.Item) || Stack.Quantity <= 0) continue;
				if (!LootSummary.IsEmpty()) LootSummary += TEXT("\n");
				LootSummary += FString::Printf(TEXT("%s  ×%d"), *Stack.Item->DisplayName.ToString(), Stack.Quantity);
			}
			if (LootSummary.IsEmpty()) LootSummary = TEXT("남은 물품 없음");
		}
		else LootSummary = TEXT("수색하면 발견 물품이 표시됩니다");
		RoomLootText->SetText(FText::FromString(LootSummary));
	}

	if (GuardStatusText && Map && Map->Rooms.IsValidIndex(Session->GetGuardRoom()))
	{
		GuardStatusText->SetText(FText::Format(
			NSLOCTEXT("SS", "ExploreGuardRoom", "경비 위치: {0}"),
			Map->Rooms[Session->GetGuardRoom()].DisplayName));
	}

	if (CarryText && Map)
	{
		FString CarrySummary = FString::Printf(TEXT("운반 %d / %d"),
			Session->GetCarriedLoad(), Map->CarryCapacity);
		for (const FSSItemStack& Stack : Session->GetCarried())
		{
			if (!IsValid(Stack.Item) || Stack.Quantity <= 0) continue;
			CarrySummary += FString::Printf(TEXT("\n%s  ×%d"), *Stack.Item->DisplayName.ToString(), Stack.Quantity);
		}
		CarryText->SetText(FText::FromString(CarrySummary));
	}

	// 누를 수 있는지는 세션이 판단, 위젯은 켜고 끄기만
	if (SearchButton) SearchButton->SetIsEnabled(Session->CanSearch());
	if (WaitButton)   WaitButton->SetIsEnabled(Session->CanAct());
	if (ReturnButton) ReturnButton->SetIsEnabled(Session->CanReturn());
	if (CloseButton)  CloseButton->SetIsEnabled(!bInProgress);   // 진행 중엔 못 닫음: 위험할 때 닫아서 피하는 꼼수 방지

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

	// 귀환·발각·시간 초과 모두 여기서 끝남 → 결과창 (ShowResultWindow 안에서 중복 방지)
	if (!bInProgress) ShowResultWindow();
}

void USSExplorationWidget::ShowResultWindow()
{
	if (IsValid(ResultWidget) || !IsValid(Session)) return;   // 이미 띄웠으면 무시
	if (!ResultWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Exploration] ResultWidgetClass not assigned in exploration WBP Class Defaults"));
		return;
	}

	ResultWidget = CreateWidget<USSExplorationResultWidget>(GetOwningPlayer(), ResultWidgetClass);
	if (!IsValid(ResultWidget)) return;

	ResultWidget->AddToViewport(40);   // 탐사 화면(30)보다 위
	ResultWidget->ShowResult(Session->MakeResult(), Session->GetTurnBudget());
	ResultWidget->OnConfirmed.AddUniqueDynamic(this, &ThisClass::HandleResultConfirmed);
}

void USSExplorationWidget::HandleResultConfirmed()
{
	FinishExploration();
}

void USSExplorationWidget::FinishExploration()
{
	// 결과창 확인이든, 끝난 뒤 닫기 버튼이든 정산은 딱 한 번
	if (!bFinishBroadcast && IsValid(Session) && Session->GetOutcome() != ESSExplorationOutcome::InProgress)
	{
		bFinishBroadcast = true;
		OnExplorationFinished.Broadcast(Session->MakeResult());
	}
	RemoveFromParent();
}

void USSExplorationWidget::HandleRoomClicked(int32 RoomIndex)
{
	if (IsValid(Session)) Session->MoveTo(RoomIndex);
}

void USSExplorationWidget::HandleCloseClicked()
{
	// 버튼은 진행 중에 비활성이지만, 혹시 눌려도 진행 중이면 무시
	if (IsValid(Session) && Session->GetOutcome() == ESSExplorationOutcome::InProgress) return;
	FinishExploration();
}

// 성공하면 세션이 방송해서 Refresh가 불린다
void USSExplorationWidget::HandleSearchClicked()
{
	if (IsValid(Session)) Session->Search();
}

void USSExplorationWidget::HandleWaitClicked()
{
	if (IsValid(Session)) Session->Wait();
}

void USSExplorationWidget::HandleReturnClicked()
{
	if (IsValid(Session)) Session->ReturnToShelter();
}
