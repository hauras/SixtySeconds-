#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Math/RandomStream.h"
#include "Companion/SSInvestigation.h"
#include "Companion/SSCompanionLines.h"
#include "SSCompanionState.generated.h"

class USSRunSubsystem;
class UDataTable;

// 조사 보고 하나 (동료가 밤에 한 번 조사한 결과)
USTRUCT()
struct FSSInvestigationReport
{
	GENERATED_BODY()

	// 누가 조사했는지 (동료 이름표)
	UPROPERTY()
	FName SurvivorId = NAME_None;

	// 며칠째 밤에 조사했는지
	UPROPERTY()
	int32 Day = 0;

	// 어디를 조사했는지
	UPROPERTY()
	ESSInvestigationSpot Spot = ESSInvestigationSpot::Storage;

	// 단서를 찾았는지
	UPROPERTY()
	bool bFoundClue = false;

	// 찾은 단서의 ID (단서 표의 행 이름). 단서 표가 없으면 비어 있음
	UPROPERTY()
	FName ClueId = NAME_None;

	// 발견 판정은 성공했지만 그 장소 단서를 이미 다 알아서 새로 나온 게 없음
	UPROPERTY()
	bool bSpotExhausted = false;

	// 플레이어가 이 보고를 들은 날 (안 들었으면 0)
	UPROPERTY()
	int32 HeardDay = 0;
};

// 동료 한 명의 조사 기록 (판마다 바뀌는 값. 체력·배고픔 같은 몸 상태는 FSSSurvivorState에)
USTRUCT()
struct FSSCompanionRecord
{
	GENERATED_BODY()

	// 어느 동료의 기록인지 (동료 이름표)
	UPROPERTY()
	FName SurvivorId = NAME_None;

	// 이 동료가 지금까지 찾은 단서 수 (아직 말 안 한 것 포함. 아라가 판단할 때 씀)
	UPROPERTY()
	int32 CluesFound = 0;

	// 조사하다 아라에게 들킨 횟수 (플레이어에게 안 보임. 아라가 판단할 때 씀)
	UPROPERTY()
	int32 TimesDetected = 0;

	// 아직 안 들은 보고들 (오래된 것이 앞)
	// 단서가 있는 보고는 들을 때까지 남고, 단서가 없는 보고는 가장 최근 것 하나만 남음
	UPROPERTY()
	TArray<FSSInvestigationReport> PendingReports;

	// 플레이어가 다음 밤 조사 장소를 정해줬는지
	UPROPERTY()
	bool bHasOrder = false;

	// 정해준 장소
	UPROPERTY()
	ESSInvestigationSpot OrderedSpot = ESSInvestigationSpot::Storage;

	// 아라가 바꿔치기한 안드로이드인지 (숨은 값. 화면에 절대 안 나옴)
	UPROPERTY()
	bool bIsAndroid = false;

	// 마지막으로 검사한 날 (안 했으면 0). 검사는 그날의 사진일 뿐 (그 뒤에 바뀔 수 있음)
	UPROPERTY()
	int32 InspectedDay = 0;

	// 그 검사에서 안드로이드로 나왔는지
	UPROPERTY()
	bool bInspectedAndroid = false;

	// B2에서 구출돼 아직 증언을 안 들었나
	UPROPERTY()
	bool bPendingTestimony = false;

	// 안 들은 보고가 있나
	bool HasPendingReport() const { return PendingReports.Num() > 0; }

	// 할 얘기가 있나 (보고 또는 증언 → 머리 위 "!" 표시)
	bool HasSomethingToSay() const { return HasPendingReport() || bPendingTestimony; }
};

// ─────────────────────────────────────────────
// 동료 조사 기록
// 동료가 밤에 어디를 조사했는지, 어떤 보고가 밀려 있는지, 플레이어가 어떤 단서를 들었는지 기억한다
// 동료마다 기록이 하나씩 있고, 동료 ID(이름표)로 찾는다
// 체력·배고픔 같은 몸 상태는 여기 말고 RunSubsystem에 있다
// 행동력을 쓰거나 기록창에 적을 때는 RunSubsystem에 부탁한다
// ─────────────────────────────────────────────
UCLASS()
class SIXTYSECONDS_API USSCompanionState : public UObject
{
	GENERATED_BODY()

public:
	// 조사 장소를 정해줄 때 드는 행동력
	static constexpr int32 InvestigationOrderCost = 1;

	// 다음 밤 조사 장소를 정해줌. 살아 있고, 오늘 아직 안 정했고, 행동력이 있으면 행동력 1을 쓰고 true
	bool OrderInvestigation(FName SurvivorId, ESSInvestigationSpot Spot);

	// 밀린 보고 중 가장 오래된 것 하나를 들음 (대화). 들을 게 없으면 false
	// 들은 보고는 OutReport에 담아줌. 단서가 있으면 "들은 단서" 목록에 들어감
	bool HearInvestigationReport(FName SurvivorId, FSSInvestigationReport& OutReport);

	// 플레이어가 대화로 들은 단서들 (들은 순서대로). 추리·사건에서 씀
	const TArray<FSSInvestigationReport>& GetHeardClues() const { return HeardClues; }

	// 동료 기록을 읽기만 함 (화면에서 "!" 표시 등). 없으면 nullptr
	const FSSCompanionRecord* FindRecord(FName SurvivorId) const;

	// ── 안드로이드 (아라가 부름) ──

	// 이 동료를 안드로이드로 바꿈. 이후 조사하는 척만 함. 진짜가 못 한 보고는 사라짐
	void MakeAndroid(FName SurvivorId);

	// 안드로이드인지 (아라·테스트용. 화면에서 쓰지 말 것)
	bool IsAndroid(FName SurvivorId) const;

