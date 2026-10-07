#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Item/SSInventoryTypes.h"
#include "Item/SSJournalTypes.h"
#include "Character/SSSurvivorTypes.h"
#include "Rescue/SSRescueSession.h"
#include "SSRunSubsystem.generated.h"

class USSCommsState;
class USSCompanionState;
class USSAraDirector;

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
	void AddDetailedEventJournal(const FText& Message, const FText& Title, const FText& Body,
		const FText& Choice, const FText& Outcome, const FText& Changes);

	void BuildAraBriefing();   // 현재 날짜·물자로 아침 보고 문장을 만들어 저장

	const FText& GetAraBriefing() const { return AraBriefing; }   // 오늘 아침 ARA 보고 (하루 시작 때 고정)
	bool HasUnreadAraBriefing() const { return LastAraReadDay < CurrentDay; }
	void MarkAraBriefingRead() { LastAraReadDay = CurrentDay; }
	bool AskAraQuestion(int32 QuestionIndex, FText& OutAnswer);
	bool HasAskedAraQuestionToday(int32 QuestionIndex) const;

	// 아라 학습도 (질문할수록 오름. 아라가 대피실 대화를 엿들을 확률에 씀)
	int32 GetAraLearningScore() const { return AraLearningScore; }

	// 아라의 판단 (누구를 의심하는지, 표적). 처음 부를 때 생성
	USSAraDirector* GetAra();

	// ── 붙잡힌 동료 (B2 격리 구역. 사망이 아니라 구출할 수 있음) ──

	// 붙잡힌 동료들 (바꿔치기된 진짜, 잘못 격리된 사람)
	const TArray<FSSSurvivorState>& GetCapturedSurvivors() const { return CapturedSurvivors; }

	// 그 동료가 B2에 붙잡혀 있나
	bool IsSurvivorCaptured(FName SurvivorId) const;

	// 은신처 동료를 붙잡힌 목록으로 옮김 (사람을 격리했을 때). 은신처에서는 사라짐
	bool MoveSurvivorToCaptured(FName SurvivorId);

	// 은신처에는 그대로 두고 몸 상태만 붙잡힌 목록에 복사 (바꿔치기: 진짜는 B2, 같은 얼굴의 안드로이드는 은신처)
	bool CopySurvivorToCaptured(FName SurvivorId);

	// 은신처 목록에서 지움 (안드로이드를 격리해 제거했을 때)
	bool RemoveRescuedSurvivor(FName SurvivorId);

	// 붙잡힌 동료를 은신처로 돌려보냄. 은신처에 같은 얼굴의 안드로이드가 있으면 그 자리를 진짜가 채움
	// bOutReplacedAndroid: 안드로이드를 밀어냈는지 (결과 카드 문장용)
	bool ReleaseCapturedSurvivor(FName SurvivorId, bool& bOutReplacedAndroid);

	// ── B2 정비 패널 (구출 퍼즐) ──

	// 패널을 열 수 있는 최소 행동력 (패널 작업은 남은 행동력을 전부 씀)
	static constexpr int32 RescueMinActionPoints = 2;

	// 패널을 못 여는 이유 (None이면 열 수 있음)
	ESSRescueBlock GetRescueBlock() const;

	// 패널 작업 시작: 남은 행동력을 전부 쓰고 세션을 만듦. 못 하면 nullptr
	// 회전 수 = 행동력 × 8 (태오가 은신처에 있으면) / × 4 (없으면). 태오가 안드로이드면 시작부터 경보 1회
	// Seed 0이면 무작위 퍼즐
	USSRescueSession* StartRescue(FName TargetId, int32 Seed = 0);

	// 진행 중인 패널 작업 (없으면 nullptr)
	USSRescueSession* GetActiveRescue() const { return ActiveRescue; }

	// 끝난 패널 작업을 반영: 성공이면 동료 귀환, 경보만큼 도운 동료 의심 상승, 기록. 반영했으면 true
	// 한 번만 반영됨 (반영 후 진행 중 작업을 비우므로 다시 부르면 false)
	bool FinishRescue(FSSRescueReport& OutReport);

	// 패널을 한 번이라도 열었나 (패널 기록 = 숨은 진실 단서 1, 엔딩 판정용)
	bool HasSeenPanelLog() const { return bSawPanelLog; }

	// 아라 판단을 읽기만 할 때 (아직 없으면 nullptr. 사건 조건처럼 const에서 씀)
	const USSAraDirector* FindAra() const { return Ara; }

	// ── 외부 통신 (역추적 + 받은 메시지) ──
	// 날을 넘어 이어지는 통신 상태. 처음 부를 때 생성
	USSCommsState* GetComms();

	// 다른 시스템(외부 통신 등)이 저널을 남기거나 행동력을 쓸 때
	void AddJournal(ESSJournalEvent Event, const FText& Message) { RecordEvent(Event, Message); }
	bool SpendActionPoints(int32 Cost) { return ConsumeActionPoints(Cost); }

	// ── 동료 조사 ──
	// 동료가 밤에 무엇을 조사했는지, 보고를 들었는지 기억하는 기록. 처음 부를 때 생성
	USSCompanionState* GetCompanions();

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
	void RecordExpeditionReturn(const FSSExpeditionResult& Result, bool bSuccess);

	// 동료 데이터
	UPROPERTY(Transient)
	TArray<TObjectPtr<USSSurvivorDefinition>> FollowingSurvivors;

	UPROPERTY(Transient)
	TArray<FSSSurvivorState> RescuedSurvivors;

	// B2에 붙잡힌 동료
	UPROPERTY(Transient)
	TArray<FSSSurvivorState> CapturedSurvivors;

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

	UPROPERTY(Transient)
	TObjectPtr<USSCommsState> Comms;

	// 동료가 조사 기록을 관리한다.
	UPROPERTY(Transient)
	TObjectPtr<USSCompanionState> Companions;

	// 아라가 동료를 의심하는 정도와 표적
	UPROPERTY(Transient)
	TObjectPtr<USSAraDirector> Ara;

	// 진행 중인 B2 패널 작업
	UPROPERTY(Transient)
	TObjectPtr<USSRescueSession> ActiveRescue;

	// 패널 작업을 도운 동료 (태오, 혼자면 None). 경보 의심을 받을 사람
	UPROPERTY(Transient)
	FName RescueHelperId = NAME_None;

	// 마지막으로 패널을 연 날 (하루 한 번)
	UPROPERTY(Transient)
	int32 LastRescueDay = 0;

	// 이번 패널 작업에 쓴 행동력 (결과 카드용)
	UPROPERTY(Transient)
	int32 RescueActionPoints = 0;

	UPROPERTY(Transient)
	bool bSawPanelLog = false;
};
