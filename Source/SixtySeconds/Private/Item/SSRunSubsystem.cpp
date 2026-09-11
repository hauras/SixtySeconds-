#include "Item/SSRunSubsystem.h"
#include "Item/SSItemDefinition.h"
#include "Item/SSExpeditionDefinition.h"

void USSRunSubsystem::DepositItems(const TArray<FSSItemStack>& CarriedItems)
{
	for (const FSSItemStack& Incoming : CarriedItems)
	{
		if (!IsValid(Incoming.Item) || Incoming.Quantity <= 0) continue;

		FSSItemStack* Existing = StoredItems.FindByPredicate([&Incoming](const FSSItemStack& S)
		{
			return S.Item == Incoming.Item;
		});

		if (Existing)
			Existing->Quantity += Incoming.Quantity;
		else
			StoredItems.Add(Incoming);
	}
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
	LastExpeditionResult = FSSExpeditionResult{};
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

	if (RobotState != ESSRobotState::Idle)
		return ESSExpeditionStartResult::RobotBusy;

	if (!IsValid(Expedition) || Expedition->DurationDays < 1)
		return ESSExpeditionStartResult::InvalidExpedition;

	// 비용 유효성 + ItemId 검사
	for (const FSSItemStack& CostStack : Expedition->Cost)
	{
		if (!IsValid(CostStack.Item) || CostStack.Item->ItemId.IsNone() || CostStack.Quantity <= 0)
			return ESSExpeditionStartResult::InvalidExpedition;
	}

	// 보상 유효성 검사 — 출발 전에 확인해서 비용만 내고 보상 없는 상황 방지
	for (const FSSItemStack& Reward : Expedition->Rewards)
	{
		if (!IsValid(Reward.Item) || Reward.Item->ItemId.IsNone() || Reward.Quantity <= 0)
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

	// TODO: 성공 확률 적용. 아래 주석 해제 후 SuccessRate 필드 추가 시 사용.
	// if (FMath::FRand() > ActiveExpedition->SuccessRate)
	// {
	// 	UE_LOG(LogTemp, Log, TEXT("[Expedition] 탐사 실패 — 보상 없음"));
	// 	ActiveExpedition = nullptr;
	// 	RemainingExpeditionDays = 0;
	// 	RobotState = ESSRobotState::Idle;
	// 	OnRobotReturned.Broadcast(Result);
	// 	return;
	// }

	for (const FSSItemStack& Reward : ActiveExpedition->Rewards)
	{
		if (!IsValid(Reward.Item) || Reward.Quantity <= 0) continue;

		// TODO: 수량 범위 적용. FSSItemStackRange로 교체 시 아래처럼 사용.
		// const int32 Qty = FMath::RandRange(Reward.MinQty, Reward.MaxQty);
		// if (Qty <= 0) continue;
		// 그 아래 Existing 찾아서 Qty만큼 추가하는 코드 동일하게 사용.

		FSSItemStack* Existing = StoredItems.FindByPredicate([&Reward](const FSSItemStack& S)
		{
			return S.Item == Reward.Item;
		});

		if (Existing)
			Existing->Quantity += Reward.Quantity;
		else
			StoredItems.Add(Reward);

		Result.ReceivedItems.Add(Reward);

		UE_LOG(LogTemp, Log, TEXT("[Expedition] 보상 지급 — %s x%d"),
			*Reward.Item->ItemId.ToString(), Reward.Quantity);
	}

	LastExpeditionResult = Result;
	ActiveExpedition = nullptr;
	RemainingExpeditionDays = 0;
	RobotState = ESSRobotState::Idle;

	UE_LOG(LogTemp, Log, TEXT("[Expedition] 귀환 완료 — Day %d"), CurrentDay);

	OnRobotReturned.Broadcast(LastExpeditionResult);
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

	return true;
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