	// B2에서 진짜를 구해 왔을 때: 안드로이드 표시와 그동안의 검사 결과·가짜 보고를 지움
	void RestoreHuman(FName SurvivorId);

	// 플레이어가 대화로 들은 단서인지
	bool HasHeardClue(FName ClueId) const;

	// B2에서 구출된 동료가 다음 대화에서 증언하게 함
	void QueueTestimony(FName SurvivorId);

	// 증언 듣기: 대사를 OutLine에 담고 증언 대기를 지움. 들을 게 없으면 false
	bool HearTestimony(FName SurvivorId, FText& OutLine);

	// 단서를 들은 것으로 바로 기록 (보고할 동료가 없을 때·테스트·디버그 명령)
	void HearClueDirectly(FName ClueId);

	// 그 단서를 플레이어가 들었거나, 은신처의 살아 있는 사람 동료가 보고하려고 들고 있나
	// (붙잡힌 사람·안드로이드가 들고 있던 보고는 플레이어에게 닿지 않으므로 안 셈)
	bool IsClueKnownOrPending(FName ClueId) const;

	// 동료에게 단서 보고 하나를 바로 맡김 (밤 조사를 거치지 않는 고정 단서. 다음 대화에서 들음)
	void QueueClueReport(FName SurvivorId, ESSInvestigationSpot Spot, FName ClueId);

	// 정해준 장소가 있을 때 안드로이드가 다른 장소를 조사했다고 말할 확률 (플레이어가 눈치챌 단서)
	static constexpr float AndroidWrongSpotChance = 0.3f;

	// ── 정체 판별 (정보창 버튼) ──

	// 검사 비용: 행동력 1 + 배터리 1
	static constexpr int32 InspectActionCost = 1;
	static constexpr int32 InspectBatteryCost = 1;

	// 안드로이드가 검사에서 정상으로 위장할 확률 (사람이 이상으로 나오는 일은 없음)
	static constexpr float AndroidMaskChance = 0.4f;

	// 검사할 수 있나 (살아 있고, 행동력·배터리가 있음)
	bool CanInspect(FName SurvivorId) const;

	// 검사: 비용을 쓰고 결과를 기록 (정보창 관찰 칸에 보임). 못 하면 false
	bool InspectCompanion(FName SurvivorId);

	// 격리: 되돌릴 수 없음. 안드로이드면 제거, 사람이면 B2로 붙잡혀 감 (아라가 원하던 것)
	// 결과는 bOutWasAndroid로. 못 하면 false
	bool IsolateCompanion(FName SurvivorId, bool& bOutWasAndroid);

	// 검사 결과 문장 (기록창·정보창 공용)
	static FText GetInspectionText(bool bAndroid);

	// ── 단서·대사 표 ──

	// 단서 표와 대사 표 연결 (HUD가 은신처를 열 때). 단서 표가 없으면 ClueId 없이 찾음/못 찾음만 기록
	void SetTables(UDataTable* InClueTable, UDataTable* InLineTable);

	// 단서 한 줄. 없으면 nullptr
	const FSSClueRow* FindClue(FName ClueId) const;

	// 그 장소의 단서 개수 ("2/3" 표시용). 단서 표가 없으면 0
	int32 CountClues(ESSInvestigationSpot Spot) const;

	// 보고 대사 (동료ID_Report_장소_Found/None, 다 찾은 장소면 동료ID_Exhausted)
	FText GetReportLine(const FSSInvestigationReport& Report) const;

	// 공통 대사 (동료ID_종류). {Spot}은 Spot 이름으로 바뀜
	FText GetLine(FName SurvivorId, ESSCompanionLine LineType, ESSInvestigationSpot Spot = ESSInvestigationSpot::Count) const;

	// ── RunSubsystem이 부름 ──

	// 밤: 살아 있는 동료마다 조사 한 번
	void RunNight();

	// 새 게임: 기록 전부 지움
	void ResetRun();

	// 조사 난수 고정 (테스트용)
	void SetInvestigationSeed(int32 Seed) { InvestigationRandom.Initialize(Seed); }

private:
	// 이 기록을 들고 있는 RunSubsystem을 찾아줌
	USSRunSubsystem& GetRun() const;

	// 동료 기록을 찾고, 없으면 새로 만들어서 돌려줌
	FSSCompanionRecord& FindOrAddRecord(FName SurvivorId);

	// 아침에 들을 보고 하나를 추가 (단서 없는 지난 보고는 이걸로 대체)
	void AddReport(FSSCompanionRecord& Record, ESSInvestigationSpot Spot, bool bFoundClue, FName ClueId, bool bSpotExhausted);

	// 이미 아는 단서 ID: 플레이어가 들은 것 + 동료들이 보고하려고 들고 있는 것
	TSet<FName> GetKnownClueIds() const;

	// 대사 표에서 행 이름으로 찾아 {Spot}을 채움. 없으면 행 이름을 괄호로 보여줌 (빠진 대사가 눈에 띄게)
	FText FindLine(const FString& RowName, ESSInvestigationSpot Spot) const;

	// 동료별 기록 목록
	UPROPERTY(Transient)
	TArray<FSSCompanionRecord> Records;

	// 플레이어가 들은 단서 보고 (모든 동료 것을 한곳에)
	UPROPERTY(Transient)
	TArray<FSSInvestigationReport> HeardClues;

	// 단서 표 (SS_Clues)
	UPROPERTY(Transient)
	TObjectPtr<UDataTable> ClueTable;

	// 대사 표 (SS_CompanionLines)
	UPROPERTY(Transient)
	TObjectPtr<UDataTable> LineTable;

	// 조사 결과를 굴리는 난수
	FRandomStream InvestigationRandom = FRandomStream(FPlatformTime::Cycles());
};
