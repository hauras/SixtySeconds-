#include "UI/Exploration/SSExpeditionWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Item/SSExpeditionDefinition.h"
#include "Item/SSItemDefinition.h"
#include "Item/SSRunSubsystem.h"
#include "UI/Exploration/SSExplorationWidget.h"
#include "Exploration/SSExplorationMapDefinition.h"   // IsValid(DirectExplorationMap)에 완전한 타입 필요
#include "Engine/GameInstance.h"

void USSExpeditionWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UGameInstance* GI = GetGameInstance())
		RunSubsystem = GI->GetSubsystem<USSRunSubsystem>();

	if (DispatchButton)
		DispatchButton->OnClicked.AddUniqueDynamic(this, &USSExpeditionWidget::OnDispatchClicked);

	if (CloseButton)
		CloseButton->OnClicked.AddUniqueDynamic(this, &USSExpeditionWidget::OnCloseClicked);

	if (DirectExploreButton)
		DirectExploreButton->OnClicked.AddUniqueDynamic(this, &USSExpeditionWidget::OnDirectExploreClicked);

	if (IsValid(RunSubsystem))
	{
		RunSubsystem->OnRobotReturned.AddUniqueDynamic(this, &USSExpeditionWidget::OnRobotReturnedHandler);
		RunSubsystem->OnRobotStateChanged.AddUniqueDynamic(this, &USSExpeditionWidget::RefreshDisplay);
		RunSubsystem->OnStoredItemsChanged.AddUniqueDynamic(this, &USSExpeditionWidget::RefreshDisplay);
		RunSubsystem->OnActionPointsChanged.AddUniqueDynamic(this, &USSExpeditionWidget::RefreshDisplay);
	}
	if (MessageText) MessageText->SetText(FText::GetEmpty());

	RefreshDisplay();
}

void USSExpeditionWidget::NativeDestruct()
{
	if (IsValid(RunSubsystem))
	{
		RunSubsystem->OnRobotReturned.RemoveDynamic(this, &USSExpeditionWidget::OnRobotReturnedHandler);
		RunSubsystem->OnRobotStateChanged.RemoveDynamic(this, &USSExpeditionWidget::RefreshDisplay);
		RunSubsystem->OnStoredItemsChanged.RemoveDynamic(this, &USSExpeditionWidget::RefreshDisplay);
		RunSubsystem->OnActionPointsChanged.RemoveDynamic(this, &USSExpeditionWidget::RefreshDisplay);
	}
	if (DispatchButton) DispatchButton->OnClicked.RemoveDynamic(this, &USSExpeditionWidget::OnDispatchClicked);
	if (CloseButton) CloseButton->OnClicked.RemoveDynamic(this, &USSExpeditionWidget::OnCloseClicked);
	if (DirectExploreButton) DirectExploreButton->OnClicked.RemoveDynamic(this, &USSExpeditionWidget::OnDirectExploreClicked);
	if (IsValid(ExplorationWidget)) ExplorationWidget->OnExplorationFinished.RemoveDynamic(this, &USSExpeditionWidget::OnExplorationFinished);

	Super::NativeDestruct();
}

