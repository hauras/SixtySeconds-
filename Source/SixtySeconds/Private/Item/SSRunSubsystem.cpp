#include "Item/SSRunSubsystem.h"
#include "Item/SSItemDefinition.h"
#include "Item/SSExpeditionDefinition.h"
#include "Character/SSSurvivorDefinition.h"
#include "Exploration/SSExplorationTypes.h"
#include "Event/SSEventDirector.h"
#include "Comms/SSCommsState.h"
#include "Companion/SSCompanionState.h"
#include "Ara/SSAraDirector.h"
#include "Rescue/SSRescueState.h"
#include "Ending/SSEndingState.h"
#include "Item/SSExpeditionState.h"

namespace
{
	constexpr float DailySatietyLoss = 20.f;
	constexpr float DailyHydrationLoss = 25.f;
	constexpr float StarvationDamage = 10.f;
	constexpr float DehydrationDamage = 20.f;

	// 하루치 포만감·수분 감소와 굶주림·탈수 피해. 플레이어와 동료 공용.
	void ApplyDailyDecay(FSSSurvivorStats& Stats)
	{
		Stats.Satiety = FMath::Clamp(Stats.Satiety - DailySatietyLoss, 0.f, 100.f);
		Stats.Hydration = FMath::Clamp(Stats.Hydration - DailyHydrationLoss, 0.f, 100.f);

		float Damage = 0.f;
		if (Stats.Satiety <= 0.f) Damage += StarvationDamage;
		if (Stats.Hydration <= 0.f) Damage += DehydrationDamage;
		Stats.Health = FMath::Clamp(Stats.Health - Damage, 0.f, 100.f);
	}

	FText RationText(bool bGiven)
	{
		return bGiven ? NSLOCTEXT("SSJournal", "Given", "배급함") : NSLOCTEXT("SSJournal", "NotGiven", "배급 안 함");
	}
}

const FSSSurvivorState* USSRunSubsystem::FindRescuedSurvivor(FName SurvivorId) const
{
	if (SurvivorId.IsNone()) return nullptr;
	return RescuedSurvivors.FindByPredicate([SurvivorId](const FSSSurvivorState& State)
	{
		return IsValid(State.Definition) && State.Definition->SurvivorId == SurvivorId;
	});
}

FSSSurvivorState* USSRunSubsystem::FindRescuedSurvivorMutable(FName SurvivorId)
{
	return const_cast<FSSSurvivorState*>(FindRescuedSurvivor(SurvivorId));
}

void USSRunSubsystem::SetSurvivorFoodRation(FName SurvivorId, bool bGive)
{
	FSSSurvivorState* Survivor = FindRescuedSurvivorMutable(SurvivorId);
	if (Survivor && Survivor->bAlive) Survivor->bGiveFood = bGive;
}

void USSRunSubsystem::SetSurvivorWaterRation(FName SurvivorId, bool bGive)
{
	FSSSurvivorState* Survivor = FindRescuedSurvivorMutable(SurvivorId);
	if (Survivor && Survivor->bAlive) Survivor->bGiveWater = bGive;
}

bool USSRunSubsystem::HealSurvivor(FName SurvivorId)
{
	if (PlayerStats.Health <= 0.f || ActionPoints < HealActionCost)
	{
		return false;
	}

	FSSSurvivorState* Survivor = FindRescuedSurvivorMutable(SurvivorId);
	if (!Survivor || !Survivor->bAlive || Survivor->Stats.Health <= 0.f || Survivor->Stats.Health >= 100.f) return false;

	const FName MedkitItemId = SSItemIds::Medkit;
	USSItemDefinition* Medkit = FindStoredItem(MedkitItemId);

	if (!IsValid(Medkit) || Medkit->UseEffect != ESSItemUseEffect::RestoreHealth || !FMath::IsFinite(Medkit->EffectAmount) || Medkit->EffectAmount <= 0.f)
	{
		return false;
	}

	const float HealAmount = Medkit->EffectAmount;

	// 차감에 성공했을 때만 회복
	if (!ConsumeStoredItems(MedkitItemId, 1))
	{
		return false;
	}

	Survivor->Stats.Health = FMath::Clamp(
		Survivor->Stats.Health + HealAmount,
		0.f,
		100.f);

	ConsumeActionPoints(HealActionCost);
	// 동료 정보창 갱신
	OnSurvivorsChanged.Broadcast();

	return true;
}

