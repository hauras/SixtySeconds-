#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Item/SSInventoryTypes.h"
#include "SSRunSubsystem.generated.h"

class USSExpeditionDefinition;

UENUM(BlueprintType)
enum class ESSRobotState : uint8
{
	Idle,
	Exploring,
};

UENUM(BlueprintType)
enum class ESSExpeditionStartResult : uint8
{
	Success,
	PlayerDead,
	RobotBusy,
	InvalidExpedition,
	NotEnoughBattery,
};

USTRUCT(BlueprintType)
struct FSSExpeditionResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 ReturnDay = 0;

	UPROPERTY(BlueprintReadOnly)
	TArray<FSSItemStack> ReceivedItems;

	UPROPERTY(BlueprintReadOnly)
	FText RegionName;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRobotReturned, const FSSExpeditionResult&, Result);

// 씬 전환(스크램블→은신처)에도 유지되는 플레이 데이터 관리
UCLASS()
class SIXTYSECONDS_API USSRunSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// 운반 목록을 은신처 보관함으로 이전
	UFUNCTION(BlueprintCallable, Category="SS|Run")
	void DepositItems(const TArray<FSSItemStack>& CarriedItems);

	// 보관된 아이템 수량 반환 (없으면 0)
	UFUNCTION(BlueprintPure, Category="SS|Run")
	int32 GetStoredQuantity(USSItemDefinition* Item) const;

	UFUNCTION(BlueprintPure, Category="SS|Run")
	const TArray<FSSItemStack>& GetStoredItems() const { return StoredItems; }

	// 새 게임 시작 시 초기화
	UFUNCTION(BlueprintCallable, Category="SS|Run")
	void ResetRun();

	void InitializeShelterStats(float InHealth, float InSatiety, float InHydration);

	int32 GetCurrentDay() const { return CurrentDay; }
	float GetHealth() const { return Health; }
	float GetSatiety() const { return Satiety; }
	float GetHydration() const { return Hydration; }

	bool AdvanceDay(bool bGiveFood, bool bGiveWater);

	int32 GetStoredQuantityById(FName ItemId) const;

	// 즉시 사용. 스탯이 이미 최대이면 사용 거부.
	bool UseItem(FName ItemId);

	// 탐사 파견. 실패 시 배터리·로봇 상태 변화 없음.
	UFUNCTION(BlueprintCallable, Category="SS|Expedition")
	ESSExpeditionStartResult StartExpedition(USSExpeditionDefinition* Expedition);

	UFUNCTION(BlueprintPure, Category="SS|Expedition")
	ESSRobotState GetRobotState() const { return RobotState; }

	UFUNCTION(BlueprintPure, Category="SS|Expedition")
	int32 GetRemainingExpeditionDays() const { return RemainingExpeditionDays; }

	UFUNCTION(BlueprintPure, Category="SS|Expedition")
	const FSSExpeditionResult& GetLastExpeditionResult() const { return LastExpeditionResult; }

	// 귀환 시 브로드캐스트
	UPROPERTY(BlueprintAssignable, Category="SS|Expedition")
	FOnRobotReturned OnRobotReturned;

private:
	USSItemDefinition* FindStoredItem(FName ItemId) const;
	bool ApplyItemEffect(USSItemDefinition* Item, bool bAllowFullStat);
	bool ConsumeItem(FName ItemId);
	bool ConsumeStoredItems(FName ItemId, int32 Quantity);

	void TickExpedition();
	void FulfillExpedition();

	UPROPERTY()
	TArray<FSSItemStack> StoredItems;

	UPROPERTY(Transient)
	int32 CurrentDay = 1;

	UPROPERTY(Transient)
	float Health = 100.f;

	UPROPERTY(Transient)
	float Satiety = 100.f;

	UPROPERTY(Transient)
	float Hydration = 100.f;

	UPROPERTY(Transient)
	ESSRobotState RobotState = ESSRobotState::Idle;

	UPROPERTY(Transient)
	TObjectPtr<USSExpeditionDefinition> ActiveExpedition = nullptr;

	UPROPERTY(Transient)
	int32 RemainingExpeditionDays = 0;

	UPROPERTY(Transient)
	FSSExpeditionResult LastExpeditionResult;
};
