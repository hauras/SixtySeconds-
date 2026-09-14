#include "Item/SSRunSubsystem.h"
#include "Item/SSItemDefinition.h"
#include "Item/SSExpeditionDefinition.h"

void USSRunSubsystem::DepositItems(const TArray<FSSItemStack>& CarriedItems)
{
	bool bChanged = false;
	for (const FSSItemStack& Incoming : CarriedItems)
	{
		if (!IsValid(Incoming.Item) || Incoming.Quantity <= 0) continue;
		bChanged = true;

		FSSItemStack* Existing = StoredItems.FindByPredicate([&Incoming](const FSSItemStack& S)
		{
			return S.Item == Incoming.Item;
		});

		if (Existing)
			Existing->Quantity += Incoming.Quantity;
		else
			StoredItems.Add(Incoming);
	}
	if (bChanged) OnStoredItemsChanged.Broadcast();
}

int32 USSRunSubsystem::GetStoredQuantity(USSItemDefinition* Item) const
{
	if (!IsValid(Item)) return 0;
	for (const FSSItemStack& Stack : StoredItems)
	{
		if (Stack.Item == Item) return Stack.Quantity;
	}
	return 0;
}

void USSRunSubsystem::ResetRun()
{
	StoredItems.Reset();

	CurrentDay = 1;
	Health = 100.f;
	Satiety = 100.f;
	Hydration = 100.f;

	RobotState = ESSRobotState::Idle;
	ActiveExpedition = nullptr;
	RemainingExpeditionDays = 0;
	RepairDaysRemaining = 0;
	LastExpeditionResult = FSSExpeditionResult{};
	OnStoredItemsChanged.Broadcast();
}

void USSRunSubsystem::InitializeShelterStats(float InHealth, float InSatiety, float InHydration)
{
	Health = FMath::Clamp(InHealth, 0.f, 100.f);
	Satiety = FMath::Clamp(InSatiety, 0.f, 100.f);
	Hydration = FMath::Clamp(InHydration, 0.f, 100.f);
}

bool USSRunSubsystem::ConsumeItem(FName ItemId)
{
	return ConsumeStoredItems(ItemId, 1);
}

bool USSRunSubsystem::ConsumeStoredItems(FName ItemId, int32 Quantity)
{
	if (ItemId.IsNone() || Quantity <= 0) return false;
	if (GetStoredQuantityById(ItemId) < Quantity) return false;

	int32 Remaining = Quantity;
	for (int32 Index = StoredItems.Num() - 1; Index >= 0 && Remaining > 0; --Index)
	{
		FSSItemStack& Stack = StoredItems[Index];
		if (!IsValid(Stack.Item) || Stack.Item->ItemId != ItemId || Stack.Quantity <= 0)
			continue;

		const int32 Take = FMath::Min(Stack.Quantity, Remaining);
		Stack.Quantity -= Take;
		Remaining -= Take;

		if (Stack.Quantity == 0)
			StoredItems.RemoveAt(Index);
	}
	if (Remaining == 0) OnStoredItemsChanged.Broadcast();
	return Remaining == 0;
}

USSItemDefinition* USSRunSubsystem::FindStoredItem(FName ItemId) const
{
	if (ItemId.IsNone()) return nullptr;
	for (const FSSItemStack& Stack : StoredItems)
	{
		if (IsValid(Stack.Item) && Stack.Quantity > 0 && Stack.Item->ItemId == ItemId)
			return Stack.Item.Get();
	}
	return nullptr;
}

bool USSRunSubsystem::UseItem(FName ItemId)
{
	return ApplyItemEffect(FindStoredItem(ItemId), false);
}