bool USSRunSubsystem::RecruitSurvivor(USSSurvivorDefinition* Definition)
{
	if (!IsValid(Definition) || Definition->SurvivorId.IsNone() || GetHealth() <= 0.f) return false;
	for (const auto& Following : FollowingSurvivors)
		if (IsValid(Following) && Following->SurvivorId == Definition->SurvivorId) return false;
	if (FindRescuedSurvivor(Definition->SurvivorId)) return false;
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
		RecordEvent(ESSJournalEvent::Deposit, FText::Format(NSLOCTEXT("SSJournal", "SurvivorRescue", "동료 {0}명을 은신처로 데려왔다."), Count));
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

void USSRunSubsystem::AddDetailedEventJournal(const FText& Message, const FText& Title,
	const FText& Body, const FText& Choice, const FText& Outcome, const FText& Changes)
{
	FSSJournalEntry& Entry = JournalEntries.AddDefaulted_GetRef();
	Entry.Day = CurrentDay;
	Entry.Event = ESSJournalEvent::Event;
	Entry.Message = Message;
	Entry.Title = Title;
	Entry.Body = Body;
	Entry.Choice = Choice;
	Entry.Outcome = Outcome;
	Entry.Changes = Changes;
	OnJournalChanged.Broadcast();
}

void USSRunSubsystem::DepositItems(const TArray<FSSItemStack>& CarriedItems, bool bRecordJournal)
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
		if (bRecordJournal) RecordEvent(ESSJournalEvent::Deposit, FText::Format(NSLOCTEXT("SSJournal", "Deposit", "보관함에 {0} ×{1}을 옮겼다."), Incoming.Item->DisplayName, Incoming.Quantity));
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
	CapturedSurvivors.Reset();
	StoredItems.Reset();
	JournalEntries.Reset();

	CurrentDay = 1;
	LastAraReadDay = 0;
	AskedAraQuestionsMask = 0;
	AraLearningScore = 0;
	BuildAraBriefing();
	GetComms()->ResetRun();
	GetCompanions()->ResetRun();
	GetAra()->ResetRun();
	PlayerStats = FSSSurvivorStats{};

	GetExpedition()->ResetRun();
	ActionPoints = MaxActionPoints;
	GetRescue()->ResetRun();
	GetEnding()->ResetRun();
	if (IsValid(EventDirector)) EventDirector->ResetRunState(); // 카탈로그는 유지, 1회성·예약만 초기화
	OnJournalChanged.Broadcast();
	OnStoredItemsChanged.Broadcast();
	OnActionPointsChanged.Broadcast();
	OnSurvivorsChanged.Broadcast();
}

void USSRunSubsystem::BuildAraBriefing()
{
	// 아라의 문장은 아라 판단(USSAraDirector)이 대사 표에서 골라 만듦
	AraBriefing = GetAra()->BuildBriefing();
}

bool USSRunSubsystem::HasAskedAraQuestionToday(int32 QuestionIndex) const
{
	return QuestionIndex >= 0 && QuestionIndex < 3 && (AskedAraQuestionsMask & (1u << QuestionIndex)) != 0;
}

bool USSRunSubsystem::AskAraQuestion(int32 QuestionIndex, FText& OutAnswer)
{
	if (QuestionIndex < 0 || QuestionIndex >= 3 || PlayerStats.Health <= 0.f)
		return false;

	static const FText Answers[] = {
		NSLOCTEXT("SSAra", "OutsideAnswer", "현재 외부 통로에는 경비 로봇이 순찰 중입니다. 대피실에 머무르는 것이 가장 안전합니다."),
		NSLOCTEXT("SSAra", "PatrolAnswer", "경비 로봇은 인원 보호 명령을 수행 중입니다. 허가되지 않은 이동을 제한하고 있습니다."),
		NSLOCTEXT("SSAra", "SurvivorAnswer", "다른 구역의 인원 정보는 확인 중입니다. 검증되지 않은 위치는 안내할 수 없습니다.")};
	OutAnswer = Answers[QuestionIndex];
	if (HasAskedAraQuestionToday(QuestionIndex)) return true;
	if (!ConsumeActionPoints(1)) return false;
	AskedAraQuestionsMask |= static_cast<uint8>(1u << QuestionIndex);
	++AraLearningScore;
	return true;
}

