#pragma once

#include "CoreMinimal.h"
#include "Math/RandomStream.h"
#include "SSInvestigation.generated.h"

class USSSurvivorDefinition;
class UDataTable;

// 동료가 밤에 조사하는 장소
UENUM(BlueprintType)
enum class ESSInvestigationSpot : uint8
{
	TerminalLog,   // 단말 로그: 아라 행동 기록 (가장 중요한 단서, 아라가 직접 봄)
	Vent,          // 환풍구: 정비 드론 경로, 숨겨진 통로
	Door,          // 출입문: 밤에 누가 오갔는지
	PatrolNoise,   // 순찰 소리: 로봇 순찰 시간
	Storage,       // 저장고: 물자
	Count UMETA(Hidden)
};

// 장소 하나의 기본 수치 (임시값, 플레이 후 조정)
struct FSSInvestigationSpotData
{
	// 기본 단서 발견 확률 (꼼꼼함 0.5·대담함 0인 동료 기준)
	float BaseFindChance = 0.f;

	// 위험도: 이 장소를 뒤지는 걸 아라가 알아챌 기본 확률
	float Risk = 0.f;
};

// 조사 한 번의 결과
struct FSSInvestigationResult
{
	ESSInvestigationSpot Spot = ESSInvestigationSpot::Storage;

	// 단서를 찾았나
	bool bFoundClue = false;

	// 아라에게 들켰나 (플레이어에게는 바로 알려주지 않음)
	bool bDetected = false;

	// 이번 판정에 쓴 확률 (디버그·테스트용)
	float FindChance = 0.f;
	float DetectChance = 0.f;
};

// ─────────────────────────────────────────────
// 동료 조사 계산
// 꼼꼼함 = 단서를 잘 찾음, 대담함 = 위험한 곳을 덜 피하고 더 깊이 뒤짐(단서↑·들킬 위험↑)
// 두 값은 동료마다 거의 고정된 내부 성향. 플레이어에게 숫자로 보여주지 않음
// 부모 없음. 기억 없이 계산만 하는 함수 모음
// ─────────────────────────────────────────────
class SIXTYSECONDS_API FSSInvestigation
{
public:
	// 장소 기본 수치
	static FSSInvestigationSpotData GetSpotData(ESSInvestigationSpot Spot);

	// 장소 이름 (기록·화면용)
	static FText GetSpotName(ESSInvestigationSpot Spot);

	// 단서 발견 확률 = 기본 확률 × (0.5 + 꼼꼼함) × (1 + 0.3 × 대담함), 0~1로 자름
	static float FindChance(ESSInvestigationSpot Spot, float Thoroughness, float Boldness);

	// 들킬 확률 = 위험도 × (0.5 + 대담함), 0~1로 자름
	static float DetectChance(ESSInvestigationSpot Spot, float Boldness);

	// 장소 하나의 추첨 무게 (클수록 자주 뽑힘, 항상 0보다 큼)
	//   무게 = (기본 무게 + 선호) × 위험 회피
	//   위험 회피 = 1 − 위험도 × (1 − 대담함) × RiskAversion  (최소 MinRiskFactor)
	//   대담할수록 위험한 장소를 덜 피함. 선호가 없는 장소도 기본 무게가 있어 가끔 뽑힘
	static float GetSpotWeight(const USSSurvivorDefinition& Survivor, ESSInvestigationSpot Spot);

	// 오늘 조사할 장소를 무게대로 추첨 (좋아하는 곳이 자주, 다른 곳도 가끔)
	// ExhaustedSpots: 단서를 다 찾은 장소. 무게에 ExhaustedSpotFactor를 곱해 거의 안 가게 함
	static ESSInvestigationSpot PickSpot(const USSSurvivorDefinition& Survivor, FRandomStream& Random,
		const TSet<ESSInvestigationSpot>& ExhaustedSpots = TSet<ESSInvestigationSpot>());

	// 그 장소에서 다음에 나올 단서 ID (단서 표의 행 이름)
	// KnownClueIds(이미 들었거나 누군가 보고하려고 들고 있는 단서)를 빼고 Stage가 가장 작은 것
	// 남은 단서가 없으면 NAME_None
	static FName PickNextClue(const UDataTable& ClueTable, ESSInvestigationSpot Spot, const TSet<FName>& KnownClueIds);

	// 그 장소의 단서 개수 (화면의 "2/3" 표시용)
	static int32 CountClues(const UDataTable& ClueTable, ESSInvestigationSpot Spot);

	// 조사 한 번 판정 (단서 발견과 들킴은 따로 굴림)
	static FSSInvestigationResult Resolve(const USSSurvivorDefinition& Survivor, ESSInvestigationSpot Spot, FRandomStream& Random);

	// 위험을 얼마나 피하는지 (위험도 0.3인 단말 로그: 겁쟁이는 무게가 0.1배까지 줄어듦)
	static constexpr float RiskAversion = 3.f;

	// 위험 회피로 줄어드는 최소 배율 (아무리 겁 많아도 0이 되지 않게)
	static constexpr float MinRiskFactor = 0.1f;

	// 선호를 정하지 않은 장소의 기본 무게 (선호 3인 곳보다 훨씬 드물게 뽑힘)
	static constexpr float BaseSpotWeight = 0.5f;

	// 단서를 다 찾은 장소의 무게 배율 (0이면 아예 안 감. 가끔은 가도록 작게만 줄임)
	static constexpr float ExhaustedSpotFactor = 0.15f;
};
