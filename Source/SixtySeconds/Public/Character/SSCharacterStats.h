#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SSCharacterStats.generated.h"

UENUM(BlueprintType)
enum class ESSStatType : uint8
{
	HP,
	Hunger,
	Thirst,
	Radiation,
	Sanity
};

UCLASS(ClassGroup=(SixtySeconds), meta=(BlueprintSpawnableComponent))
class SIXTYSECONDS_API USSCharacterStats : public UActorComponent
{
	GENERATED_BODY()

public:
	USSCharacterStats();

	// 현재 스탯 값 반환
	UFUNCTION(BlueprintPure, Category="SS|Stats")
	float GetStat(ESSStatType StatType) const;

	// 스탯 값 직접 설정 (클램핑 적용)
	UFUNCTION(BlueprintCallable, Category="SS|Stats")
	void SetStat(ESSStatType StatType, float NewValue);

	// 스탯 값 증감 (양수=회복, 음수=감소)
	UFUNCTION(BlueprintCallable, Category="SS|Stats")
	void ModifyStat(ESSStatType StatType, float Delta);

	// 스탯이 0인지 확인
	UFUNCTION(BlueprintPure, Category="SS|Stats")
	bool IsStatDepleted(ESSStatType StatType) const;

	// 최대값 반환
	UFUNCTION(BlueprintPure, Category="SS|Stats")
	float GetMaxStat(ESSStatType StatType) const;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SS|Stats|HP")
	float MaxHP = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SS|Stats|HP")
	float CurrentHP = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SS|Stats|Hunger")
	float MaxHunger = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SS|Stats|Hunger")
	float CurrentHunger = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SS|Stats|Thirst")
	float MaxThirst = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SS|Stats|Thirst")
	float CurrentThirst = 100.f;

	// 방사능은 0이 안전, 높을수록 위험
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SS|Stats|Radiation")
	float MaxRadiation = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SS|Stats|Radiation")
	float CurrentRadiation = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SS|Stats|Sanity")
	float MaxSanity = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SS|Stats|Sanity")
	float CurrentSanity = 100.f;

private:
	float& GetStatRef(ESSStatType StatType);
	float GetMaxStatInternal(ESSStatType StatType) const;
};