USSCommsState* USSRunSubsystem::GetComms()
{
	if (!IsValid(Comms)) Comms = NewObject<USSCommsState>(this);
	return Comms;
}

bool USSRunSubsystem::MoveSurvivorToCaptured(FName SurvivorId)
{
	const int32 Index = RescuedSurvivors.IndexOfByPredicate([SurvivorId](const FSSSurvivorState& Each)
	{
		return IsValid(Each.Definition) && Each.Definition->SurvivorId == SurvivorId;
	});
	if (Index == INDEX_NONE) return false;

	// 같은 동료가 이미 붙잡혀 있으면 한 번만 (구출할 때 한 명만 돌아오게)
	if (!IsSurvivorCaptured(SurvivorId)) CapturedSurvivors.Add(RescuedSurvivors[Index]);
	RescuedSurvivors.RemoveAt(Index);
	OnSurvivorsChanged.Broadcast();
	return true;
}

bool USSRunSubsystem::CopySurvivorToCaptured(FName SurvivorId)
{
	const FSSSurvivorState* Survivor = FindRescuedSurvivor(SurvivorId);
	if (!Survivor || IsSurvivorCaptured(SurvivorId)) return false;

	CapturedSurvivors.Add(*Survivor);
	return true;
}

bool USSRunSubsystem::IsSurvivorCaptured(FName SurvivorId) const
{
	return CapturedSurvivors.ContainsByPredicate([SurvivorId](const FSSSurvivorState& Each)
	{
		return IsValid(Each.Definition) && Each.Definition->SurvivorId == SurvivorId;
	});
}

bool USSRunSubsystem::ReleaseCapturedSurvivor(FName SurvivorId, bool& bOutReplacedAndroid)
{
	bOutReplacedAndroid = false;

	const int32 Index = CapturedSurvivors.IndexOfByPredicate([SurvivorId](const FSSSurvivorState& Each)
	{
		return IsValid(Each.Definition) && Each.Definition->SurvivorId == SurvivorId;
	});
	if (Index == INDEX_NONE) return false;

	const FSSSurvivorState Released = CapturedSurvivors[Index];
	CapturedSurvivors.RemoveAt(Index);

	// 바꿔치기였으면 은신처에 같은 얼굴의 안드로이드가 있음 → 그 자리를 진짜가 채움
	if (FSSSurvivorState* Double = FindRescuedSurvivorMutable(SurvivorId))
	{
		*Double = Released;
		bOutReplacedAndroid = true;
	}
	else
	{
		RescuedSurvivors.Add(Released);
	}

	GetCompanions()->RestoreHuman(SurvivorId);

	// 하린은 돌아오면 다음 대화에서 폐기 회의를 증언함 (숨은 진실 2)
	if (SurvivorId == SSRescueIds::Researcher() && !GetEnding()->HasHiddenTruth(SSRescueIds::TestimonyTruth()))
	{
		GetCompanions()->QueueTestimony(SurvivorId);
	}

	OnSurvivorsChanged.Broadcast();
	return true;
}

USSExpeditionState* USSRunSubsystem::GetExpedition()
{
	if (!IsValid(Expedition)) Expedition = NewObject<USSExpeditionState>(this);
	return Expedition;
}

USSRescueState* USSRunSubsystem::GetRescue()
{
	if (!IsValid(Rescue)) Rescue = NewObject<USSRescueState>(this);
	return Rescue;
}

USSEndingState* USSRunSubsystem::GetEnding()
{
	if (!IsValid(Ending)) Ending = NewObject<USSEndingState>(this);
	return Ending;
}

bool USSRunSubsystem::RemoveRescuedSurvivor(FName SurvivorId)
{
	const int32 Removed = RescuedSurvivors.RemoveAll([SurvivorId](const FSSSurvivorState& Each)
	{
		return IsValid(Each.Definition) && Each.Definition->SurvivorId == SurvivorId;
	});
	if (Removed > 0) OnSurvivorsChanged.Broadcast();
	return Removed > 0;
}

USSAraDirector* USSRunSubsystem::GetAra()
{
	if (!IsValid(Ara)) Ara = NewObject<USSAraDirector>(this);
	return Ara;
}

