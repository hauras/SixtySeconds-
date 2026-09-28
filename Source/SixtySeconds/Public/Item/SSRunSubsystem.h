#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Item/SSInventoryTypes.h"
#include "Item/SSJournalTypes.h"
#include "Character/SSSurvivorTypes.h"
#include "SSRunSubsystem.generated.h"

class USSExpeditionDefinition;
class USSEventDirector;
struct FSSExplorationResult;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSSOnDayAdvanced);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSSOnPlayerStatsChanged);
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
	Broken,    // 고장 — 수리키트 필요
	Repairing, // 수리 중 — 1일 후 Idle 복귀
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

	UPROPERTY(BlueprintReadOnly) int32 ReturnDay = 0;
	UPROPERTY(BlueprintReadOnly) TArray<FSSItemStack> ReceivedItems;
	UPROPERTY(BlueprintReadOnly) FText RegionName;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRobotReturned, const FSSExpeditionResult&, Result);

// 씬 전환(스크램블↔은신처)에도 유지되는 플레이 데이터 관리
UCLASS()
class SIXTYSECONDS_API USSRunSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// 게임 진행 및 플레이어 상태
	UFUNCTION(BlueprintCallable, Category="SS|Run")
	void ResetRun();

	void InitializeShelterStats(float InHealth, float InSatiety, float InHydration);
	// 다음 날 버튼: 배급 → 하루 경과 → OnDayAdvanced
	bool AdvanceDay(bool bGiveFood, bool bGiveWater);

	// 직접 탐사 정산: 하루 경과(출발 전 재고로 배급) → 귀환 성공이면 입고, 실패면 부상 → OnDayAdvanced
	bool ApplyExplorationResult(const FSSExplorationResult& Result, bool bGiveFood, bool bGiveWater);

	// 플레이어 배급 선택 + 살아 있는 동료의 배급 예약을 합친 필요 수량
	void GetRequiredRations(bool bGiveFood, bool bGiveWater, int32& OutFood, int32& OutWater) const;
	bool HasRationsFor(bool bGiveFood, bool bGiveWater) const;

	int32 GetCurrentDay() const { return CurrentDay; }
	float GetHealth() const { return PlayerStats.Health; }
	float GetSatiety() const { return PlayerStats.Satiety; }
	float GetHydration() const { return PlayerStats.Hydration; }

	// 행동력
	static constexpr int32 MaxActionPoints = 5;
	static constexpr int32 ExpeditionActionCost = 2;
	static constexpr int32 RepairActionCost = 4;
	static constexpr int32 HealActionCost = 1;
	
	int32 GetActionPoints() const { return ActionPoints; }

	// 보관함 및 아이템 사용
	// 운반 목록을 은신처 보관함으로 이전
	UFUNCTION(BlueprintCallable, Category="SS|Run")
	void DepositItems(const TArray<FSSItemStack>& CarriedItems);

	// 보관된 아이템 수량 반환 (없으면 0)
	UFUNCTION(BlueprintPure, Category="SS|Run")
	int32 GetStoredQuantity(USSItemDefinition* Item) const;

	int32 GetStoredQuantityById(FName ItemId) const;

	UFUNCTION(BlueprintPure, Category="SS|Run")
	const TArray<FSSItemStack>& GetStoredItems() const { return StoredItems; }

	// 즉시 사용. 스탯이 이미 최대이면 사용 거부.
	bool UseItem(FName ItemId);

	// 동료 구조 및 배급
	bool RecruitSurvivor(USSSurvivorDefinition* Definition);
	int32 RescueFollowingSurvivors();
	void ClearFollowingSurvivors() { FollowingSurvivors.Reset(); }
	void SetSurvivorFoodRation(FName SurvivorId, bool bGive);
	void SetSurvivorWaterRation(FName SurvivorId, bool bGive);

	bool HealSurvivor(FName SurvivorId);

	const FSSSurvivorState* FindRescuedSurvivor(FName SurvivorId) const;

	UFUNCTION(BlueprintPure, Category="SS|Survivor")
	bool IsSurvivorRescued(FName SurvivorId) const { return FindRescuedSurvivor(SurvivorId) != nullptr; }

	UFUNCTION(BlueprintPure, Category="SS|Survivor")
	int32 GetFollowingSurvivorCount() const { return FollowingSurvivors.Num(); }

	UFUNCTION(BlueprintPure, Category="SS|Survivor")
	const TArray<FSSSurvivorState>& GetRescuedSurvivors() const { return RescuedSurvivors; }

	// 로봇 탐사 및 수리
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

	// 일일 기록
	const TArray<FSSJournalEntry>& GetJournalEntries() const { return JournalEntries; }

	// 하루 사건: 규칙과 런 상태(1회성·쿨다운·예약)를 가진 디렉터. 처음 부를 때 생성
	USSEventDirector* GetEventDirector();

	// 사건 효과용 변경 함수
	bool RemoveStoredItemsById(FName ItemId, int32 Quantity) { return ConsumeStoredItems(ItemId, Quantity); }
	void ModifyPlayerStats(float DeltaHealth, float DeltaSatiety, float DeltaHydration);
	void ModifySurvivorsHealth(float Delta);   // 살아 있는 모든 동료. 0이 되면 사망 처리
	void AdjustActionPoints(int32 Delta);      // 0 ~ MaxActionPoints
	void AddEventJournal(const FText& Message) { RecordEvent(ESSJournalEvent::Event, Message); }

	const FText& GetAraBriefing() const { return AraBriefing; }   // 오늘 아침 ARA 보고 (하루 시작 때 고정)
	bool HasUnreadAraBriefing() const { return LastAraReadDay < CurrentDay; }
	void MarkAraBriefingRead() { LastAraReadDay = CurrentDay; }
	bool AskAraQuestion(int32 QuestionIndex, FText& OutAnswer);
	bool HasAskedAraQuestionToday(int32 QuestionIndex) const;

	// UI 등에 데이터 변경을 알리는 이벤트
	// 하루가 지남 (다음 날 버튼, 직접 탐사 정산 모두). 날짜·플레이어 스탯이 바뀌었으니 다시 그리고 사망 확인
	UPROPERTY(BlueprintAssignable, Category="SS|Run")
	FSSOnDayAdvanced OnDayAdvanced;

	// 하루 경과 외의 이유(사건 등)로 플레이어 스탯이 바뀜. HUD가 스탯을 다시 그리고 사망 확인
	UPROPERTY(BlueprintAssignable, Category="SS|Run")
	FSSOnPlayerStatsChanged OnPlayerStatsChanged;

	UPROPERTY(BlueprintAssignable, Category="SS|Survivor")
	FSSOnSurvivorsChanged OnSurvivorsChanged;

	UPROPERTY(BlueprintAssignable, Category="SS|Journal")
	FSSOnJournalChanged OnJournalChanged;

	UPROPERTY(BlueprintAssignable, Category="SS|Expedition")
	FSSOnRobotStateChanged OnRobotStateChanged;

	UPROPERTY(BlueprintAssignable, Category="SS|Run")
	FSSOnActionPointsChanged OnActionPointsChanged;

	UPROPERTY(BlueprintAssignable, Category="SS|Run")
	FSSOnStoredItemsChanged OnStoredItemsChanged;

	// 귀환 결과 전달
	UPROPERTY(BlueprintAssignable, Category="SS|Expedition")
	FOnRobotReturned OnRobotReturned;

