#include "Event/SSEventCatalog.h"
#include "Event/SSEventTypes.h"
#include "Item/SSItemDefinition.h"
#include "Engine/DataTable.h"
#include "Misc/DataValidation.h"

#define LOCTEXT_NAMESPACE "SSEventCatalog"

USSItemDefinition* USSEventCatalog::FindItem(FName ItemId) const
{
	if (ItemId.IsNone()) return nullptr;
	for (USSItemDefinition* Item : Items)
	{
		if (IsValid(Item) && Item->ItemId == ItemId) return Item;
	}
	return nullptr;
}

bool USSEventCatalog::Validate(TArray<FText>& OutErrors) const
{
	const int32 ErrorsBefore = OutErrors.Num();

	const auto CheckTable = [&OutErrors](const UDataTable* Table, const UScriptStruct* Expected, const TCHAR* Name)
	{
		if (!Table)
		{
			OutErrors.Add(FText::Format(LOCTEXT("MissingTable", "{0} 테이블이 지정되지 않았습니다."), FText::FromString(Name)));
			return false;
		}
		if (Table->GetRowStruct() != Expected)
		{
			OutErrors.Add(FText::Format(LOCTEXT("WrongStruct", "{0} 테이블의 행 구조가 {1}이(가) 아닙니다."),
				FText::FromString(Name), FText::FromString(Expected->GetName())));
			return false;
		}
		return true;
	};
	const bool bTablesOk = CheckTable(EventTable, FSSEventRow::StaticStruct(), TEXT("Event")) & CheckTable(ChoiceTable, FSSEventChoiceRow::StaticStruct(), TEXT("Choice")) & CheckTable(EffectTable, FSSEventEffectRow::StaticStruct(), TEXT("Effect"));
	if (!bTablesOk) return false;

	const auto CheckItemRef = [this, &OutErrors](FName ItemId, const FName& RowName)
	{
		if (!FindItem(ItemId))
			OutErrors.Add(FText::Format(LOCTEXT("UnknownItem", "'{0}' 행의 아이템 '{1}'이(가) Items 목록에 없습니다."),
				FText::FromName(RowName), FText::FromName(ItemId)));
	};

	// 사건
	TSet<FName> EventsWithChoice;
	for (const TPair<FName, uint8*>& Pair : EventTable->GetRowMap())
	{
		const FSSEventRow& Row = *reinterpret_cast<const FSSEventRow*>(Pair.Value);
		if (Row.MinDay < 1 || (Row.MaxDay != 0 && Row.MaxDay < Row.MinDay) || Row.Weight < 0 || Row.Cooldown < 0)
			OutErrors.Add(FText::Format(LOCTEXT("BadEventNumbers", "사건 '{0}'의 MinDay·MaxDay·Weight·Cooldown 값이 잘못됐습니다."), FText::FromName(Pair.Key)));
		if (Row.Condition == ESSEventCondition::HasItem) CheckItemRef(Row.ConditionTarget, Pair.Key);
	}

	// 선택지
	for (const TPair<FName, uint8*>& Pair : ChoiceTable->GetRowMap())
	{
		const FSSEventChoiceRow& Row = *reinterpret_cast<const FSSEventChoiceRow*>(Pair.Value);
		if (!EventTable->FindRowUnchecked(Row.EventId))
			OutErrors.Add(FText::Format(LOCTEXT("ChoiceNoEvent", "선택지 '{0}'가 없는 사건 '{1}'을(를) 가리킵니다."),
				FText::FromName(Pair.Key), FText::FromName(Row.EventId)));
		else
			EventsWithChoice.Add(Row.EventId);
		if (Row.Condition == ESSEventCondition::HasItem) CheckItemRef(Row.ConditionTarget, Pair.Key);
	}
	for (const TPair<FName, uint8*>& Pair : EventTable->GetRowMap())
	{
		if (!EventsWithChoice.Contains(Pair.Key))
			OutErrors.Add(FText::Format(LOCTEXT("EventNoChoice", "사건 '{0}'에 선택지가 없습니다."), FText::FromName(Pair.Key)));
	}

	// 효과
	for (const TPair<FName, uint8*>& Pair : EffectTable->GetRowMap())
	{
		const FSSEventEffectRow& Row = *reinterpret_cast<const FSSEventEffectRow*>(Pair.Value);
		if (!ChoiceTable->FindRowUnchecked(Row.ChoiceId))
			OutErrors.Add(FText::Format(LOCTEXT("EffectNoChoice", "효과 '{0}'가 없는 선택지 '{1}'을(를) 가리킵니다."),
				FText::FromName(Pair.Key), FText::FromName(Row.ChoiceId)));
		if (!FMath::IsFinite(Row.Chance) || Row.Chance < 0.f || Row.Chance > 1.f)
			OutErrors.Add(FText::Format(LOCTEXT("BadChance", "효과 '{0}'의 Chance는 0~1이어야 합니다."), FText::FromName(Pair.Key)));

		switch (Row.Type)
		{
		case ESSEventEffect::None:
			OutErrors.Add(FText::Format(LOCTEXT("NoEffectType", "효과 '{0}'의 Type이 비어 있습니다."), FText::FromName(Pair.Key)));
			break;
		case ESSEventEffect::Item:
			CheckItemRef(Row.Target, Pair.Key);
			break;
		case ESSEventEffect::ScheduleEvent:
			if (!EventTable->FindRowUnchecked(Row.Target))
				OutErrors.Add(FText::Format(LOCTEXT("ScheduleNoEvent", "효과 '{0}'가 예약하는 사건 '{1}'이(가) 없습니다."),
					FText::FromName(Pair.Key), FText::FromName(Row.Target)));
			if (Row.Amount < 1)
				OutErrors.Add(FText::Format(LOCTEXT("ScheduleDays", "효과 '{0}'의 예약 일수(Amount)는 1 이상이어야 합니다."), FText::FromName(Pair.Key)));
			break;
		default:
			break;
		}
	}

	if (!FMath::IsFinite(DailyEventChance) || DailyEventChance < 0.f || DailyEventChance > 1.f)
		OutErrors.Add(LOCTEXT("BadDailyChance", "DailyEventChance는 0~1이어야 합니다."));

	return OutErrors.Num() == ErrorsBefore;
}

#if WITH_EDITOR
EDataValidationResult USSEventCatalog::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	TArray<FText> Errors;
	if (!Validate(Errors))
	{
		for (const FText& Error : Errors) Context.AddError(Error);
		Result = EDataValidationResult::Invalid;
	}
	return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