void USSExpeditionWidget::RefreshDisplay()
{
	if (DispatchButton) DispatchButton->SetIsEnabled(IsValid(RunSubsystem)
		&& IsValid(ExpeditionDefinition) && RunSubsystem->GetHealth() > 0.f
		&& RunSubsystem->GetRobotState() == ESSRobotState::Idle
		&& RunSubsystem->GetActionPoints() >= USSRunSubsystem::ExpeditionActionCost);

	if (DirectExploreButton)
	{
		// 지도가 없는 지역은 로봇 파견만 가능하므로 버튼 자체를 숨김
		const bool bHasMap = IsValid(ExpeditionDefinition) && IsValid(ExpeditionDefinition->DirectExplorationMap);
		DirectExploreButton->SetVisibility(bHasMap ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		// 남은 행동력이 지도의 최소 행동력 이상이어야 출발 가능 (행동력 → 탐사 턴)
		DirectExploreButton->SetIsEnabled(bHasMap && IsValid(RunSubsystem) && RunSubsystem->GetHealth() > 0.f
			&& ExpeditionDefinition->DirectExplorationMap->CanStartWithActionPoints(RunSubsystem->GetActionPoints()));
	}

	if (!IsValid(RunSubsystem)) return;

	// 지역 정보
	if (IsValid(ExpeditionDefinition))
	{
		if (RegionNameText)
			RegionNameText->SetText(ExpeditionDefinition->RegionName);

		if (DurationText)
			DurationText->SetText(FText::Format(
				NSLOCTEXT("SS", "ExpDuration", "소요 기간: {0}일"),
				ExpeditionDefinition->DurationDays));

		// 모든 비용과 실제 보유량을 함께 표시한다.
		if (CostText)
		{
			TMap<FName, int32> Totals;
			for (const FSSItemStack& Cost : ExpeditionDefinition->Cost)
				if (IsValid(Cost.Item)) Totals.FindOrAdd(Cost.Item->ItemId) += Cost.Quantity;
			TArray<FName> Shown;
			FString CostLines;
			for (const FSSItemStack& Cost : ExpeditionDefinition->Cost)
			{
				if (!IsValid(Cost.Item) || Shown.Contains(Cost.Item->ItemId)) continue;
				Shown.Add(Cost.Item->ItemId);
				if (!CostLines.IsEmpty()) CostLines += TEXT("\n");
				CostLines += FString::Printf(TEXT("%s ×%d  /  보유 %d"), *Cost.Item->DisplayName.ToString(),
					Totals[Cost.Item->ItemId], RunSubsystem->GetStoredQuantityById(Cost.Item->ItemId));
			}
			CostText->SetText(CostLines.IsEmpty() ? NSLOCTEXT("SSExpeditionUI", "Free", "필요한 물자가 없습니다.") : FText::FromString(CostLines));
		}
		if (RiskText) RiskText->SetText(FText::Format(NSLOCTEXT("SSExpeditionUI", "Risk", "탐사 성공률 {0}%  ·  귀환 후 고장 확률 {1}%"),
			FMath::RoundToInt(FMath::Clamp(ExpeditionDefinition->SuccessRate, 0.f, 1.f) * 100),
			FMath::RoundToInt(FMath::Clamp(ExpeditionDefinition->BreakdownChance, 0.f, 1.f) * 100)));

		// 보상 표시 (Min~Max 범위)
		if (RewardsText)
		{
			FString RewardStr;
			for (const FSSItemStackRange& R : ExpeditionDefinition->Rewards)
			{
				if (!IsValid(R.Item)) continue;
				if (!RewardStr.IsEmpty()) RewardStr += TEXT("\n");
				if (R.MinQuantity == R.MaxQuantity)
					RewardStr += FString::Printf(TEXT("%s ×%d"), *R.Item->DisplayName.ToString(), R.MinQuantity);
				else
					RewardStr += FString::Printf(TEXT("%s ×%d~%d"), *R.Item->DisplayName.ToString(), R.MinQuantity, R.MaxQuantity);
			}
			RewardsText->SetText(RewardStr.IsEmpty() ? NSLOCTEXT("SSExpeditionUI", "NoRewards", "예상되는 물자가 없습니다.") : FText::FromString(RewardStr));
		}
	}

	// 로봇 상태
	if (RobotStatusText)
	{
		switch (RunSubsystem->GetRobotState())
		{
		case ESSRobotState::Idle:
			RobotStatusText->SetText(NSLOCTEXT("SSExpeditionUI", "Idle", "로봇 대기 중 · 파견 가능"));
			break;
		case ESSRobotState::Exploring:
			RobotStatusText->SetText(FText::Format(
				NSLOCTEXT("SS", "RobotExploring", "탐사 중 · 남은 {0}일"),
				RunSubsystem->GetRemainingExpeditionDays()));
			break;
		case ESSRobotState::Broken:
			RobotStatusText->SetText(NSLOCTEXT("SS", "RobotBroken", "고장 — 수리키트 필요"));
			break;
		case ESSRobotState::Repairing:
			RobotStatusText->SetText(FText::Format(NSLOCTEXT("SSExpeditionUI", "Repairing", "수리 중 · 완료까지 {0}일"), RunSubsystem->GetRemainingRepairDays()));
			break;
		}
	}

}

void USSExpeditionWidget::OnDispatchClicked()
{
	if (!IsValid(RunSubsystem))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Expedition] RunSubsystem null"));
		return;
	}
	if (!IsValid(ExpeditionDefinition))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Expedition] ExpeditionDefinition not assigned in widget Class Defaults"));
		if (MessageText)
			MessageText->SetText(NSLOCTEXT("SS", "ExpNoDA", "탐사 데이터 미설정 (Class Defaults 확인)"));
		return;
	}

	const ESSExpeditionStartResult Result = RunSubsystem->StartExpedition(ExpeditionDefinition);

	FText Msg;
	switch (Result)
	{
	case ESSExpeditionStartResult::Success:
		Msg = NSLOCTEXT("SS", "ExpSuccess", "파견 완료.");
		break;
	case ESSExpeditionStartResult::RobotBusy:
		Msg = NSLOCTEXT("SS", "ExpBusy", "이미 탐사 중입니다.");
		break;
	case ESSExpeditionStartResult::RobotBroken:
		Msg = NSLOCTEXT("SS", "ExpBroken", "로봇이 고장났습니다. 수리키트로 수리하세요.");
		break;
	case ESSExpeditionStartResult::NotEnoughBattery:
		Msg = NSLOCTEXT("SSExpeditionUI", "NotEnough", "파견에 필요한 물자가 부족합니다.");
		break;
	case ESSExpeditionStartResult::NotEnoughActionPoints:
		Msg = NSLOCTEXT("SSExpeditionUI", "NoAP", "행동력이 부족합니다.");
		break;
	case ESSExpeditionStartResult::PlayerDead:
		Msg = NSLOCTEXT("SS", "ExpDead", "행동 불가.");
		break;
	default:
		Msg = NSLOCTEXT("SS", "ExpInvalid", "탐사 데이터 오류.");
		break;
	}

	if (MessageText)
	{
		MessageText->SetText(Msg);
		MessageText->SetColorAndOpacity(FSlateColor(FLinearColor::FromSRGBColor(
			Result == ESSExpeditionStartResult::Success ? FColor(169, 191, 135) : FColor(242, 156, 117))));
	}

	RefreshDisplay();
}

