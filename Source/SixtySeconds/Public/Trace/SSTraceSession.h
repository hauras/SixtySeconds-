#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Trace/SSSuspectMemory.h"
#include "SSTraceSession.generated.h"

class USSTraceConfig;

// 판 상태가 바뀜 (송신, 단자 선택, 판 끝). 화면이 듣고 다시 그림
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSSOnTraceChanged);

// 역추적 한 판이 어떻게 끝났는지
UENUM(BlueprintType)
enum class ESSTraceOutcome : uint8
{
	InProgress,   // 진행 중
	Completed,    // 메시지 수신 100%
	Stopped,      // 플레이어가 종료했거나 턴을 다 씀
	Exposed,      // 들킴 → 그날 밤 습격
};

// ─────────────────────────────────────────────
// 외부 통신(역추적) 한 판
// 플레이어의 송신을 신호로 만들어 적의 기억에 넣고, 들킴·단자 차단·판 끝을 판단한다
// 중계기는 연구소에 원래 있는 통신 단자(Config의 RelayPositions) 중 하나를 골라 원격으로 경유
// ─────────────────────────────────────────────
UCLASS()
class SIXTYSECONDS_API USSTraceSession : public UObject
{
	GENERATED_BODY()

public:
	// 판 시작
	//   InConfig         : 이 판의 맵과 규칙
	//   InSpots          : 날을 넘어온 적의 의심 장소들 (세션 안에 복사해서 씀)
	//   InBlockedRelays  : 지금 차단돼서 못 쓰는 단자 번호들
	//   InReceived       : 지난번까지 받은 횟수 (수신 진행도 이어받기)
	void Initialize(
		USSTraceConfig* InConfig,
		const TArray<FSSSuspectSpot>& InSpots,
		const TArray<int32>& InBlockedRelays,
		int32 InReceived);

	// 은신처에서 직접 송신. 판이 끝났으면 false
	bool SendDirect();

	// 고른 단자를 거쳐 송신. 고른 단자가 없거나 판이 끝났으면 false
	bool SendViaRelay();

	// 경유할 단자 고르기. 번호가 없거나 차단된 단자면 false
	bool SelectRelay(int32 RelayIndex);

	// 판 끝내기 (받은 만큼은 남음)
	void EndSession();

	// 난수 시드 고정 (테스트에서 매번 같은 결과가 나오게). Initialize가 새 시드를 뽑으므로 그 뒤에 부를 것
	void SetSeed(int32 Seed) { Random.Initialize(Seed); }

	// ── 화면과 RunSubsystem이 읽는 값 ──

	const USSTraceConfig* GetConfig() const { return Config; }

	// 적의 의심 장소들
	const TArray<FSSSuspectSpot>& GetSpots() const { return Spots; }

	// 고른 단자
	bool HasRelay() const { return SelectedRelay != INDEX_NONE; }
	int32 GetSelectedRelay() const { return SelectedRelay; }
	FVector2D GetRelayPosition() const;

	// 단자가 차단됐나
	bool IsRelayBlocked(int32 RelayIndex) const { return BlockedRelays.Contains(RelayIndex); }

	// 이번 판에 새로 차단된 단자들 (판이 끝나면 RunSubsystem이 날짜와 함께 기억)
	const TArray<int32>& GetNewlyBlockedRelays() const { return NewlyBlockedRelays; }

	// 받은 횟수 (수신률 = 받은 횟수 ÷ Config의 ReceiveGoal)
	int32 GetReceived() const { return Received; }

	// 지금까지 송신한 횟수
	int32 GetTurn() const { return Turn; }

	// 적의 경계 수준
	int32 GetAlertLevel() const { return AlertLevel; }

	// 판 결과
	ESSTraceOutcome GetOutcome() const { return Outcome; }

	// 판 상태가 바뀔 때마다 알림
	UPROPERTY(BlueprintAssignable, Category="SS|Trace")
	FSSOnTraceChanged OnTraceChanged;

private:
	// 지금 행동할 수 있나 (규칙이 있고 판이 진행 중)
	bool CanAct() const;

	// 경계 수준이 반영된 센서 오차
	float ScaledSigma(float Sigma) const;

	// 신호 위치에서 센서마다 잡음 섞인 읽은 값을 만듦
	//   Source : 신호가 나간 위치
	//   Sigma  : 이번 신호의 센서 오차 (경계 수준이 반영되기 전 값)
	TArray<FSSSensorReading> MakeReadings(const FVector2D& Source, float Sigma);

	// 송신 뒤 확인: 들킴, 단자 차단, 경계 상승, 판 끝. 마지막에 OnTraceChanged
	void AfterSend(int32 SpotCountBefore);

	// 평균 0, 표준편차 1인 정규분포 난수 (박스-뮬러)
	double NextGaussian();

	// 이 판의 맵과 규칙
	UPROPERTY()
	TObjectPtr<USSTraceConfig> Config = nullptr;

	// 적의 의심 장소들 (날을 넘어 이어짐)
	UPROPERTY()
	TArray<FSSSuspectSpot> Spots;

	// 지금까지 송신한 횟수
	int32 Turn = 0;

	// 받은 횟수 (ReceiveGoal만큼 받으면 수신 100%)
	int32 Received = 0;

	// 고른 단자 번호 (없으면 INDEX_NONE)
	int32 SelectedRelay = INDEX_NONE;

	// 못 쓰는 단자들 (지난날 차단 + 이번 판 차단)
	TArray<int32> BlockedRelays;

	// 이번 판에 새로 차단된 단자들
	TArray<int32> NewlyBlockedRelays;

	// 적의 경계 수준 (오를수록 센서가 정밀해짐)
	int32 AlertLevel = 0;

	// 판 결과
	ESSTraceOutcome Outcome = ESSTraceOutcome::InProgress;

	// 센서 잡음을 만들 난수
	FRandomStream Random;
};
