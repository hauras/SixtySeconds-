#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Rescue/SSPowerGrid.h"
#include "SSRescueSession.generated.h"   // 반드시 맨 마지막 include

// 패널 작업이 어떻게 끝났는지
UENUM()
enum class ESSRescueOutcome : uint8
{
	InProgress,   // 작업 중
	Unlocked,     // 잠금 해제 성공
	OutOfMoves,   // 회전을 다 씀
	Aborted,      // 작업 중단
};

// 구출에 쓰는 고정 ID
namespace SSRescueIds
{
	// B2로 내려가는 덕트 단서 (이걸 들어야 패널을 열 수 있음)
	inline FName RouteClue() { return FName(TEXT("Clue_Vent_2")); }

	// 패널을 돕는 정비사 강태오
	inline FName Technician() { return FName(TEXT("Technician")); }
}

// 패널을 못 여는 이유 (은신처 버튼 툴팁)
UENUM()
enum class ESSRescueBlock : uint8
{
	None,                    // 열 수 있음
	NobodyCaptured,          // 붙잡힌 사람이 없음
	NoRoute,                 // B2로 가는 덕트를 모름 (단서 Clue_Vent_2)
	NotEnoughActionPoints,   // 행동력 부족
	AlreadyToday,            // 오늘 이미 염
	InProgress,              // 작업 중
};

// 끝난 패널 작업 결과 (결과 카드·기록용)
struct FSSRescueReport
{
	ESSRescueOutcome Outcome = ESSRescueOutcome::InProgress;
	FName TargetId = NAME_None;
	int32 AlarmCount = 0;
	int32 MovesUsed = 0;

	// 시작할 때 실제로 쓴 행동력 (회전 수와 다름)
	int32 ActionPointsSpent = 0;

	// 진짜가 돌아오면서 은신처의 안드로이드가 밀려났는지
	bool bReplacedAndroid = false;
};

// 회전할 때마다 화면 갱신용 알림
DECLARE_MULTICAST_DELEGATE(FSSOnRescueChanged);

// B2 정비 패널 작업 한 번 (퍼즐 + 회전 횟수 + 경보 횟수 + 결과)
UCLASS()
class SIXTYSECONDS_API USSRescueSession : public UObject
{
	GENERATED_BODY()

public:
	// 행동력으로 회전 횟수 계산: 태오가 도우면 × 8, 혼자면 × 4
	static int32 CalcMoveBudget(int32 ActionPoints, bool bTechnicianHelps);

	// 작업 시작. bAlarmAlreadyTripped = 안드로이드 태오가 도운 경우 (시작부터 경보 1회)
	void Start(FName InTargetId, int32 InMoveBudget, bool bAlarmAlreadyTripped, int32 Seed);

	// 타일 하나 돌리기. 돌렸으면 true (끝났거나 회전이 없으면 false)
	bool Rotate(int32 Cell);

	// 작업 중단
	void Abort();

	// 화면이 읽는 값들
	const FSSPowerGrid& GetGrid() const { return Grid; }
	FName GetTargetId() const { return TargetId; }
	int32 GetMoveBudget() const { return MoveBudget; }
	int32 GetRemainingMoves() const { return RemainingMoves; }
	int32 GetAlarmCount() const { return AlarmCount; }
	ESSRescueOutcome GetOutcome() const { return Outcome; }
	bool IsFinished() const { return Outcome != ESSRescueOutcome::InProgress; }

	FSSOnRescueChanged OnChanged;

	// 판 크기와 경보 수
	static constexpr int32 GridWidth = 5;
	static constexpr int32 GridHeight = 4;
	static constexpr int32 AlarmTerminals = 3;

	// 정답 경로를 맞추는 데 드는 회전이 예산의 이 비율을 넘지 않게 섞음 (나머지는 헤매는 여유)
	static constexpr float SolvableShare = 0.6f;

private:
	// 지금 전력이 닿은 단자 중 새로 닿은 것만 세기
	void CountNewAlarms();

	FSSPowerGrid Grid;
	FName TargetId;
	int32 MoveBudget = 0;
	int32 RemainingMoves = 0;
	int32 AlarmCount = 0;
	ESSRescueOutcome Outcome = ESSRescueOutcome::InProgress;

	// 바로 전 회전 때 닿아 있던 단자들 (계속 닿아 있으면 다시 안 셈)
	TSet<int32> TouchingAlarms;
};