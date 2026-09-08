#include "Character/SSStatusComponent.h"

USSStatusComponent::USSStatusComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USSStatusComponent::AddStatus(const FSSStatusEffect& Effect)
{
	// 같은 타입이 이미 있으면 턴 수만 갱신
	for (FSSStatusEffect& Existing : ActiveStatuses)
	{
		if (Existing.Type == Effect.Type)
		{
			Existing.RemainingTurns = Effect.RemainingTurns;
			return;
		}
	}
	ActiveStatuses.Add(Effect);
}

void USSStatusComponent::RemoveStatus(ESSStatusType StatusType)
{
	ActiveStatuses.RemoveAll([StatusType](const FSSStatusEffect& Effect)
	{
		return Effect.Type == StatusType;
	});
}

bool USSStatusComponent::HasStatus(ESSStatusType StatusType) const
{
	return ActiveStatuses.ContainsByPredicate([StatusType](const FSSStatusEffect& Effect)
	{
		return Effect.Type == StatusType;
	});
}

void USSStatusComponent::OnTurnPassed(USSCharacterStats* Stats)
{
	if (!IsValid(Stats)) return;

	// 만료된 상태이상 제거용 임시 배열
	TArray<ESSStatusType> ToRemove;

	for (FSSStatusEffect& Effect : ActiveStatuses)
	{
		// 스탯에 틱 효과 적용
		if (!FMath::IsNearlyZero(Effect.HPTickDelta))
			Stats->ModifyStat(ESSStatType::HP, Effect.HPTickDelta);

		if (!FMath::IsNearlyZero(Effect.SanityTickDelta))
			Stats->ModifyStat(ESSStatType::Sanity, Effect.SanityTickDelta);

		if (!FMath::IsNearlyZero(Effect.RadiationTickDelta))
			Stats->ModifyStat(ESSStatType::Radiation, Effect.RadiationTickDelta);

		// 턴 감소 (-1은 영구)
		if (Effect.RemainingTurns > 0)
		{
			Effect.RemainingTurns--;
			if (Effect.RemainingTurns == 0)
				ToRemove.Add(Effect.Type);
		}
	}

	for (ESSStatusType Type : ToRemove)
		RemoveStatus(Type);
}