void USSExpeditionWidget::OnCloseClicked()
{
	RemoveFromParent();
}

void USSExpeditionWidget::OnDirectExploreClicked()
{
	if (!IsValid(ExpeditionDefinition) || !IsValid(ExpeditionDefinition->DirectExplorationMap) || !IsValid(RunSubsystem)) return;
	USSExplorationMapDefinition* Map = ExpeditionDefinition->DirectExplorationMap;
	const int32 ActionPoints = RunSubsystem->GetActionPoints();
	if (!Map->CanStartWithActionPoints(ActionPoints))
	{
		if (MessageText) MessageText->SetText(FText::Format(
			NSLOCTEXT("SSExpeditionUI", "ExploreNeedAP", "직접 탐사에는 행동력 {0} 이상이 필요합니다."), Map->MinActionPoints));
		return;
	}
	// 직접 탐사를 가면 그날이 끝나므로 오늘 배급을 지금 확정. 부족하면 출발 불가
	const bool bGiveFood  = ExploreFoodCheckBox  && ExploreFoodCheckBox->IsChecked();
	const bool bGiveWater = ExploreWaterCheckBox && ExploreWaterCheckBox->IsChecked();
	if (!RunSubsystem->HasRationsFor(bGiveFood, bGiveWater))
	{
		int32 RequiredFood = 0;
		int32 RequiredWater = 0;
		RunSubsystem->GetRequiredRations(bGiveFood, bGiveWater, RequiredFood, RequiredWater);
		if (MessageText) MessageText->SetText(FText::Format(
			NSLOCTEXT("SSExpeditionUI", "ExploreRationShortage", "배급 수량 부족 — 식량 {0}/{1} · 물 {2}/{3}"),
			RunSubsystem->GetStoredQuantityById(SSItemIds::Food), RequiredFood,
			RunSubsystem->GetStoredQuantityById(SSItemIds::Water), RequiredWater));
		return;
	}
	if (!ExplorationWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Exploration] ExplorationWidgetClass not assigned in WBP_Expedition Class Defaults"));
		return;
	}
	if (IsValid(ExplorationWidget) && ExplorationWidget->IsInViewport()) return;   // 이미 열려 있음

	ExplorationWidget = CreateWidget<USSExplorationWidget>(GetOwningPlayer(), ExplorationWidgetClass);
	if (!IsValid(ExplorationWidget)) return;

	ExplorationWidget->AddToViewport(30);   // 탐사 창·정보창(20)보다 위
	// 남은 행동력이 곧 탐사 시간. 행동력은 정산의 하루 경과에서 새 날 값으로 바뀜
	if (!ExplorationWidget->StartExploration(Map, Map->GetTurnBudget(ActionPoints)))
	{
		ExplorationWidget->RemoveFromParent();   // 지도 검사 실패 — 이유는 [Exploration] 로그 참고
		if (MessageText) MessageText->SetText(NSLOCTEXT("SSExpeditionUI", "ExploreMapInvalid", "탐사 지도 데이터 오류."));
		return;
	}

	bExploreGiveFood = bGiveFood;
	bExploreGiveWater = bGiveWater;
	ExplorationWidget->OnExplorationFinished.AddUniqueDynamic(this, &USSExpeditionWidget::OnExplorationFinished);
	if (MessageText) MessageText->SetText(FText::GetEmpty());
}

