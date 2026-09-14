#include "UI/SSExpeditionWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Item/SSExpeditionDefinition.h"
#include "Item/SSRunSubsystem.h"
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

	if (IsValid(RunSubsystem))
		RunSubsystem->OnRobotReturned.AddUniqueDynamic(this, &USSExpeditionWidget::OnRobotReturnedHandler);

	RefreshDisplay();
}

void USSExpeditionWidget::NativeDestruct()
{
	if (IsValid(RunSubsystem))
		RunSubsystem->OnRobotReturned.RemoveDynamic(this, &USSExpeditionWidget::OnRobotReturnedHandler);

	Super::NativeDestruct();
}

void USSExpeditionWidget::RefreshDisplay()
{
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

		// 비용 표시 (첫 번째 항목만 — 현재 배터리 1개)
		if (CostText && ExpeditionDefinition->Cost.Num() > 0)
		{
			const FSSItemStack& C = ExpeditionDefinition->Cost[0];
			if (IsValid(C.Item))
				CostText->SetText(FText::Format(
					NSLOCTEXT("SS", "ExpCost", "{0} ×{1}"),
					C.Item->DisplayName, C.Quantity));
		}

		// 보상 표시 (Min~Max 범위)
		if (RewardsText)
		{
			FString RewardStr;
			for (const FSSItemStackRange& R : ExpeditionDefinition->Rewards)
			{
				if (!IsValid(R.Item)) continue;
				if (!RewardStr.IsEmpty()) RewardStr += TEXT(", ");
				if (R.MinQuantity == R.MaxQuantity)
					RewardStr += FString::Printf(TEXT("%s ×%d"), *R.Item->DisplayName.ToString(), R.MinQuantity);
				else
					RewardStr += FString::Printf(TEXT("%s ×%d~%d"), *R.Item->DisplayName.ToString(), R.MinQuantity, R.MaxQuantity);
			}
			FString RateStr = (ExpeditionDefinition->SuccessRate < 1.f)
				? FString::Printf(TEXT(" (성공률 %d%%)"), FMath::RoundToInt(ExpeditionDefinition->SuccessRate * 100.f))
				: FString();
			RewardsText->SetText(FText::Format(
				NSLOCTEXT("SS", "ExpRewards", "예상 보상: {0}{1}"),
				FText::FromString(RewardStr),
				FText::FromString(RateStr)));
		}
	}

	// 로봇 상태
	if (RobotStatusText)
	{
		switch (RunSubsystem->GetRobotState())
		{
		case ESSRobotState::Idle:
			RobotStatusText->SetText(NSLOCTEXT("SS", "RobotIdle", "대기 중"));
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
			RobotStatusText->SetText(NSLOCTEXT("SS", "RobotRepairing", "수리 중 · 1일 후 복구"));
			break;
		}
	}

	if (MessageText)
		MessageText->SetText(FText::GetEmpty());
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
		Msg = NSLOCTEXT("SS", "ExpNoBattery", "배터리가 부족합니다.");
		break;
	case ESSExpeditionStartResult::PlayerDead:
		Msg = NSLOCTEXT("SS", "ExpDead", "행동 불가.");
		break;
	default:
		Msg = NSLOCTEXT("SS", "ExpInvalid", "탐사 데이터 오류.");
		break;
	}

	if (MessageText)
		MessageText->SetText(Msg);

	RefreshDisplay();
}

void USSExpeditionWidget::OnCloseClicked()
{
	RemoveFromParent();
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
			FText::FromString(ItemStr)));

	RefreshDisplay();
}
