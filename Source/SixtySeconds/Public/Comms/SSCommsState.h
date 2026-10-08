#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Math/RandomStream.h"
#include "Event/SSEventTypes.h"
#include "Trace/SSSuspectMemory.h"
#include "Trace/SSTraceMessage.h"
#include "SSCommsState.generated.h"

class USSRunSubsystem;
class USSTraceSession;
class USSTraceConfig;

// ─────────────────────────────────────────────
// 외부 통신 상태 (역추적 + 받은 메시지)
// 판이 끝나도 날을 넘어 이어지는 것만 보관한다. 한 판의 규칙은 USSTraceSession
// USSRunSubsystem이 소유 (Outer). 행동력·저널·날짜는 RunSubsystem에 요청
// ─────────────────────────────────────────────
UCLASS()
class SIXTYSECONDS_API USSCommsState : public UObject
{
	GENERATED_BODY()

public:
	// ── 역추적 ──

	// 접속할 때 드는 행동력
	static constexpr int32 TraceActionCost = 1;

	// 살아 있고, 오늘 아직 접속 안 했고, 행동력이 있으면 true
	bool CanStartTrace() const;

	// 행동력을 쓰고 오늘 접속했다고 표시. 못 하면 false
	bool BeginTrace();

	// 판이 끝났을 때 세션 결과를 받아 저장 (의심 장소, 받은 횟수, 차단된 단자, 메시지, 저널)
	void FinishTrace(const USSTraceSession& Session);

	// 적의 의심 장소들 (매일 아침 흐려짐)
	const TArray<FSSSuspectSpot>& GetTraceSpots() const { return TraceSpots; }

	// 지금 받고 있는 메시지의 받은 횟수
	int32 GetTraceReceived() const { return TraceReceived; }

	// 지금 차단돼서 못 쓰는 통신 단자 번호들 (차단은 며칠 뒤 풀림)
	TArray<int32> GetBlockedTraceRelays() const;

	// ── 받은 메시지 ──

	// 다 받은 메시지 수
	int32 GetCompletedMessages() const { return CompletedMessages; }

	// 해독 대기함 (다 받았지만 아직 못 푼 메시지들)
	const TArray<FSSPendingMessage>& GetPendingMessages() const { return PendingMessages; }

	// 대기함의 PendingIndex번째 메시지 해독 성공: 내용 공개, 효과 적용, 저널 기록, 대기함에서 제거
	// 번호가 잘못됐으면 false
	bool DecodeMessage(int32 PendingIndex);

	// 대기함 메시지의 다이얼 위치 저장 (해독 창을 닫아도 이어서 풀 수 있게)
	void SaveDials(int32 PendingIndex, const TArray<int32>& Dials);

	// 해독 실패: 대기함에서 메시지를 버림 (효과 없음, 저널에 기록). 번호가 잘못됐으면 아무 일 없음
	void DiscardMessage(int32 PendingIndex);

	// 진실 통신 추측표와 자동 해독 여부 저장 (창을 닫아도 이어서 풀 수 있게)
	void SaveGuess(int32 PendingIndex, const TArray<int32>& Guess, bool bSolverDone);

	// 마지막으로 해독한 메시지 (Title = 제목, Lines[0] = 원문, Changes = 실제로 바뀐 것)
	const FSSEventResult& GetLastMessage() const { return LastMessage; }

	// 지금까지 받은 진실 단서 수 (엔딩 조건에 쓸 예정)
	int32 GetTruthCluesFound() const { return TruthCluesFound; }

	// 메시지 뽑기 난수 고정 (테스트용)
	void SetMessageSeed(int32 Seed) { MessageRandom.Initialize(Seed); }

	// ── RunSubsystem이 부름 ──

	// 하룻밤 지남: 적의 기억이 흐려지고, 기한 지난 메시지가 사라짐
	void OnNewDay();

	// 새 게임: 전부 초기화
	void ResetRun();

private:
	// 소유자
	USSRunSubsystem& GetRun() const;

	// 다 받은 메시지 하나를 골라 해독 대기함에 넣음
	void EnqueueMessage(const USSTraceConfig* Config);

	// 적의 의심 장소들
	UPROPERTY(Transient)
	TArray<FSSSuspectSpot> TraceSpots;

	// 지금 받고 있는 메시지의 받은 횟수
	UPROPERTY(Transient)
	int32 TraceReceived = 0;

	// 차단된 통신 단자 → 차단이 풀리는 날
	UPROPERTY(Transient)
	TMap<int32, int32> TraceRelayBlockedUntil;

	// 마지막으로 접속한 날 (하루 1회)
	UPROPERTY(Transient)
	int32 LastTraceDay = 0;

	// 다 받은 메시지 수
	UPROPERTY(Transient)
	int32 CompletedMessages = 0;

	// 이미 받은 메시지들 (다시 안 나옴)
	UPROPERTY(Transient)
	TSet<FName> ReceivedMessages;

	// 받은 진실 단서 수
	UPROPERTY(Transient)
	int32 TruthCluesFound = 0;

	// 마지막으로 다 받은 메시지 결과
	FSSEventResult LastMessage;

	// 해독 대기함 (다 받았지만 아직 못 푼 메시지들)
	UPROPERTY(Transient)
	TArray<FSSPendingMessage> PendingMessages;

	// 실용 정보를 뽑을 난수
	FRandomStream MessageRandom = FRandomStream(FPlatformTime::Cycles());
};
