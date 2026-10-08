#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Item/SSInventoryTypes.h"
#include "SSExpeditionState.generated.h"

class USSRunSubsystem;
class USSExpeditionDefinition;

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

	UPROPERTY(BlueprintReadOnly)
	int32 ReturnDay = 0;

	UPROPERTY(BlueprintReadOnly)
	TArray<FSSItemStack> ReceivedItems;

	UPROPERTY(BlueprintReadOnly)
	FText RegionName;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSSOnRobotStateChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRobotReturned, const FSSExpeditionResult&, Result);

// ─────────────────────────────────────────────
// 탐사 로봇 (판마다 바뀌는 값)
// 파견 → 며칠 뒤 귀환(보상·고장) → 수리. 날이 지날 때 RunSubsystem이 TickDay를 부름
// 행동력·보관함·기록은 RunSubsystem의 공개 함수로 부탁함
// ─────────────────────────────────────────────
UCLASS()
class SIXTYSECONDS_API USSExpeditionState : public UObject
{
	GENERATED_BODY()

public:
	static constexpr int32 ExpeditionActionCost = 2;
	static constexpr int32 RepairActionCost = 4;

	// 탐사 파견. 실패 시 배터리·로봇 상태 변화 없음
	ESSExpeditionStartResult StartExpedition(USSExpeditionDefinition* Expedition);

	// 수리키트 1개 + 행동력 4 소모 후 수리 시작. 성공 시 true
	bool RepairRobot();

	ESSRobotState GetRobotState() const { return RobotState; }
	int32 GetRemainingExpeditionDays() const { return RemainingExpeditionDays; }
	int32 GetRemainingRepairDays() const { return RepairDaysRemaining; }
	const FSSExpeditionResult& GetLastExpeditionResult() const { return LastExpeditionResult; }

	// 하루가 지남 (RunSubsystem이 날짜를 넘기기 전에 부름): 탐사 남은 날 → 귀환, 수리 남은 날 → 완료
	void TickDay();

	// 새 판
	void ResetRun();

	UPROPERTY(BlueprintAssignable, Category="SS|Expedition")
	FSSOnRobotStateChanged OnRobotStateChanged;

	// 귀환 결과 전달
	UPROPERTY(BlueprintAssignable, Category="SS|Expedition")
	FOnRobotReturned OnRobotReturned;

private:
	// 이 상태를 들고 있는 RunSubsystem
	USSRunSubsystem& GetRun() const;

	void TickExpedition();
	void FulfillExpedition();
	void TickRepair();
	void RecordExpeditionReturn(const FSSExpeditionResult& Result, bool bSuccess);

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
};