private:
	// 내부 처리 함수
	bool AdvanceDayCore(bool bGiveFood, bool bGiveWater);   // 배급·감소·날짜·행동력·로봇 진행 (알림 없음)
	bool ConsumeActionPoints(int32 Cost);
	FSSSurvivorState* FindRescuedSurvivorMutable(FName SurvivorId);

	USSItemDefinition* FindStoredItem(FName ItemId) const;
	bool ApplyItemEffect(USSItemDefinition* Item, bool bAllowFullStat);
	bool ConsumeItem(FName ItemId);
	bool ConsumeStoredItems(FName ItemId, int32 Quantity);

	void TickExpedition();
	void FulfillExpedition();
	void TickRepair();

	void RecordEvent(ESSJournalEvent Event, const FText& Message);
	void BuildAraBriefing();   // 오늘 날짜·물자로 아침 보고 문장을 만들어 저장
	void RecordExpeditionReturn(const FSSExpeditionResult& Result, bool bSuccess);

	// 동료 데이터
	UPROPERTY(Transient)
	TArray<TObjectPtr<USSSurvivorDefinition>> FollowingSurvivors;

	UPROPERTY(Transient)
	TArray<FSSSurvivorState> RescuedSurvivors;

	// 일일 기록
	UPROPERTY(Transient)
	TArray<FSSJournalEntry> JournalEntries;

	// 보관함
	UPROPERTY()
	TArray<FSSItemStack> StoredItems;

	// 게임 진행 및 플레이어 상태
	UPROPERTY(Transient)
	int32 CurrentDay = 1;

	UPROPERTY(Transient)
	int32 LastAraReadDay = 0;

	UPROPERTY(Transient)
	FText AraBriefing;   // 아침에 만든 문장을 저장. 열 때마다 다시 만들지 않음

	UPROPERTY(Transient)
	uint8 AskedAraQuestionsMask = 0;

	UPROPERTY(Transient)
	int32 AraLearningScore = 0;

	UPROPERTY(Transient)
	FSSSurvivorStats PlayerStats;

	// 로봇 상태 및 탐사 결과
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

	// 남은 행동력
	UPROPERTY(Transient)
	int32 ActionPoints = MaxActionPoints;

	UPROPERTY(Transient)
	TObjectPtr<USSEventDirector> EventDirector;
};
