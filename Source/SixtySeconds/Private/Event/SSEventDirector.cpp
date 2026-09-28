#include "Event/SSEventDirector.h"
#include "Event/SSEventCatalog.h"
#include "Item/SSRunSubsystem.h"
#include "Item/SSItemDefinition.h"
#include "Character/SSSurvivorDefinition.h"
#include "Engine/DataTable.h"

bool USSEventDirector::SetCatalog(USSEventCatalog* InCatalog)
{
	Catalog = nullptr;
	ChoicesByEvent.Reset();
	EffectsByChoice.Reset();
	if (!IsValid(InCatalog)) return false;

	TArray<FText> Errors;
	if (!InCatalog->Validate(Errors))
	{
		for (const FText& Error : Errors) UE_LOG(LogTemp, Warning, TEXT("[Event] %s"), *Error.ToString());
		return false;
	}
	Catalog = InCatalog;

	// 선택지를 사건별로 모아 Order 순으로 정렬
	TMap<FName, TArray<TPair<int32, FName>>> Ordered;
	for (const TPair<FName, uint8*>& Pair : Catalog->ChoiceTable->GetRowMap())
	{
		const FSSEventChoiceRow& Row = *reinterpret_cast<const FSSEventChoiceRow*>(Pair.Value);
		Ordered.FindOrAdd(Row.EventId).Emplace(Row.Order, Pair.Key);
	}
	for (TPair<FName, TArray<TPair<int32, FName>>>& Pair : Ordered)
	{
		Pair.Value.StableSort([](const TPair<int32, FName>& A, const TPair<int32, FName>& B) { return A.Key < B.Key; });
		TArray<FName>& Choices = ChoicesByEvent.FindOrAdd(Pair.Key);
		for (const TPair<int32, FName>& Entry : Pair.Value) Choices.Add(Entry.Value);
	}

	for (const TPair<FName, uint8*>& Pair : Catalog->EffectTable->GetRowMap())
	{
		const FSSEventEffectRow& Row = *reinterpret_cast<const FSSEventEffectRow*>(Pair.Value);
		EffectsByChoice.FindOrAdd(Row.ChoiceId).Add(Row);
	}
	return true;
}

const FSSEventRow* USSEventDirector::FindEvent(FName EventId) const
{
	if (!IsValid(Catalog) || EventId.IsNone()) return nullptr;
	return reinterpret_cast<const FSSEventRow*>(Catalog->EventTable->FindRowUnchecked(EventId));
}

bool USSEventDirector::CheckCondition(ESSEventCondition Condition, FName Target, int32 Amount, const USSRunSubsystem& Run)
{
	switch (Condition)
	{
	case ESSEventCondition::None:
		return true;
	case ESSEventCondition::HasItem:
		return Run.GetStoredQuantityById(Target) >= FMath::Max(1, Amount);
	case ESSEventCondition::AnySurvivorAlive:
		return Run.GetRescuedSurvivors().ContainsByPredicate([](const FSSSurvivorState& S) { return S.bAlive; });
	case ESSEventCondition::SurvivorAlive:
	{
		const FSSSurvivorState* Survivor = Run.FindRescuedSurvivor(Target);
		return Survivor && Survivor->bAlive;
	}
	case ESSEventCondition::RobotIdle:
		return Run.GetRobotState() == ESSRobotState::Idle;
	case ESSEventCondition::RobotAway:
		return Run.GetRobotState() == ESSRobotState::Exploring;
	}
	return false;
}

bool USSEventDirector::HasAvailableChoice(FName EventId, const USSRunSubsystem& Run) const
{
	for (const FSSEventChoiceView& Choice : GetChoices(EventId, Run))
	{
		if (Choice.bAvailable) return true;
	}
	return false;
}

bool USSEventDirector::IsEligible(FName EventId, const FSSEventRow& Row, const USSRunSubsystem& Run) const
{
	const int32 Day = Run.GetCurrentDay();
	if (Row.bScheduledOnly || Row.Weight <= 0) return false;
	if (Day < Row.MinDay || (Row.MaxDay != 0 && Day > Row.MaxDay)) return false;
	if (Row.bOnceOnly && FiredOnce.Contains(EventId)) return false;
	if (const int32* Last = LastFiredDay.Find(EventId); Last && Row.Cooldown > 0 && Day - *Last <= Row.Cooldown) return false;
	if (!CheckCondition(Row.Condition, Row.ConditionTarget, Row.ConditionAmount, Run)) return false;
	return HasAvailableChoice(EventId, Run);   // 고를 수 있는 선택지가 하나도 없는 사건은 띄우지 않음
}