USSCompanionState* USSRunSubsystem::GetCompanions()
{
	if (!IsValid(Companions)) Companions = NewObject<USSCompanionState>(this);
	return Companions;
}

USSEventDirector* USSRunSubsystem::GetEventDirector()
{
	if (!IsValid(EventDirector)) EventDirector = NewObject<USSEventDirector>(this);
	return EventDirector;
}

void USSRunSubsystem::ModifyPlayerStats(float DeltaHealth, float DeltaSatiety, float DeltaHydration)
{
	if (PlayerStats.Health <= 0.f) return; // 이미 사망

	PlayerStats.Health = FMath::Clamp(PlayerStats.Health + DeltaHealth, 0.f, 100.f);
	PlayerStats.Satiety = FMath::Clamp(PlayerStats.Satiety + DeltaSatiety, 0.f, 100.f);
	PlayerStats.Hydration = FMath::Clamp(PlayerStats.Hydration + DeltaHydration, 0.f, 100.f);
	if (PlayerStats.Health <= 0.f)
		RecordEvent(ESSJournalEvent::Death, NSLOCTEXT("SSJournal", "Death", "생존자가 사망했다."));
	OnPlayerStatsChanged.Broadcast();
}

void USSRunSubsystem::ModifySurvivorsHealth(float Delta)
{
	bool bChanged = false;
	for (FSSSurvivorState& Survivor : RescuedSurvivors)
	{
		if (!Survivor.bAlive || !IsValid(Survivor.Definition)) continue;
		Survivor.Stats.Health = FMath::Clamp(Survivor.Stats.Health + Delta, 0.f, 100.f);
		bChanged = true;
		if (Survivor.Stats.Health <= 0.f)
		{
			Survivor.bAlive = false;
			RecordEvent(ESSJournalEvent::Death, FText::Format(NSLOCTEXT("SSJournal", "SurvivorDeath", "{0}이(가) 사망했다."), Survivor.Definition->DisplayName));
		}
	}
	if (bChanged) OnSurvivorsChanged.Broadcast();
}

void USSRunSubsystem::AdjustActionPoints(int32 Delta)
{
	const int32 Before = ActionPoints;
	ActionPoints = FMath::Clamp(ActionPoints + Delta, 0, MaxActionPoints);
	if (ActionPoints != Before) OnActionPointsChanged.Broadcast();
}

