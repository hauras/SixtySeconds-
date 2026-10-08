#include "Event/SSEventDirector.h"
#include "Engine/DataTable.h"
#include "Event/SSEventCatalog.h"
#include "Item/SSRunSubsystem.h"
#include "Ending/SSEndingState.h"
#include "Item/SSItemDefinition.h"
#include "Character/SSSurvivorDefinition.h"
#include "Ara/SSAraDirector.h"

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
		Pair.Value.StableSort([](const TPair<int32, FName>& A, const TPair<int32, FName>& B)
		{
			return A.Key < B.Key;
		});
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
		return Run.GetRescuedSurvivors().ContainsByPredicate([](const FSSSurvivorState& S)
		{
			return S.bAlive;
		});
	case ESSEventCondition::SurvivorAlive:
	{
		const FSSSurvivorState* Survivor = Run.FindRescuedSurvivor(Target);
		return Survivor && Survivor->bAlive;
	}
	case ESSEventCondition::RobotIdle:
		return Run.GetRobotState() == ESSRobotState::Idle;
	case ESSEventCondition::RobotAway:
		return Run.GetRobotState() == ESSRobotState::Exploring;
	case ESSEventCondition::AraHasTarget:
	{
		// 아라가 아직 아무도 바꾸지 않았고, 살아 있는 표적이 있을 때만
		const USSAraDirector* Ara = Run.FindAra();
		return Ara && Ara->CanSwapTarget();
	}
	case ESSEventCondition::HiddenTruthCount:
		return Run.FindEnding() && Run.FindEnding()->CountHiddenTruths() >= FMath::Max(1, Amount);
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
	return HasAvailableChoice(EventId, Run); // 고를 수 있는 선택지가 하나도 없는 사건은 띄우지 않음
}

FName USSEventDirector::PickEventForToday(const USSRunSubsystem& Run)
{
	if (!IsValid(Catalog)) return NAME_None;
	const int32 Day = Run.GetCurrentDay();

	FName Picked = NAME_None;

	// 0. 마지막 밤: 서버실 사건은 확률·예약과 상관없이 한 번
	const FName FinalEvent = GetFinalEventId();
	if (Day >= USSEndingState::FinalDay && !FiredOnce.Contains(FinalEvent) && FindEvent(FinalEvent) && HasAvailableChoice(FinalEvent, Run))
	{
		FiredOnce.Add(FinalEvent);
		LastFiredDay.Add(FinalEvent, Day);
		return FinalEvent;
	}

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
				if (Roll <= 0)
				{
					Picked = Entry.Key;
					break;
				}
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
		View.Text = FillText(Row->Text, Run);
		View.bAvailable = CheckCondition(Row->Condition, Row->ConditionTarget, Row->ConditionAmount, Run);
	}
	return Views;
}

bool USSEventDirector::ApplyChoice(FName EventId, FName ChoiceId, USSRunSubsystem& Run, FSSEventResult& OutResult)
{
	const TArray<FSSEventChoiceView> Choices = GetChoices(EventId, Run);
	const FSSEventChoiceView* Chosen = Choices.FindByPredicate([ChoiceId](const FSSEventChoiceView& View)
	{
		return View.ChoiceId == ChoiceId;
	});

	if (!Chosen || !Chosen->bAvailable) return false;

	// {Target} 이름은 효과 적용 전에 정해 둠 (교체 효과가 표적을 지운 뒤에도 같은 이름으로 기록)
	const FFormatNamedArguments TextArgs = MakeTextArgs(Run);

	OutResult = FSSEventResult();
	OutResult.EventId = EventId;
	if (const FSSEventRow* Event = FindEvent(EventId)) OutResult.Title = FText::Format(Event->Title, TextArgs);
	OutResult.ChoiceText = Chosen->Text;

	if (const TArray<FSSEventEffectRow>* Effects = EffectsByChoice.Find(ChoiceId))
	{
		for (const FSSEventEffectRow& Effect : *Effects)
		{
			if (Effect.Chance < 1.f && Random.FRand() >= Effect.Chance) continue;

			ApplyEffect(Effect, Run, OutResult);
		}
	}

	// 결과 문장의 {Target}도 같은 이름으로 채움
	for (FText& Line : OutResult.Lines)
	{
		Line = FText::Format(Line, TextArgs);
	}

	// 저널에 사건 한 줄: "지난 밤 · 제목 — 선택. 결과 문장 (변화)"
	FText Line = FText::Format(NSLOCTEXT("SSEvent", "JournalHead", "지난 밤 · {0} — {1}."), OutResult.Title, OutResult.ChoiceText);
	if (OutResult.Lines.Num() > 0) // 결과 문장이 있으면 이어 붙임
		Line = FText::Format(NSLOCTEXT("SSEvent", "JournalLines", "{0} {1}"), Line, FText::Join(FText::FromString(TEXT(" ")), OutResult.Lines));
	const FText Changes = DescribeChanges(OutResult);
	if (!Changes.IsEmpty()) // 실제로 바뀐 게 있으면 괄호로
		Line = FText::Format(NSLOCTEXT("SSEvent", "JournalChanges", "{0} ({1})"), Line, Changes);
	const FSSEventRow* RecordedEvent = FindEvent(EventId);
	Run.AddDetailedEventJournal(Line, OutResult.Title, RecordedEvent ? FText::Format(RecordedEvent->Body, TextArgs) : FText::GetEmpty(),
		OutResult.ChoiceText, FText::Join(FText::FromString(TEXT(" ")), OutResult.Lines), Changes);

	return true;
}

