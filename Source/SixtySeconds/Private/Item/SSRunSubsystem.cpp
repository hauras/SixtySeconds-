#include "Item/SSRunSubsystem.h"
#include "Item/SSItemDefinition.h"
#include "Item/SSExpeditionDefinition.h"
#include "Character/SSSurvivorDefinition.h"

bool USSRunSubsystem::IsSurvivorRescued(FName SurvivorId) const
{
    if (SurvivorId.IsNone()) return false;
    return RescuedSurvivors.ContainsByPredicate([SurvivorId](const FSSSurvivorState& State)
    {
        return IsValid(State.Definition) && State.Definition->SurvivorId == SurvivorId;
    });
}

bool USSRunSubsystem::RecruitSurvivor(USSSurvivorDefinition* Definition)
{
    if (!IsValid(Definition) || Definition->SurvivorId.IsNone() || GetHealth() <= 0.f) return false;
    for (const auto& Following : FollowingSurvivors)
        if (IsValid(Following) && Following->SurvivorId == Definition->SurvivorId) return false;
    for (const FSSSurvivorState& Rescued : RescuedSurvivors)
        if (IsValid(Rescued.Definition) && Rescued.Definition->SurvivorId == Definition->SurvivorId) return false;
    FollowingSurvivors.Add(Definition);
    OnSurvivorsChanged.Broadcast();
    UE_LOG(LogTemp, Log, TEXT("[Survivor] Following: %s"), *Definition->SurvivorId.ToString());
    return true;
}

int32 USSRunSubsystem::RescueFollowingSurvivors()
{
    int32 Count = 0;
    for (USSSurvivorDefinition* Definition : FollowingSurvivors)
    {
        if (!IsValid(Definition)) continue;
        FSSSurvivorState& State = RescuedSurvivors.AddDefaulted_GetRef();
        State.Definition = Definition;
        State.Stats = Definition->InitialStats;
        ++Count;
        UE_LOG(LogTemp, Log, TEXT("[Survivor] Rescued: %s"), *Definition->SurvivorId.ToString());
    }
    FollowingSurvivors.Reset();
    if (Count > 0)
    {
        RecordEvent(ESSJournalEvent::Deposit, FText::Format(
            NSLOCTEXT("SSJournal", "SurvivorRescue", "동료 {0}명을 은신처로 데려왔다."), Count));
        OnSurvivorsChanged.Broadcast();
    }
    return Count;
}

void USSRunSubsystem::RecordEvent(ESSJournalEvent Event, const FText& Message)
{
	FSSJournalEntry& Entry = JournalEntries.AddDefaulted_GetRef();
	Entry.Day = CurrentDay;
	Entry.Event = Event;
	Entry.Message = Message;
	OnJournalChanged.Broadcast();
}

void USSRunSubsystem::RecordExpeditionReturn(const FSSExpeditionResult& Result, bool bSuccess)
{
	RecordEvent(ESSJournalEvent::Expedition, FText::Format(
		bSuccess ? NSLOCTEXT("SSJournal", "ReturnSuccess", "{0} 탐사에서 귀환했다.")
		: NSLOCTEXT("SSJournal", "ReturnFailure", "{0} 탐사에 실패하여 물자 없이 귀환했다."), Result.RegionName));
	for (const FSSItemStack& Stack : Result.ReceivedItems)
		if (IsValid(Stack.Item) && Stack.Quantity > 0)
			RecordEvent(ESSJournalEvent::Expedition, FText::Format(NSLOCTEXT("SSJournal", "Reward", "획득: {0} ×{1}"), Stack.Item->DisplayName, Stack.Quantity));
	if (bSuccess && Result.ReceivedItems.IsEmpty())
		RecordEvent(ESSJournalEvent::Expedition, NSLOCTEXT("SSJournal", "NoReward", "가져온 물자는 없다."));
	if (RobotState == ESSRobotState::Broken)
		RecordEvent(ESSJournalEvent::Robot, NSLOCTEXT("SSJournal", "Broken", "귀환한 탐사로봇에 고장이 발생했다."));
}

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
		RecordEvent(ESSJournalEvent::Deposit, FText::Format(NSLOCTEXT("SSJournal", "Deposit", "보관함에 {0} ×{1}을 옮겼다."), Incoming.Item->DisplayName, Incoming.Quantity));
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
    FollowingSurvivors.Reset();
    RescuedSurvivors.Reset();
	StoredItems.Reset();
	JournalEntries.Reset();

	CurrentDay = 1;
	PlayerStats = FSSSurvivorStats{};

	RobotState = ESSRobotState::Idle;
	ActiveExpedition = nullptr;
	RemainingExpeditionDays = 0;
	RepairDaysRemaining = 0;
	LastExpeditionResult = FSSExpeditionResult{};
	ActionPoints = MaxActionPoints;
	OnJournalChanged.Broadcast();
	OnRobotStateChanged.Broadcast();
	OnStoredItemsChanged.Broadcast();
	OnActionPointsChanged.Broadcast();
    OnSurvivorsChanged.Broadcast();
}

