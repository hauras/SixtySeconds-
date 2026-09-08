#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Character/SSCharacterStats.h"
#include "SSStatusComponent.generated.h"

UENUM(BlueprintType)
enum class ESSStatusType : uint8
{
	Poisoned,     // 독: 매 턴 HP 감소
	Irradiated,   // 방사능 오염: 매 턴 Radiation 증가
	Starving,     // 굶주림: 매 턴 HP 감소 (Hunger 0일 때)
	Dehydrated,   // 탈수: 매 턴 HP 감소 (Thirst 0일 때)
	Insane,       // 정신이상: 매 턴 Sanity 감소
	Injured       // 부상: HP 회복 불가
};

USTRUCT(BlueprintType)
struct FSSStatusEffect
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ESSStatusType Type = ESSStatusType::Poisoned;

	// 남은 턴 수 (-1 = 영구)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 RemainingTurns = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float HPTickDelta = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SanityTickDelta = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RadiationTickDelta = 0.f;
};

UCLASS(ClassGroup=(SixtySeconds), meta=(BlueprintSpawnableComponent))
class SIXTYSECONDS_API USSStatusComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USSStatusComponent();

	// 상태이상 추가 (같은 타입이면 턴 수 갱신)
	UFUNCTION(BlueprintCallable, Category="SS|Status")
	void AddStatus(const FSSStatusEffect& Effect);

	// 상태이상 제거
	UFUNCTION(BlueprintCallable, Category="SS|Status")
	void RemoveStatus(ESSStatusType StatusType);

	// 특정 상태이상 보유 여부
	UFUNCTION(BlueprintPure, Category="SS|Status")
	bool HasStatus(ESSStatusType StatusType) const;

	// 현재 상태이상 목록 반환
	UFUNCTION(BlueprintPure, Category="SS|Status")
	const TArray<FSSStatusEffect>& GetActiveStatuses() const { return ActiveStatuses; }

	// 턴 경과 처리 — 은신처 턴제에서 매 턴 호출
	UFUNCTION(BlueprintCallable, Category="SS|Status")
	void OnTurnPassed(USSCharacterStats* Stats);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SS|Status")
	TArray<FSSStatusEffect> ActiveStatuses;
};