FText USSEventDirector::DescribeChange(const FSSEventChange& Change) const
{
	FText Label;
	switch (Change.Type)
	{
	case ESSEventEffect::Item:
	{
		const USSItemDefinition* Item = IsValid(Catalog) ? Catalog->FindItem(Change.Target) : nullptr;
		// 카탈로그에 이름이 없으면 아이템 식별자를 쓴다.
		Label = Item ? Item->DisplayName : FText::FromName(Change.Target);
		break;
	}
	case ESSEventEffect::PlayerHealth:
		Label = NSLOCTEXT("SSEvent", "ChangeHealth", "체력");
		break;
	case ESSEventEffect::PlayerSatiety:
		Label = NSLOCTEXT("SSEvent", "ChangeSatiety", "포만감");
		break;
	case ESSEventEffect::PlayerHydration:
		Label = NSLOCTEXT("SSEvent", "ChangeHydration", "수분");
		break;
	case ESSEventEffect::SurvivorsHealth:
		Label = NSLOCTEXT("SSEvent", "ChangeSurvivors", "동료 체력");
		break;
	case ESSEventEffect::ActionPoints:
		Label = NSLOCTEXT("SSEvent", "ChangeAP", "행동력");
		break;
	default:
		return FText::GetEmpty();
	}

	// 수량은 기존 기록과 같이 항상 부호를 붙인다.
	const FText Amount = FText::FromString(FString::Printf(TEXT("%+d"), Change.Amount));
	return FText::Format(
		NSLOCTEXT("SSEvent", "ChangeEntry", "{0} {1}"),
		Label,
		Amount);
}

