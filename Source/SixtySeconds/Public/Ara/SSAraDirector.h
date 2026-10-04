#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Math/RandomStream.h"
#include "Engine/DataTable.h"
#include "Companion/SSInvestigation.h"
#include "SSAraDirector.generated.h"

class USSRunSubsystem;

// 아라 대사 표(SS_AraLines) 한 줄. 행 이름 = 종류_번호 (예: Daily_3)
// 같은 종류가 여러 줄이면 날짜마다 돌아가며 골라서 매일 같은 말을 하지 않음
// 문장 안의 {Day} {Food} {Water} {Name} {Count}는 값으로 바뀜
USTRUCT(BlueprintType)
struct SIXTYSECONDS_API FSSAraLineRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Ara", meta=(MultiLine=true))
	FText Line;
};

// 아라가 동료 한 명을 얼마나 의심하는지
USTRUCT()
struct FSSAraSuspicion
{
	GENERATED_BODY()

	// 어느 동료인지
	UPROPERTY()
	FName SurvivorId = NAME_None;

	// "이 사람이 나를 조사하고 있다"는 믿음 (로그 오즈. 0이면 반반, 음수면 아닐 쪽)
	UPROPERTY()
	float LogOdds = 0.f;

	// 아라가 직접 본 밤 활동 횟수 (브리핑 문장용)
	UPROPERTY()
	int32 DetectionsSeen = 0;

	// 아라가 엿들은 보고 횟수 (플레이어에게 말하지 않음)
	UPROPERTY()
	int32 EavesdropsHeard = 0;
};

// ─────────────────────────────────────────────
// 아라의 판단: 누가 나를 캐고 있나
// 아라는 동료가 단서를 몇 개 찾았는지 모른다. 자기가 본 증거만으로 추측한다
//   - 밤에 조사하다 들킴: 그 장소에 있을 핑계가 없을수록 크게 의심 (저장고 < 단말 로그)
//   - 조용한 밤: 조금 덜 의심
//   - 은신처 대화를 엿들음: 보고를 듣는 순간 확률로 (단말 로그 1단계 단서와 연결)
//   - 매일 조금씩 잊음 (처음 믿음 쪽으로 돌아감)
// 계산: 베이즈 갱신. 믿음을 로그 오즈로 들고, 증거마다 가능도 비율의 로그를 더함
// 위협도 = 의심 확률 × 동료의 영향력. 기준을 넘으면 표적 (아침 브리핑이 파견을 권함)
// ─────────────────────────────────────────────
UCLASS()
class SIXTYSECONDS_API USSAraDirector : public UObject
{
	GENERATED_BODY()

public:
	// ── 밤 (RunSubsystem이 부름) ──

	// 하룻밤 시작: 모든 의심이 처음 믿음 쪽으로 조금 돌아감
	void BeginNight();

	// 동료 한 명의 밤 조사 결과를 아라가 본 만큼 반영 (들켰으면 그 장소로, 아니면 조용한 밤)
	void ObserveNight(FName SurvivorId, ESSInvestigationSpot Spot, bool bDetected);

	// 밤이 끝남: 위협도로 표적을 정하거나 풂
	void UpdateTarget();

	// ── 낮 (동료 대화가 부름) ──

	// 보고를 듣는 순간 아라가 엿들을지 굴림. 엿들었으면 의심을 올리고 true
	bool TryEavesdrop(FName SurvivorId, bool bFoundClue);

	// 엿들은 보고 하나를 반영 (굴림 없이. 테스트와 TryEavesdrop이 씀)
	void ObserveEavesdrop(FName SurvivorId, bool bFoundClue);

	// ── 읽기 ──

	// 의심 확률 0~1 (기록이 없으면 처음 믿음)
	float GetSuspicion(FName SurvivorId) const;

	// 위협도 = 의심 확률 × 영향력
	float GetThreat(FName SurvivorId) const;

	// 지금 표적 (없으면 NAME_None)
	FName GetTarget() const { return TargetId; }
	bool HasTarget() const { return !TargetId.IsNone(); }

	// 아침 브리핑 전체 (날짜·물자 한 줄 + 의심받는 동료·표적 문장)
	FText BuildBriefing() const;