bool USSRunSubsystem::ApplyItemEffect(USSItemDefinition* Item, bool bAllowFullStat)
{
	if (Health <= 0.f || !IsValid(Item) || GetStoredQuantity(Item) <= 0
		|| !FMath::IsFinite(Item->EffectAmount) || Item->EffectAmount <= 0.f) return false;

	float* TargetStat = nullptr;
	switch (Item->UseEffect)
	{
	case ESSItemUseEffect::RestoreHealth:    TargetStat = &Health;    break;
	case ESSItemUseEffect::RestoreSatiety:   TargetStat = &Satiety;   break;
	case ESSItemUseEffect::RestoreHydration: TargetStat = &Hydration; break;
	default: return false;
	}
	if (!bAllowFullStat && *TargetStat >= 100.f) return false;
	if (Item->bConsumeOnUse && !ConsumeItem(Item->ItemId)) return false;
	*TargetStat = FMath::Clamp(*TargetStat + Item->EffectAmount, 0.f, 100.f);
	return true;
}

ESSExpeditionStartResult USSRunSubsystem::StartExpedition(USSExpeditionDefinition* Expedition)
{
	if (Health <= 0.f)
		return ESSExpeditionStartResult::PlayerDead;

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
		if (GetStoredQuantityById(Pair.Key) < Pair.Value)
			return ESSExpeditionStartResult::NotEnoughBattery;
	}

	// 검사 통과 후 비용 차감
	for (const auto& Pair : RequiredTotals)
		ConsumeStoredItems(Pair.Key, Pair.Value);

	ActiveExpedition = Expedition;
	RemainingExpeditionDays = Expedition->DurationDays;
	RobotState = ESSRobotState::Exploring;

	UE_LOG(LogTemp, Log, TEXT("[Expedition] 파견 시작 — %s, %d일 소요"),
		*Expedition->RegionName.ToString(), Expedition->DurationDays);

	return ESSExpeditionStartResult::Success;
}

void USSRunSubsystem::TickExpedition()
{
	if (RobotState != ESSRobotState::Exploring || !IsValid(ActiveExpedition)) return;

	--RemainingExpeditionDays;

	UE_LOG(LogTemp, Log, TEXT("[Expedition] 남은 일수: %d"), RemainingExpeditionDays);

	if (RemainingExpeditionDays <= 0)
		FulfillExpedition();
}

void USSRunSubsystem::FulfillExpedition()
{
	if (!IsValid(ActiveExpedition)) return;

	FSSExpeditionResult Result;
	Result.ReturnDay = CurrentDay;
	Result.RegionName = ActiveExpedition->RegionName;

	// 성공 확률 판정
	if (FMath::FRand() > ActiveExpedition->SuccessRate)
	{
		UE_LOG(LogTemp, Log, TEXT("[Expedition] 탐사 실패 — 보상 없음"));
		const float BreakChance = ActiveExpedition->BreakdownChance;
		ActiveExpedition = nullptr;
		RemainingExpeditionDays = 0;
		RobotState = (FMath::FRand() < BreakChance) ? ESSRobotState::Broken : ESSRobotState::Idle;
		if (RobotState == ESSRobotState::Broken)
			UE_LOG(LogTemp, Log, TEXT("[Robot] 귀환 후 고장 발생"));
		OnRobotReturned.Broadcast(Result);
		return;
	}

	for (const FSSItemStackRange& Reward : ActiveExpedition->Rewards)
	{
		if (!IsValid(Reward.Item)) continue;

		const int32 Qty = FMath::RandRange(Reward.MinQuantity, Reward.MaxQuantity);
		if (Qty <= 0) continue;

		FSSItemStack* Existing = StoredItems.FindByPredicate([&Reward](const FSSItemStack& S)
		{
			return S.Item == Reward.Item;
		});

		if (Existing)
			Existing->Quantity += Qty;
		else
		{
			FSSItemStack NewStack;
			NewStack.Item = Reward.Item;
			NewStack.Quantity = Qty;
			StoredItems.Add(NewStack);
		}

		FSSItemStack ResultStack;
		ResultStack.Item = Reward.Item;
		ResultStack.Quantity = Qty;
		Result.ReceivedItems.Add(ResultStack);

		UE_LOG(LogTemp, Log, TEXT("[Expedition] 보상 지급 — %s x%d"),
			*Reward.Item->ItemId.ToString(), Qty);
	}

	const float BreakChance = ActiveExpedition->BreakdownChance;
	LastExpeditionResult = Result;
	ActiveExpedition = nullptr;
	RemainingExpeditionDays = 0;
	RobotState = (FMath::FRand() < BreakChance) ? ESSRobotState::Broken : ESSRobotState::Idle;

	UE_LOG(LogTemp, Log, TEXT("[Expedition] 귀환 완료 — Day %d"), CurrentDay);
	if (RobotState == ESSRobotState::Broken)
		UE_LOG(LogTemp, Log, TEXT("[Robot] 귀환 후 고장 발생"));

	OnRobotReturned.Broadcast(LastExpeditionResult);
	OnStoredItemsChanged.Broadcast();
}