void USSRunSubsystem::InitializeShelterStats(float InHealth, float InSatiety, float InHydration)
{
	PlayerStats.Health = FMath::Clamp(InHealth, 0.f, 100.f);
	PlayerStats.Satiety = FMath::Clamp(InSatiety, 0.f, 100.f);
	PlayerStats.Hydration = FMath::Clamp(InHydration, 0.f, 100.f);
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
	USSItemDefinition* UsedItem = FindStoredItem(ItemId);
	if (!ApplyItemEffect(UsedItem, false)) return false;
	RecordEvent(ESSJournalEvent::ItemUse, FText::Format(NSLOCTEXT("SSJournal", "UseItem", "{0}을 사용했다."), UsedItem->DisplayName));
	return true;
}

bool USSRunSubsystem::ApplyItemEffect(USSItemDefinition* Item, bool bAllowFullStat)
{
	if (PlayerStats.Health <= 0.f || !IsValid(Item) || GetStoredQuantity(Item) <= 0
		|| !FMath::IsFinite(Item->EffectAmount) || Item->EffectAmount <= 0.f) return false;

	float* TargetStat = nullptr;
	switch (Item->UseEffect)
	{
	case ESSItemUseEffect::RestoreHealth:    TargetStat = &PlayerStats.Health;    break;
	case ESSItemUseEffect::RestoreSatiety:   TargetStat = &PlayerStats.Satiety;   break;
	case ESSItemUseEffect::RestoreHydration: TargetStat = &PlayerStats.Hydration; break;
	default: return false;
	}
	if (!bAllowFullStat && *TargetStat >= 100.f) return false;
	if (Item->bConsumeOnUse && !ConsumeItem(Item->ItemId)) return false;
	*TargetStat = FMath::Clamp(*TargetStat + Item->EffectAmount, 0.f, 100.f);
	return true;
}

bool USSRunSubsystem::ConsumeActionPoints(int32 Cost)
{
	if (Cost <= 0 || ActionPoints < Cost) return false;
	ActionPoints -= Cost;
	OnActionPointsChanged.Broadcast();
	return true;
}

ESSExpeditionStartResult USSRunSubsystem::StartExpedition(USSExpeditionDefinition* Expedition)
{
	if (PlayerStats.Health <= 0.f)
		return ESSExpeditionStartResult::PlayerDead;

	if (ActionPoints < ExpeditionActionCost)
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
		if (GetStoredQuantityById(Pair.Key) < Pair.Value)
			return ESSExpeditionStartResult::NotEnoughBattery;
	}

	// 검사 통과 후 비용 차감
	for (const auto& Pair : RequiredTotals)
		ConsumeStoredItems(Pair.Key, Pair.Value);

	ConsumeActionPoints(ExpeditionActionCost);

	ActiveExpedition = Expedition;
	RemainingExpeditionDays = Expedition->DurationDays;
	RobotState = ESSRobotState::Exploring;
	RecordEvent(ESSJournalEvent::Expedition, FText::Format(NSLOCTEXT("SSJournal", "Dispatch", "{0}에 로봇을 파견했다. 소요 {1}일."), Expedition->RegionName, Expedition->DurationDays));
	for (const FSSItemStack& CostStack : Expedition->Cost)
		RecordEvent(ESSJournalEvent::Expedition, FText::Format(NSLOCTEXT("SSJournal", "DispatchCost", "파견 비용: {0} ×{1}"), CostStack.Item->DisplayName, CostStack.Quantity));
	OnRobotStateChanged.Broadcast();

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
	RecordExpeditionReturn(LastExpeditionResult, true);
	OnRobotStateChanged.Broadcast();
	OnStoredItemsChanged.Broadcast();
}

