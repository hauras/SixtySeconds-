#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Rescue/SSRescueSession.h"
#include "SSRescueState.generated.h"

class USSRunSubsystem;

// ─────────────────────────────────────────────
// B2 구출 상태 (판마다 바뀌는 값)
// 패널을 열 수 있는지, 진행 중인 패널 작업, 구출 결과 반영, 엔딩용 기록
// 붙잡힌 동료 목록 자체는 동료 명단이라 RunSubsystem에 있고, 여기서는 그걸 읽고 귀환을 부탁함
// ─────────────────────────────────────────────
UCLASS()
class SIXTYSECONDS_API USSRescueState : public UObject
{
	GENERATED_BODY()

public:
	// 패널을 열 수 있는 최소 행동력 (패널 작업은 남은 행동력을 전부 씀)
	static constexpr int32 MinActionPoints = 2;

	// 패널을 못 여는 이유 (None이면 열 수 있음)
	ESSRescueBlock GetBlock() const;

	// 패널 작업 시작: 남은 행동력을 전부 쓰고 세션을 만듦. 못 하면 nullptr
	// 회전 수 = 행동력 × 8 (태오가 은신처에 있으면) / × 4 (없으면). 태오가 안드로이드면 시작부터 경보 1회
	// Seed 0이면 무작위 퍼즐
	USSRescueSession* Start(FName TargetId, int32 Seed = 0);

	// 진행 중인 패널 작업 (없으면 nullptr)
	USSRescueSession* GetActive() const { return Active; }

	// 끝난 패널 작업을 반영: 성공이면 동료 귀환, 경보만큼 도운 동료 의심 상승, 기록. 반영했으면 true
	// 한 번만 반영됨 (반영 후 진행 중 작업을 비우므로 다시 부르면 false)
	bool Finish(FSSRescueReport& OutReport);

	// 엔딩 카드 기록용
	int32 GetSuccessCount() const { return SuccessCount; }
	int32 GetAlarmTotal() const { return AlarmTotal; }

	// 새 판
	void ResetRun();

private:
	// 이 상태를 들고 있는 RunSubsystem
	USSRunSubsystem& GetRun() const;

	// 진행 중인 패널 작업
	UPROPERTY(Transient)
	TObjectPtr<USSRescueSession> Active;

	// 패널 작업을 도운 동료 (태오, 혼자면 None). 경보 의심을 받을 사람
	FName HelperId = NAME_None;

	// 마지막으로 패널을 연 날 (하루 한 번)
	int32 LastDay = 0;

	// 이번 패널 작업에 쓴 행동력 (결과 카드용)
	int32 SpentActionPoints = 0;

	// 구출 성공 수, 패널 경보 합계 (엔딩 기록)
	int32 SuccessCount = 0;
	int32 AlarmTotal = 0;
};