bool USSRunSubsystem::AdvanceDay(bool bGiveFood, bool bGiveWater)
{
	if (Health <= 0.f) return false;

	static const FName FoodItemId(TEXT("Food"));
	static const FName WaterItemId(TEXT("Water"));

	USSItemDefinition* Food  = FindStoredItem(FoodItemId);
	USSItemDefinition* Water = FindStoredItem(WaterItemId);

	const auto IsRationValid = [](const USSItemDefinition* Item, ESSItemUseEffect Effect)
	{
		return IsValid(Item) && Item->UseEffect == Effect
			&& FMath::IsFinite(Item->EffectAmount) && Item->EffectAmount > 0.f;
	};

	if (bGiveFood && !IsRationValid(Food, ESSItemUseEffect::RestoreSatiety))
		return false;

	if (bGiveWater && !IsRationValid(Water, ESSItemUseEffect::RestoreHydration))
		return false;

	if (bGiveFood)  ApplyItemEffect(Food,  true);
	if (bGiveWater) ApplyItemEffect(Water, true);

	Satiety   = FMath::Clamp(Satiety   - 20.f, 0.f, 100.f);
	Hydration = FMath::Clamp(Hydration - 25.f, 0.f, 100.f);

	float Damage = 0.f;
	if (Satiety   <= 0.f) Damage += 10.f;
	if (Hydration <= 0.f) Damage += 20.f;

	Health = FMath::Clamp(Health - Damage, 0.f, 100.f);

	++CurrentDay;

	// 사망했으면 탐사 진행 없이 종료
	if (Health <= 0.f) return true;

	TickExpedition();
	TickRepair();

	return true;
}

bool USSRunSubsystem::RepairRobot()
{
	if (RobotState != ESSRobotState::Broken) return false;

	static const FName RepairKitId(TEXT("RepairKit"));
	if (!ConsumeItem(RepairKitId))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Robot] 수리키트 부족 — 수리 불가"));
		return false;
	}

	RobotState = ESSRobotState::Repairing;
	RepairDaysRemaining = 1;
	UE_LOG(LogTemp, Log, TEXT("[Robot] 수리 시작 — 1일 후 복구"));
	return true;
}

void USSRunSubsystem::TickRepair()
{
	if (RobotState != ESSRobotState::Repairing) return;

	--RepairDaysRemaining;
	if (RepairDaysRemaining <= 0)
	{
		RobotState = ESSRobotState::Idle;
		UE_LOG(LogTemp, Log, TEXT("[Robot] 수리 완료 — 대기 상태 복귀"));
	}
}

int32 USSRunSubsystem::GetStoredQuantityById(FName ItemId) const
{
	if (ItemId.IsNone()) return 0;

	int32 Total = 0;
	for (const FSSItemStack& Stack : StoredItems)
	{
		if (IsValid(Stack.Item) && Stack.Item->ItemId == ItemId && Stack.Quantity > 0)
			Total += Stack.Quantity;
	}
	return Total;
}