	// 아침 브리핑에 붙일 문장 (의심받는 동료·표적). 없으면 빈 문장
	FText BuildBriefingNotes() const;

	// 대사 표 연결 (HUD가 은신처를 열 때). 줄 형식이 FSSAraLineRow가 아니면 쓰지 않음
	// 표가 없으면 코드 안의 기본 문장을 씀
	void SetLineTable(UDataTable* InLineTable);
	bool HasLineTable() const { return LineTable != nullptr; }

	// 새 게임
	void ResetRun();

	// 엿듣기 난수 고정 (테스트용)
	void SetSeed(int32 Seed) { Random.Initialize(Seed); }

	// ── 계산 (기억 없이 숫자만) ──

	static float ToLogOdds(float Probability);
	static float ToProbability(float LogOdds);

	// 그 장소에서 들켰을 때의 가능도 비율 = 1 / (조사하지 않는 사람이 그곳에 있을 핑계)
	static float DetectionRatio(ESSInvestigationSpot Spot);

	// 엿들을 확률 = 기본 + 학습도 × 증가폭 (최대치에서 멈춤)
	static float EavesdropChance(int32 LearningScore);

	// ── 수치 (임시값, 플레이 후 조정) ──

	// 처음 믿음: 누구든 10%는 의심
	static constexpr float PriorSuspicion = 0.1f;

	// 하룻밤마다 처음 믿음과의 차이 중 남는 비율 (나머지는 잊음)
	static constexpr float DailyKeep = 0.85f;

	// 조용한 밤의 가능도 비율 (조사 중이라면 들켰을 법한데 안 들킴 → 조금 덜 의심)
	static constexpr float QuietNightRatio = 0.85f;

	// 엿들은 보고의 가능도 비율 (단서 있는 보고 / 없는 보고)
	static constexpr float EavesdropClueRatio = 8.f;
	static constexpr float EavesdropPlainRatio = 2.f;

	// 엿들을 확률: 기본, 학습도 1마다 증가, 최대
	static constexpr float EavesdropBaseChance = 0.25f;
	static constexpr float EavesdropPerLearning = 0.05f;
	static constexpr float EavesdropMaxChance = 0.6f;

	// 브리핑에서 언급하기 시작하는 의심 확률 (본 것이 있을 때만: 야간 활동 n회)
	static constexpr float NoticeSuspicion = 0.35f;

	// 이보다 높으면 말투가 바뀜 ("행동 패턴이 기준에서 벗어나고 있습니다")
	static constexpr float ConcernSuspicion = 0.55f;

	// 표적이 되는 위협도 / 표적에서 풀리는 위협도 (사이에선 상태 유지 → 매일 바뀌지 않게)
	static constexpr float TargetThreat = 0.7f;
	static constexpr float ReleaseThreat = 0.4f;

private:
	USSRunSubsystem& GetRun() const;

	const FSSAraSuspicion* FindSuspicion(FName SurvivorId) const;
	FSSAraSuspicion& FindOrAddSuspicion(FName SurvivorId);

	// 동료의 영향력 (데이터 에셋 값. 없으면 1)
	float GetInfluence(FName SurvivorId) const;

	// 살아 있는 구조된 동료인지
	bool IsAliveSurvivor(FName SurvivorId) const;

	// 대사 표에서 "종류_번호" 줄 중 하나를 골라 값을 채움
	// Seed가 같으면 같은 줄 (날짜·이름으로 정해서 같은 날 다시 만들어도 안 바뀜)
	// 표에 그 종류가 없으면 Fallback을 씀
	FText PickLine(const TCHAR* Kind, int32 Seed, const FFormatNamedArguments& Args, const FText& Fallback) const;

	// 아라 대사 표
	UPROPERTY(Transient)
	TObjectPtr<UDataTable> LineTable;

	// 동료별 의심
	UPROPERTY(Transient)
	TArray<FSSAraSuspicion> Suspicions;

	// 지금 표적
	UPROPERTY(Transient)
	FName TargetId = NAME_None;

	// 엿듣기 굴림
	FRandomStream Random = FRandomStream(FPlatformTime::Cycles());
};