void USSExpeditionWidget::OnExplorationFinished(const FSSExplorationResult& Result)
{
	if (!IsValid(RunSubsystem)) return;

	// 하루 경과 → 입고/부상 → OnDayAdvanced (은신처 HUD가 날짜·스탯 갱신과 사망 확인)
	if (!RunSubsystem->ApplyExplorationResult(Result, bExploreGiveFood, bExploreGiveWater))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Exploration] Settlement failed: rations changed or player already dead"));
		if (MessageText) MessageText->SetText(NSLOCTEXT("SSExpeditionUI", "ExploreSettleFailed", "탐사 결과를 반영하지 못했습니다."));
		return;
	}

	if (ExploreFoodCheckBox)  ExploreFoodCheckBox->SetIsChecked(false);   // 다음 날 배급은 다시 선택
	if (ExploreWaterCheckBox) ExploreWaterCheckBox->SetIsChecked(false);
	RefreshDisplay();
}

void USSExpeditionWidget::OnRobotReturnedHandler(const FSSExpeditionResult& Result)
{
	FString ItemStr;
	for (const FSSItemStack& Stack : Result.ReceivedItems)
	{
		if (!IsValid(Stack.Item)) continue;
		if (!ItemStr.IsEmpty()) ItemStr += TEXT(", ");
		ItemStr += FString::Printf(TEXT("%s ×%d"), *Stack.Item->DisplayName.ToString(), Stack.Quantity);
	}

	if (MessageText)
		MessageText->SetText(FText::Format(
			NSLOCTEXT("SS", "ExpReturned", "귀환 완료 — {0}"),
			ItemStr.IsEmpty() ? NSLOCTEXT("SSExpeditionUI", "EmptyReturn", "가져온 물자 없음") : FText::FromString(ItemStr)));

	RefreshDisplay();
}