FText USSEventDirector::DescribeChanges(const FSSEventResult& Result) const
{
	TArray<FText> Parts;
	for (const FSSEventChange& Change : Result.Changes)
	{
		const FText Part = DescribeChange(Change);
		if (!Part.IsEmpty()) Parts.Add(Part);
	}
	return FText::Join(NSLOCTEXT("SSEvent", "ChangeSeparator", ", "), Parts);
}
void USSEventDirector::ApplyEffect(const FSSEventEffectRow& Effect, USSRunSubsystem& Run, FSSEventResult& OutResult)
{
	// 실제로 바뀐 양이 있을 때만 결과에 한 줄 추가 (0이면 "아무 일 없음"이라 안 남김)
	const auto AddChange = [&OutResult, &Effect](int32 Amount)
	{
		if (Amount != 0) OutResult.Changes.Add({Effect.Type, Effect.Target, Amount});
	};

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
				Run.DepositItems({Stack});
				AddChange(Effect.Amount); // 얻은 만큼
			}
		}
		else if (Effect.Amount < 0)
		{
			// 가진 것보다 많이 잃으라고 하면 가진 만큼만
			const int32 Lose = FMath::Min(-Effect.Amount, Run.GetStoredQuantityById(Effect.Target));
			if (Lose > 0 && Run.RemoveStoredItemsById(Effect.Target, Lose))
				AddChange(-Lose); // CSV 값이 아니라 실제로 잃은 양
		}
		break;
	case ESSEventEffect::PlayerHealth:
	{
		const float Before = Run.GetHealth();
		Run.ModifyPlayerStats(Effect.Amount, 0.f, 0.f);
		AddChange(FMath::RoundToInt(Run.GetHealth() - Before)); // 0~100에서 잘린 뒤의 실제 변화
		break;
	}
	case ESSEventEffect::PlayerSatiety:
	{
		const float Before = Run.GetSatiety();
		Run.ModifyPlayerStats(0.f, Effect.Amount, 0.f);
		AddChange(FMath::RoundToInt(Run.GetSatiety() - Before));
		break;
	}
	case ESSEventEffect::PlayerHydration:
	{
		const float Before = Run.GetHydration();
		Run.ModifyPlayerStats(0.f, 0.f, Effect.Amount);
		AddChange(FMath::RoundToInt(Run.GetHydration() - Before));
		break;
	}
	case ESSEventEffect::SurvivorsHealth:
		Run.ModifySurvivorsHealth(Effect.Amount);
		AddChange(Effect.Amount); // 동료마다 다를 수 있어서 적힌 값으로 표시
		break;
	case ESSEventEffect::ActionPoints:
	{
		const int32 Before = Run.GetActionPoints();
		Run.AdjustActionPoints(Effect.Amount);
		AddChange(Run.GetActionPoints() - Before); // 행동력이 0이었으면 -2라도 실제 변화는 0
		break;
	}
	case ESSEventEffect::Journal:
		OutResult.Lines.Add(Effect.Text); // 저널에 바로 쓰지 않음. 결과로 모아서 한 번에 기록 (4단계)
		break;
	case ESSEventEffect::ScheduleEvent:
		Scheduled.Add({Effect.Target, Run.GetCurrentDay() + FMath::Max(1, Effect.Amount)});
		break; // 다음 날 일어날 일은 결과에 안 넣음 (복선)
	case ESSEventEffect::AraSwapTarget:
		// 비밀: 결과(Changes)에 넣지 않음 → 화면 칩·기록에 안 나옴
		Run.GetAra()->SwapTarget();
		break;
	case ESSEventEffect::AraRefused:
		Run.GetAra()->OnOfferRefused();
		break;
	case ESSEventEffect::Ending:
	{
		// 비밀: 결과(Changes)에 넣지 않음. 밤이 끝나면 HUD가 엔딩 카드를 띄움
		const int64 Value = StaticEnum<ESSEnding>()->GetValueByNameString(Effect.Target.ToString());
		if (Value != INDEX_NONE) Run.GetEnding()->ReachEnding(static_cast<ESSEnding>(Value));
		break;
	}
	default:
		break;
	}
}

void USSEventDirector::ApplyStandaloneEffect(const FSSEventEffectRow& Effect, USSRunSubsystem& Run, FSSEventResult& OutResult)
{
	// 아이템은 카탈로그에서 에셋을 찾아야 해서, 카탈로그가 없으면 적용하지 않음
	if (Effect.Type == ESSEventEffect::Item && !IsValid(Catalog)) return;
	ApplyEffect(Effect, Run, OutResult);
}

void USSEventDirector::ScheduleEvent(FName InEventId, int32 Day)
{
	if (InEventId.IsNone()) return;
	Scheduled.Add({InEventId, Day});
}

void USSEventDirector::CancelScheduled(FName InEventId)
{
	Scheduled.RemoveAll([InEventId](const FScheduled& Each)
	{
		return Each.EventId == InEventId;
	});
}

int32 USSEventDirector::FindScheduledDay(FName InEventId) const
{
	int32 Earliest = -1;
	for (const FScheduled& Each : Scheduled)
	{
		if (Each.EventId != InEventId) continue;
		if (Earliest < 0 || Each.Day < Earliest) Earliest = Each.Day;
	}
	return Earliest;
}

FFormatNamedArguments USSEventDirector::MakeTextArgs(const USSRunSubsystem& Run)
{
	// 표적이 없으면 누구인지 드러나지 않게 "동료"
	FText TargetName = NSLOCTEXT("SSEvent", "UnknownTarget", "동료");
	const USSAraDirector* Ara = Run.FindAra();
	if (Ara && Ara->HasTarget())
	{
		const FSSSurvivorState* Survivor = Run.FindRescuedSurvivor(Ara->GetTarget());
		if (Survivor && IsValid(Survivor->Definition)) TargetName = Survivor->Definition->DisplayName;
	}

	FFormatNamedArguments Args;
	Args.Add(TEXT("Target"), TargetName);
	return Args;
}

FText USSEventDirector::FillText(const FText& Text, const USSRunSubsystem& Run)
{
	return FText::Format(Text, MakeTextArgs(Run));
}

void USSEventDirector::ResetRunState()
{
	Scheduled.Reset();
	FiredOnce.Reset();
	LastFiredDay.Reset();
}
