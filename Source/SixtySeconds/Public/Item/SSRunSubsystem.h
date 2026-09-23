#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Item/SSInventoryTypes.h"
#include "Item/SSJournalTypes.h"
#include "Character/SSSurvivorTypes.h"
#include "SSRunSubsystem.generated.h"

class USSExpeditionDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSSOnStoredItemsChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSSOnRobotStateChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSSOnJournalChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSSOnActionPointsChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSSOnSurvivorsChanged);

UENUM(BlueprintType)
enum class ESSRobotState : uint8
{
	Idle,
	Exploring,
	Broken,     // 고장 — 수리키트 필요
	Repairing,  // 수리 중 — 1일 후 Idle 복귀
};

UENUM(BlueprintType)
enum class ESSExpeditionStartResult : uint8
{
	Success,
	PlayerDead,
	RobotBusy,
	RobotBroken,
	InvalidExpedition,
	NotEnoughBattery,
	NotEnoughActionPoints,
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
    UPROPERTY(BlueprintAssignable, Category="SS|Survivor")
    FSSOnSurvivorsChanged OnSurvivorsChanged;

    UFUNCTION(BlueprintPure, Category="SS|Survivor")
    bool IsSurvivorRescued(FName SurvivorId) const;

    bool RecruitSurvivor(USSSurvivorDefinition* Definition);
    int32 RescueFollowingSurvivors();
    void ClearFollowingSurvivors() { FollowingSurvivors.Reset(); }
    UFUNCTION(BlueprintPure, Category="SS|Survivor")
    int32 GetFollowingSurvivorCount() const { return FollowingSurvivors.Num(); }
    UFUNCTION(BlueprintPure, Category="SS|Survivor")
    const TArray<FSSSurvivorState>& GetRescuedSurvivors() const { return RescuedSurvivors; }
	const TArray<FSSJournalEntry>& GetJournalEntries() const { return JournalEntries; }
	UPROPERTY(BlueprintAssignable, Category="SS|Journal")
	FSSOnJournalChanged OnJournalChanged;
	UPROPERTY(BlueprintAssignable, Category="SS|Expedition")
	FSSOnRobotStateChanged OnRobotStateChanged;
	UPROPERTY(BlueprintAssignable, Category="SS|Run")
	FSSOnActionPointsChanged OnActionPointsChanged;

	UPROPERTY(BlueprintAssignable, Category="SS|Run")
	FSSOnStoredItemsChanged OnStoredItemsChanged;

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
	float GetHealth() const { return PlayerStats.Health; }
	float GetSatiety() const { return PlayerStats.Satiety; }
	float GetHydration() const { return PlayerStats.Hydration; }
	int32 GetActionPoints() const { return ActionPoints; }
	static constexpr int32 MaxActionPoints = 5;
	static constexpr int32 ExpeditionActionCost = 2;
	static constexpr int32 RepairActionCost = 4;

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

	// 수리키트 1개 + 행동력 4 소모 후 수리 시작. 성공 시 true.
	UFUNCTION(BlueprintCallable, Category="SS|Expedition")
	bool RepairRobot();
	int32 GetRemainingRepairDays() const { return RepairDaysRemaining; }

	// 귀환 시 브로드캐스트
	UPROPERTY(BlueprintAssignable, Category="SS|Expedition")
	FOnRobotReturned OnRobotReturned;

private:
    UPROPERTY(Transient) TArray<TObjectPtr<USSSurvivorDefinition>> FollowingSurvivors;
    UPROPERTY(Transient) TArray<FSSSurvivorState> RescuedSurvivors;
	void RecordEvent(ESSJournalEvent Event, const FText& Message);
	void RecordExpeditionReturn(const FSSExpeditionResult& Result, bool bSuccess);
	UPROPERTY(Transient) TArray<FSSJournalEntry> JournalEntries;
	USSItemDefinition* FindStoredItem(FName ItemId) const;
	bool ApplyItemEffect(USSItemDefinition* Item, bool bAllowFullStat);
	bool ConsumeItem(FName ItemId);
	bool ConsumeStoredItems(FName ItemId, int32 Quantity);

	void TickExpedition();
	void FulfillExpedition();
	void TickRepair();

	UPROPERTY()
	TArray<FSSItemStack> StoredItems;

	UPROPERTY(Transient)
	int32 CurrentDay = 1;

	UPROPERTY(Transient)
	FSSSurvivorStats PlayerStats;
	
	UPROPERTY(Transient)
	ESSRobotState RobotState = ESSRobotState::Idle;

	UPROPERTY(Transient)
	TObjectPtr<USSExpeditionDefinition> ActiveExpedition = nullptr;

	UPROPERTY(Transient)
	int32 RemainingExpeditionDays = 0;

	UPROPERTY(Transient)
	int32 RepairDaysRemaining = 0;

	UPROPERTY(Transient)
	FSSExpeditionResult LastExpeditionResult;

	UPROPERTY(Transient)
	int32 ActionPoints = MaxActionPoints;

	bool ConsumeActionPoints(int32 Cost);
};