FName USSEventDirector::PickEventForToday(const USSRunSubsystem& Run)
{
	if (!IsValid(Catalog)) return NAME_None;
	const int32 Day = Run.GetCurrentDay();

	FName Picked = NAME_None;

	// 1. 예약된 사건이 먼저 (가장 오래된 것부터, 확률과 상관없이)
	for (int32 i = 0; i < Scheduled.Num(); ++i)
	{
		if (Scheduled[i].Day <= Day && FindEvent(Scheduled[i].EventId) && HasAvailableChoice(Scheduled[i].EventId, Run))
		{
			Picked = Scheduled[i].EventId;
			Scheduled.RemoveAt(i);
			break;
		}
	}

	// 2. 없으면 오늘 사건이 일어날지 굴리고, 가중치로 하나 뽑기
	if (Picked.IsNone() && Random.FRand() < Catalog->DailyEventChance)
	{
		TArray<TPair<FName, int32>> Pool;
		int32 TotalWeight = 0;
		for (const TPair<FName, uint8*>& Pair : Catalog->EventTable->GetRowMap())
		{
			const FSSEventRow& Row = *reinterpret_cast<const FSSEventRow*>(Pair.Value);
			if (!IsEligible(Pair.Key, Row, Run)) continue;
			Pool.Emplace(Pair.Key, Row.Weight);
			TotalWeight += Row.Weight;
		}
		if (TotalWeight > 0)
		{
			int32 Roll = Random.RandRange(1, TotalWeight);
			for (const TPair<FName, int32>& Entry : Pool)
			{
				Roll -= Entry.Value;
				if (Roll <= 0) { Picked = Entry.Key; break; }
			}
		}
	}

	// 나온 순간 기록 (선택하지 않고 창을 닫아도 1회성·쿨다운 유지)
	if (!Picked.IsNone())
	{
		LastFiredDay.Add(Picked, Day);
		if (const FSSEventRow* Row = FindEvent(Picked); Row && Row->bOnceOnly) FiredOnce.Add(Picked);
	}
	return Picked;
}

TArray<FSSEventChoiceView> USSEventDirector::GetChoices(FName EventId, const USSRunSubsystem& Run) const
{
	TArray<FSSEventChoiceView> Views;
	if (!IsValid(Catalog)) return Views;
	const TArray<FName>* ChoiceIds = ChoicesByEvent.Find(EventId);
	if (!ChoiceIds) return Views;

	for (const FName ChoiceId : *ChoiceIds)
	{
		const FSSEventChoiceRow* Row = reinterpret_cast<const FSSEventChoiceRow*>(Catalog->ChoiceTable->FindRowUnchecked(ChoiceId));
		if (!Row) continue;
		FSSEventChoiceView& View = Views.AddDefaulted_GetRef();
		View.ChoiceId = ChoiceId;
		View.Text = Row->Text;
		View.bAvailable = CheckCondition(Row->Condition, Row->ConditionTarget, Row->ConditionAmount, Run);
	}
	return Views;
}

bool USSEventDirector::ApplyChoice(FName EventId, FName ChoiceId, USSRunSubsystem& Run)
{
	// 규칙은 여기서 다시 확인: 화면이 잘못된 선택지를 넘겨도 적용되지 않게
	const bool bValid = GetChoices(EventId, Run).ContainsByPredicate([ChoiceId](const FSSEventChoiceView& View)
	{
		return View.ChoiceId == ChoiceId && View.bAvailable;
	});
	if (!bValid) return false;

	if (const TArray<FSSEventEffectRow>* Effects = EffectsByChoice.Find(ChoiceId))
	{
		for (const FSSEventEffectRow& Effect : *Effects)
		{
			if (Effect.Chance < 1.f && Random.FRand() >= Effect.Chance) continue;
			ApplyEffect(Effect, Run);
		}
	}
	return true;
}

void USSEventDirector::ApplyEffect(const FSSEventEffectRow& Effect, USSRunSubsystem& Run)
{
	switch (Effect.Type)
	{
	case ESSEventEffect::Item:
		if (Effect.Amount > 0)
		{
			if (USSItemDefinition* Item = Catalog->FindItem(Effect.Target))
			{
				FSSItemStack Stack;
				Stack.Item = Item;
				Stack.Quantity = Effect.Amount;
				Run.DepositItems({ Stack });
			}
		}
		else if (Effect.Amount < 0)
		{
			// 가진 것보다 많이 잃으라고 하면 가진 만큼만
			const int32 Lose = FMath::Min(-Effect.Amount, Run.GetStoredQuantityById(Effect.Target));
			if (Lose > 0) Run.RemoveStoredItemsById(Effect.Target, Lose);
		}
		break;
	case ESSEventEffect::PlayerHealth:
		Run.ModifyPlayerStats(Effect.Amount, 0.f, 0.f);
		break;
	case ESSEventEffect::PlayerSatiety:
		Run.ModifyPlayerStats(0.f, Effect.Amount, 0.f);
		break;
	case ESSEventEffect::PlayerHydration:
		Run.ModifyPlayerStats(0.f, 0.f, Effect.Amount);
		break;
	case ESSEventEffect::SurvivorsHealth:
		Run.ModifySurvivorsHealth(Effect.Amount);
		break;
	case ESSEventEffect::ActionPoints:
		Run.AdjustActionPoints(Effect.Amount);
		break;
	case ESSEventEffect::Journal:
		Run.AddEventJournal(Effect.Text);
		break;
	case ESSEventEffect::ScheduleEvent:
		Scheduled.Add({ Effect.Target, Run.GetCurrentDay() + FMath::Max(1, Effect.Amount) });
		break;
	default:
		break;
	}
}

void USSEventDirector::ResetRunState()
{
	Scheduled.Reset();
	FiredOnce.Reset();
	LastFiredDay.Reset();
}
