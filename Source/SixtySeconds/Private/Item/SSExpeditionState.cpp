#include "Item/SSExpeditionState.h"
#include "Item/SSExpeditionDefinition.h"
#include "Item/SSItemDefinition.h"
#include "Item/SSRunSubsystem.h"

USSRunSubsystem& USSExpeditionState::GetRun() const
{
	return *CastChecked<USSRunSubsystem>(GetOuter());
}

ESSExpeditionStartResult USSExpeditionState::StartExpedition(USSExpeditionDefinition* Expedition)
{
	USSRunSubsystem& Run = GetRun();

	if (Run.GetHealth() <= 0.f)
		return ESSExpeditionStartResult::PlayerDead;

	if (Run.GetActionPoints() < ExpeditionActionCost)
		return ESSExpeditionStartResult::NotEnoughActionPoints;

	if (RobotState == ESSRobotState::Exploring)
		return ESSExpeditionStartResult::RobotBusy;

	if (RobotState == ESSRobotState::Broken || RobotState == ESSRobotState::Repairing)
		return ESSExpeditionStartResult::RobotBroken;

	if (!IsValid(Expedition) || Expedition->DurationDays < 1)
		return ESSExpeditionStartResult::InvalidExpedition;

	// 비용 유효성 + ItemId 검사
	for (const FSSItemStack& CostStack : Expedition->Cost)
	{
		if (!IsValid(CostStack.Item) || CostStack.Item->ItemId.IsNone() || CostStack.Quantity <= 0)
			return ESSExpeditionStartResult::InvalidExpedition;
	}

	// 보상 유효성 검사 — 출발 전에 확인해서 비용만 내고 보상 없는 상황 방지
	for (const FSSItemStackRange& Reward : Expedition->Rewards)
	{
		if (!IsValid(Reward.Item) || Reward.Item->ItemId.IsNone() || Reward.MaxQuantity <= 0)
			return ESSExpeditionStartResult::InvalidExpedition;
	}

	// ItemId별 필요 총량 합산 후 보유량 검사
	TMap<FName, int32> RequiredTotals;
	for (const FSSItemStack& CostStack : Expedition->Cost)
		RequiredTotals.FindOrAdd(CostStack.Item->ItemId) += CostStack.Quantity;

	for (const auto& Pair : RequiredTotals)
	{
		if (Run.GetStoredQuantityById(Pair.Key) < Pair.Value)
			return ESSExpeditionStartResult::NotEnoughBattery;
	}

	// 검사 통과 후 비용 차감
	for (const auto& Pair : RequiredTotals)
		Run.RemoveStoredItemsById(Pair.Key, Pair.Value);

	Run.SpendActionPoints(ExpeditionActionCost);

	ActiveExpedition = Expedition;
	RemainingExpeditionDays = Expedition->DurationDays;
	RobotState = ESSRobotState::Exploring;
	Run.AddJournal(ESSJournalEvent::Expedition, FText::Format(NSLOCTEXT("SSJournal", "Dispatch", "{0}에 로봇을 파견했다. 소요 {1}일."), Expedition->RegionName, Expedition->DurationDays));
	for (const FSSItemStack& CostStack : Expedition->Cost)
		Run.AddJournal(ESSJournalEvent::Expedition, FText::Format(NSLOCTEXT("SSJournal", "DispatchCost", "파견 비용: {0} ×{1}"), CostStack.Item->DisplayName, CostStack.Quantity));
	OnRobotStateChanged.Broadcast();

	UE_LOG(LogTemp, Log, TEXT("[Expedition] 파견 시작 — %s, %d일 소요"),
		*Expedition->RegionName.ToString(), Expedition->DurationDays);

	return ESSExpeditionStartResult::Success;
}

void USSExpeditionState::TickDay()
{
	TickExpedition();
	TickRepair();
}

void USSExpeditionState::TickExpedition()
{
	if (RobotState != ESSRobotState::Exploring || !IsValid(ActiveExpedition)) return;

	--RemainingExpeditionDays;

	UE_LOG(LogTemp, Log, TEXT("[Expedition] 남은 일수: %d"), RemainingExpeditionDays);

	if (RemainingExpeditionDays <= 0)
		FulfillExpedition();
}

