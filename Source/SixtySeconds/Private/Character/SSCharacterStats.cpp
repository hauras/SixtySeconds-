#include "Character/SSCharacterStats.h"

USSCharacterStats::USSCharacterStats()
{
	PrimaryComponentTick.bCanEverTick = false;
}

float USSCharacterStats::GetStat(ESSStatType StatType) const
{
	switch (StatType)
	{
	case ESSStatType::HP:        return CurrentHP;
	case ESSStatType::Hunger:    return CurrentHunger;
	case ESSStatType::Thirst:    return CurrentThirst;
	case ESSStatType::Radiation: return CurrentRadiation;
	case ESSStatType::Sanity:    return CurrentSanity;
	default:                     return 0.f;
	}
}

void USSCharacterStats::SetStat(ESSStatType StatType, float NewValue)
{
	const float Max = GetMaxStatInternal(StatType);
	const float Clamped = FMath::Clamp(NewValue, 0.f, Max);

	switch (StatType)
	{
	case ESSStatType::HP:        CurrentHP = Clamped;        break;
	case ESSStatType::Hunger:    CurrentHunger = Clamped;    break;
	case ESSStatType::Thirst:    CurrentThirst = Clamped;    break;
	case ESSStatType::Radiation: CurrentRadiation = Clamped; break;
	case ESSStatType::Sanity:    CurrentSanity = Clamped;    break;
	}
}

void USSCharacterStats::ModifyStat(ESSStatType StatType, float Delta)
{
	SetStat(StatType, GetStat(StatType) + Delta);
}

bool USSCharacterStats::IsStatDepleted(ESSStatType StatType) const
{
	return FMath::IsNearlyZero(GetStat(StatType));
}

float USSCharacterStats::GetMaxStat(ESSStatType StatType) const
{
	return GetMaxStatInternal(StatType);
}

float USSCharacterStats::GetMaxStatInternal(ESSStatType StatType) const
{
	switch (StatType)
	{
	case ESSStatType::HP:        return MaxHP;
	case ESSStatType::Hunger:    return MaxHunger;
	case ESSStatType::Thirst:    return MaxThirst;
	case ESSStatType::Radiation: return MaxRadiation;
	case ESSStatType::Sanity:    return MaxSanity;
	default:                     return 0.f;
	}
}