void USSRunSubsystem::InitializeShelterStats(float InHealth, float InSatiety, float InHydration)
{
	PlayerStats.Health = FMath::Clamp(InHealth, 0.f, 100.f);
	PlayerStats.Satiety = FMath::Clamp(InSatiety, 0.f, 100.f);
	PlayerStats.Hydration = FMath::Clamp(InHydration, 0.f, 100.f);
	BuildAraBriefing(); // 은신처 첫날 보고 (ResetRun을 거치지 않고 들어와도 비어 있지 않게)
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
	if (PlayerStats.Health <= 0.f || !IsValid(Item) || GetStoredQuantity(Item) <= 0 || !FMath::IsFinite(Item->EffectAmount) || Item->EffectAmount <= 0.f) return false;

	float* TargetStat = nullptr;
	switch (Item->UseEffect)
	{
	case ESSItemUseEffect::RestoreHealth: TargetStat = &PlayerStats.Health; break;
	case ESSItemUseEffect::RestoreSatiety: TargetStat = &PlayerStats.Satiety; break;
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

void USSRunSubsystem::GetRequiredRations(bool bGiveFood, bool bGiveWater, int32& OutFood, int32& OutWater) const
{
	OutFood = bGiveFood ? 1 : 0;
	OutWater = bGiveWater ? 1 : 0;
	for (const FSSSurvivorState& Survivor : RescuedSurvivors)
	{
		if (!Survivor.bAlive || !IsValid(Survivor.Definition)) continue;
		OutFood += Survivor.bGiveFood ? 1 : 0;
		OutWater += Survivor.bGiveWater ? 1 : 0;
	}
}

bool USSRunSubsystem::HasRationsFor(bool bGiveFood, bool bGiveWater) const
{
	int32 RequiredFood = 0;
	int32 RequiredWater = 0;
	GetRequiredRations(bGiveFood, bGiveWater, RequiredFood, RequiredWater);
	return GetStoredQuantityById(SSItemIds::Food) >= RequiredFood && GetStoredQuantityById(SSItemIds::Water) >= RequiredWater;
}

bool USSRunSubsystem::AdvanceDay(bool bGiveFood, bool bGiveWater)
{
	if (!AdvanceDayCore(bGiveFood, bGiveWater)) return false;
	OnDayAdvanced.Broadcast(); // 은신처 HUD가 날짜·스탯 갱신과 사망 확인
	return true;
}

bool USSRunSubsystem::ApplyExplorationResult(const FSSExplorationResult& Result, bool bGiveFood, bool bGiveWater)
{
	if (PlayerStats.Health <= 0.f || Result.Outcome == ESSExplorationOutcome::InProgress) return false;
	if (!HasRationsFor(bGiveFood, bGiveWater)) return false; // 출발 때 확인했지만, 그사이 바뀌었으면 아무것도 바꾸지 않음

	// 탐사는 출발한 날의 일이라 하루가 넘어가기 전에 기록
	FText Summary;
	switch (Result.Outcome)
	{
	case ESSExplorationOutcome::Returned:
		Summary = FText::Format(NSLOCTEXT("SSJournal", "ExploreReturned", "직접 탐사에서 무사히 귀환했다. ({0}턴)"), Result.TurnsUsed);
		break;
	case ESSExplorationOutcome::Caught:
		Summary = NSLOCTEXT("SSJournal", "ExploreCaught", "직접 탐사 중 경비 로봇에게 발각되어 물품을 버리고 도망쳤다.");
		break;
	default:
		Summary = NSLOCTEXT("SSJournal", "ExploreTimeOut", "직접 탐사 중 시간이 다 되어 물품을 버리고 도망쳤다.");
		break;
	}
	RecordEvent(ESSJournalEvent::Expedition, Summary);

	// 정산 순서: 하루 경과(출발 전 재고로 배급) → 입고 → 부상
	if (!AdvanceDayCore(bGiveFood, bGiveWater)) return false;

	if (Result.Outcome == ESSExplorationOutcome::Returned)
	{
		DepositItems(Result.Items);
	}
	else if (Result.Injury > 0.f && PlayerStats.Health > 0.f)
	{
		PlayerStats.Health = FMath::Clamp(PlayerStats.Health - Result.Injury, 0.f, 100.f);
		RecordEvent(ESSJournalEvent::DayEnd, FText::Format(NSLOCTEXT("SSJournal", "ExploreInjury", "도망치다 다쳐 체력이 {0} 감소했다."), FMath::RoundToInt(Result.Injury)));
		if (PlayerStats.Health <= 0.f)
			RecordEvent(ESSJournalEvent::Death, NSLOCTEXT("SSJournal", "Death", "생존자가 사망했다."));
	}

	OnDayAdvanced.Broadcast();
	return true;
}

bool USSRunSubsystem::AdvanceDayCore(bool bGiveFood, bool bGiveWater)
{
	if (PlayerStats.Health <= 0.f) return false;

	const FName FoodItemId = SSItemIds::Food;
	const FName WaterItemId = SSItemIds::Water;

	USSItemDefinition* Food = FindStoredItem(FoodItemId);
	USSItemDefinition* Water = FindStoredItem(WaterItemId);

	const auto IsRationValid = [](const USSItemDefinition* Item, ESSItemUseEffect Effect)
	{
		return IsValid(Item) && Item->UseEffect == Effect && FMath::IsFinite(Item->EffectAmount) && Item->EffectAmount > 0.f;
	};

	// 플레이어와 동료의 전체 배급 필요량
	int32 RequiredFood = 0;
	int32 RequiredWater = 0;
	GetRequiredRations(bGiveFood, bGiveWater, RequiredFood, RequiredWater);

	// 배급 아이템 데이터 검사
	if (RequiredFood > 0 && !IsRationValid(Food, ESSItemUseEffect::RestoreSatiety))
	{
		return false;
	}

	if (RequiredWater > 0 && !IsRationValid(Water, ESSItemUseEffect::RestoreHydration))
	{
		return false;
	}

	// 전체 배급량이 부족하면 아무것도 변경하지 않고 중단
	if (GetStoredQuantityById(FoodItemId) < RequiredFood || GetStoredQuantityById(WaterItemId) < RequiredWater)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Ration] Insufficient supplies. Required: Food %d, Water %d"),
			RequiredFood, RequiredWater);

		return false;
	}

	const float HealthBeforeDay = PlayerStats.Health;
	if (bGiveFood) ApplyItemEffect(Food, true);
	if (bGiveWater) ApplyItemEffect(Water, true);

	ApplyDailyDecay(PlayerStats);

	RecordEvent(ESSJournalEvent::Rations, FText::Format(NSLOCTEXT("SSJournal", "Rations", "오늘 배급 — 식량: {0} / 물: {1}"), RationText(bGiveFood), RationText(bGiveWater)));

	RecordEvent(ESSJournalEvent::DayEnd, FText::Format(NSLOCTEXT("SSJournal", "DayEnd", "하루 종료 — 체력 {0} / 포만감 {1} / 수분 {2}"), FMath::RoundToInt(PlayerStats.Health), FMath::RoundToInt(PlayerStats.Satiety), FMath::RoundToInt(PlayerStats.Hydration)));

	if (PlayerStats.Health < HealthBeforeDay)
		RecordEvent(ESSJournalEvent::DayEnd, FText::Format(NSLOCTEXT("SSJournal", "Damage", "굶주림·탈수로 체력이 {0} 감소했다."), FMath::RoundToInt(HealthBeforeDay - PlayerStats.Health)));

	if (PlayerStats.Health <= 0.f)
		RecordEvent(ESSJournalEvent::Death, NSLOCTEXT("SSJournal", "Death", "생존자가 사망했다."));

	// 동료 배급 및 스탯 처리
	for (FSSSurvivorState& Survivor : RescuedSurvivors)
	{
		if (!Survivor.bAlive || !IsValid(Survivor.Definition)) continue;

		if (Survivor.bGiveFood && IsRationValid(Food, ESSItemUseEffect::RestoreSatiety) && ConsumeStoredItems(FoodItemId, 1))
		{
			Survivor.Stats.Satiety = FMath::Clamp(Survivor.Stats.Satiety + Food->EffectAmount, 0.f, 100.f);
		}
		if (Survivor.bGiveWater && IsRationValid(Water, ESSItemUseEffect::RestoreHydration) && ConsumeStoredItems(WaterItemId, 1))
		{
			Survivor.Stats.Hydration = FMath::Clamp(Survivor.Stats.Hydration + Water->EffectAmount, 0.f, 100.f);
		}

		RecordEvent(ESSJournalEvent::Rations, FText::Format(NSLOCTEXT("SSJournal", "SurvivorRations", "{0} 배급 — 식량: {1} / 물: {2}"), Survivor.Definition->DisplayName, RationText(Survivor.bGiveFood), RationText(Survivor.bGiveWater)));

		ApplyDailyDecay(Survivor.Stats);

		if (Survivor.Stats.Health <= 0.f)
		{
			Survivor.bAlive = false;
			RecordEvent(ESSJournalEvent::Death, FText::Format(NSLOCTEXT("SSJournal", "SurvivorDeath", "{0}이(가) 사망했다."), Survivor.Definition->DisplayName));
		}
	}

	// 밤: 아라의 기억이 조금 흐려진 뒤, 동료 조사(아라는 들킨 것만 봄), 표적 판단
	// (보고는 다음 날 대화로 들음)
	GetAra()->BeginNight();
	GetCompanions()->RunNight();
	GetAra()->UpdateTarget();

	// 하루 배급 처리가 끝났으므로 다음 날 예약은 해제
	for (FSSSurvivorState& Survivor : RescuedSurvivors)
	{
		Survivor.bGiveFood = false;
		Survivor.bGiveWater = false;
	}

	OnSurvivorsChanged.Broadcast();

	++CurrentDay;
	AskedAraQuestionsMask = 0;

	// 사망했으면 탐사 진행 없이 종료
	if (PlayerStats.Health <= 0.f) return true;

	ActionPoints = MaxActionPoints;
	OnActionPointsChanged.Broadcast();

	GetExpedition()->TickDay();

	// 하룻밤 지나면 적의 기억이 흐려짐 (외부 통신)
	GetComms()->OnNewDay();

	BuildAraBriefing(); // 로봇 귀환까지 반영된 아침 상태로 보고를 고정

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