void USSExpeditionState::FulfillExpedition()
{
	if (!IsValid(ActiveExpedition)) return;
	USSRunSubsystem& Run = GetRun();

	FSSExpeditionResult Result;
	Result.ReturnDay = Run.GetCurrentDay();
	Result.RegionName = ActiveExpedition->RegionName;

	// 성공 확률 판정
	if (ActiveExpedition->SuccessRate <= 0.f || (ActiveExpedition->SuccessRate < 1.f && FMath::FRand() >= ActiveExpedition->SuccessRate))
	{
		UE_LOG(LogTemp, Log, TEXT("[Expedition] 탐사 실패 — 보상 없음"));
		const float BreakChance = ActiveExpedition->BreakdownChance;
		ActiveExpedition = nullptr;
		RemainingExpeditionDays = 0;
		RobotState = (FMath::FRand() < BreakChance) ? ESSRobotState::Broken : ESSRobotState::Idle;
		if (RobotState == ESSRobotState::Broken)
			UE_LOG(LogTemp, Log, TEXT("[Robot] 귀환 후 고장 발생"));
		LastExpeditionResult = Result;
		RecordExpeditionReturn(Result, false);
		OnRobotReturned.Broadcast(Result);
		OnRobotStateChanged.Broadcast();
		return;
	}

	// 보상 굴리기
	for (const FSSItemStackRange& Reward : ActiveExpedition->Rewards)
	{
		if (!IsValid(Reward.Item)) continue;

		const int32 Qty = FMath::RandRange(Reward.MinQuantity, Reward.MaxQuantity);
		if (Qty <= 0) continue;

		FSSItemStack ResultStack;
		ResultStack.Item = Reward.Item;
		ResultStack.Quantity = Qty;
		Result.ReceivedItems.Add(ResultStack);

		UE_LOG(LogTemp, Log, TEXT("[Expedition] 보상 지급 — %s x%d"),
			*Reward.Item->ItemId.ToString(), Qty);
	}

	// 보관함에 입고 (귀환 기록은 아래에서 따로 남기므로 입고 기록은 생략)
	Run.DepositItems(Result.ReceivedItems, false);

	const float BreakChance = ActiveExpedition->BreakdownChance;
	LastExpeditionResult = Result;
	ActiveExpedition = nullptr;
	RemainingExpeditionDays = 0;
	RobotState = (FMath::FRand() < BreakChance) ? ESSRobotState::Broken : ESSRobotState::Idle;

	UE_LOG(LogTemp, Log, TEXT("[Expedition] 귀환 완료 — Day %d"), Run.GetCurrentDay());
	if (RobotState == ESSRobotState::Broken)
		UE_LOG(LogTemp, Log, TEXT("[Robot] 귀환 후 고장 발생"));

	OnRobotReturned.Broadcast(LastExpeditionResult);
	RecordExpeditionReturn(LastExpeditionResult, true);
	OnRobotStateChanged.Broadcast();
}

void USSExpeditionState::RecordExpeditionReturn(const FSSExpeditionResult& Result, bool bSuccess)
{
	USSRunSubsystem& Run = GetRun();
	Run.AddJournal(ESSJournalEvent::Expedition, FText::Format(bSuccess ? NSLOCTEXT("SSJournal", "ReturnSuccess", "{0} 탐사에서 귀환했다.") : NSLOCTEXT("SSJournal", "ReturnFailure", "{0} 탐사에 실패하여 물자 없이 귀환했다."), Result.RegionName));
	for (const FSSItemStack& Stack : Result.ReceivedItems)
		if (IsValid(Stack.Item) && Stack.Quantity > 0)
			Run.AddJournal(ESSJournalEvent::Expedition, FText::Format(NSLOCTEXT("SSJournal", "Reward", "획득: {0} ×{1}"), Stack.Item->DisplayName, Stack.Quantity));
	if (bSuccess && Result.ReceivedItems.IsEmpty())
		Run.AddJournal(ESSJournalEvent::Expedition, NSLOCTEXT("SSJournal", "NoReward", "가져온 물자는 없다."));
	if (RobotState == ESSRobotState::Broken)
		Run.AddJournal(ESSJournalEvent::Robot, NSLOCTEXT("SSJournal", "Broken", "귀환한 탐사로봇에 고장이 발생했다."));
}

bool USSExpeditionState::RepairRobot()
{
	if (RobotState != ESSRobotState::Broken) return false;
	USSRunSubsystem& Run = GetRun();

	if (Run.GetActionPoints() < RepairActionCost)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Robot] 행동력 부족 — 수리 불가"));
		return false;
	}

	if (!Run.RemoveStoredItemsById(SSItemIds::RepairKit, 1))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Robot] 수리키트 부족 — 수리 불가"));
		return false;
	}

	Run.SpendActionPoints(RepairActionCost);

	RobotState = ESSRobotState::Repairing;
	RepairDaysRemaining = 1;
	Run.AddJournal(ESSJournalEvent::Robot, NSLOCTEXT("SSJournal", "RepairStart", "수리키트 1개를 소모해 로봇 수리를 시작했다. 소요 1일."));
	OnRobotStateChanged.Broadcast();
	UE_LOG(LogTemp, Log, TEXT("[Robot] 수리 시작 — 1일 후 복구"));
	return true;
}

void USSExpeditionState::TickRepair()
{
	if (RobotState != ESSRobotState::Repairing) return;

	--RepairDaysRemaining;
	if (RepairDaysRemaining <= 0)
	{
		RobotState = ESSRobotState::Idle;
		GetRun().AddJournal(ESSJournalEvent::Robot, NSLOCTEXT("SSJournal", "RepairDone", "탐사로봇 수리가 완료되었다."));
		OnRobotStateChanged.Broadcast();
		UE_LOG(LogTemp, Log, TEXT("[Robot] 수리 완료 — 대기 상태 복귀"));
	}
}

void USSExpeditionState::ResetRun()
{
	RobotState = ESSRobotState::Idle;
	ActiveExpedition = nullptr;
	RemainingExpeditionDays = 0;
	RepairDaysRemaining = 0;
	LastExpeditionResult = FSSExpeditionResult{};
	OnRobotStateChanged.Broadcast();
}
