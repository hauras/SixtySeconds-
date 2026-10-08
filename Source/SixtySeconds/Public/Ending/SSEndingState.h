#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Ending/SSEndingTypes.h"
#include "SSEndingState.generated.h"

class USSRunSubsystem;

// ─────────────────────────────────────────────
// 이야기의 끝 (판마다 바뀌는 값)
// 플레이어가 알게 된 숨은 진실(운영진의 폐기 결정과 정화 프로토콜)과 마지막 밤에 정해진 엔딩
// ─────────────────────────────────────────────
UCLASS()
class SIXTYSECONDS_API USSEndingState : public UObject
{
	GENERATED_BODY()

public:
	// 이 날이 되는 밤에 서버실 사건이 확률과 상관없이 옴
	static constexpr int32 FinalDay = 12;

	// ── 숨은 진실 (반전 엔딩 조건) ──

	// 알게 된 숨은 진실 추가. 처음 알았으면 true (기록창에 남김)
	bool AddHiddenTruth(FName TruthId, const FText& JournalLine);

	bool HasHiddenTruth(FName TruthId) const { return HiddenTruths.Contains(TruthId); }
	int32 CountHiddenTruths() const { return HiddenTruths.Num(); }

	// 패널을 한 번이라도 열었나 (패널 기록 = 숨은 진실 1)
	bool HasSeenPanelLog() const;

	// ── 엔딩 ──

	// 서버실 선택의 결과를 엔딩으로 확정 (한 번만). 하린을 데려가 종료하려 했는데 하린이 안드로이드면 지배로 바뀜
	void ReachEnding(ESSEnding Requested);

	bool HasEnded() const { return Report.Ending != ESSEnding::None; }
	const FSSEndingReport& GetReport() const { return Report; }

	// 새 판
	void ResetRun();

private:
	// 이 상태를 들고 있는 RunSubsystem
	USSRunSubsystem& GetRun() const;

	// 알게 된 숨은 진실 (알게 된 순서)
	UPROPERTY(Transient)
	TArray<FName> HiddenTruths;

	// 정해진 엔딩 (None이면 진행 중)
	FSSEndingReport Report;
};