bool USSRunSubsystem::AdvanceDay(bool bGiveFood, bool bGiveWater)
{
	if (PlayerStats.Health <= 0.f) return false;

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

	const float HealthBeforeDay = PlayerStats.Health;
	if (bGiveFood)  ApplyItemEffect(Food,  true);
	if (bGiveWater) ApplyItemEffect(Water, true);

	PlayerStats.Satiety   = FMath::Clamp(PlayerStats.Satiety   - 20.f, 0.f, 100.f);
	PlayerStats.Hydration = FMath::Clamp(PlayerStats.Hydration - 25.f, 0.f, 100.f);

	float Damage = 0.f;
	if (PlayerStats.Satiety   <= 0.f) Damage += 10.f;
	if (PlayerStats.Hydration <= 0.f) Damage += 20.f;

	PlayerStats.Health = FMath::Clamp(PlayerStats.Health - Damage, 0.f, 100.f);
	RecordEvent(ESSJournalEvent::Rations, FText::Format(NSLOCTEXT("SSJournal", "Rations", "오늘 배급 — 식량: {0} / 물: {1}"),
		bGiveFood ? NSLOCTEXT("SSJournal", "Given", "배급함") : NSLOCTEXT("SSJournal", "NotGiven", "배급 안 함"),
		bGiveWater ? NSLOCTEXT("SSJournal", "Given", "배급함") : NSLOCTEXT("SSJournal", "NotGiven", "배급 안 함")));
	RecordEvent(ESSJournalEvent::DayEnd, FText::Format(NSLOCTEXT("SSJournal", "DayEnd", "하루 종료 — 체력 {0} / 포만감 {1} / 수분 {2}"),
		FMath::RoundToInt(PlayerStats.Health), FMath::RoundToInt(PlayerStats.Satiety), FMath::RoundToInt(PlayerStats.Hydration)));
	if (PlayerStats.Health < HealthBeforeDay)
		RecordEvent(ESSJournalEvent::DayEnd, FText::Format(NSLOCTEXT("SSJournal", "Damage", "굶주림·탈수로 체력이 {0} 감소했다."), FMath::RoundToInt(HealthBeforeDay - PlayerStats.Health)));
	if (PlayerStats.Health <= 0.f)
		RecordEvent(ESSJournalEvent::Death, NSLOCTEXT("SSJournal", "Death", "생존자가 사망했다."));

	// 동료 배급 및 스탯 처리
	for (FSSSurvivorState& Survivor : RescuedSurvivors)
	{
		if (!Survivor.bAlive || !IsValid(Survivor.Definition)) continue;

		if (Survivor.bGiveFood && IsRationValid(Food, ESSItemUseEffect::RestoreSatiety))
		{
			ConsumeStoredItems(FoodItemId, 1);
			Survivor.Stats.Satiety = FMath::Clamp(Survivor.Stats.Satiety + Food->EffectAmount, 0.f, 100.f);
		}
		if (Survivor.bGiveWater && IsRationValid(Water, ESSItemUseEffect::RestoreHydration))
		{
			ConsumeStoredItems(WaterItemId, 1);
			Survivor.Stats.Hydration = FMath::Clamp(Survivor.Stats.Hydration + Water->EffectAmount, 0.f, 100.f);
		}

		Survivor.Stats.Satiety   = FMath::Clamp(Survivor.Stats.Satiety   - 20.f, 0.f, 100.f);
		Survivor.Stats.Hydration = FMath::Clamp(Survivor.Stats.Hydration - 25.f, 0.f, 100.f);

		float SurvivorDamage = 0.f;
		if (Survivor.Stats.Satiety   <= 0.f) SurvivorDamage += 10.f;
		if (Survivor.Stats.Hydration <= 0.f) SurvivorDamage += 20.f;
		Survivor.Stats.Health = FMath::Clamp(Survivor.Stats.Health - SurvivorDamage, 0.f, 100.f);

		if (Survivor.Stats.Health <= 0.f)
		{
			Survivor.bAlive = false;
			RecordEvent(ESSJournalEvent::Death, FText::Format(
				NSLOCTEXT("SSJournal", "SurvivorDeath", "{0}이(가) 사망했다."),
				Survivor.Definition->DisplayName));
		}
	}
	OnSurvivorsChanged.Broadcast();

	++CurrentDay;

	// 사망했으면 탐사 진행 없이 종료
	if (PlayerStats.Health <= 0.f) return true;

	ActionPoints = MaxActionPoints;
	OnActionPointsChanged.Broadcast();

	TickExpedition();
	TickRepair();

	return true;
}

bool USSRunSubsystem::RepairRobot()
{
	if (RobotState != ESSRobotState::Broken) return false;

	if (ActionPoints < RepairActionCost)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Robot] 행동력 부족 — 수리 불가"));
		return false;
	}

	static const FName RepairKitId(TEXT("RepairKit"));
	if (!ConsumeItem(RepairKitId))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Robot] 수리키트 부족 — 수리 불가"));
		return false;
	}

	ConsumeActionPoints(RepairActionCost);

	RobotState = ESSRobotState::Repairing;
	RepairDaysRemaining = 1;
	RecordEvent(ESSJournalEvent::Robot, NSLOCTEXT("SSJournal", "RepairStart", "수리키트 1개를 소모해 로봇 수리를 시작했다. 소요 1일."));
	OnRobotStateChanged.Broadcast();
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
		RecordEvent(ESSJournalEvent::Robot, NSLOCTEXT("SSJournal", "RepairDone", "탐사로봇 수리가 완료되었다."));
		OnRobotStateChanged.Broadcast();
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
