#include "Companion/SSInvestigation.h"
#include "Companion/SSCompanionLines.h"
#include "Engine/DataTable.h"
#include "Character/SSSurvivorDefinition.h"

FSSInvestigationSpotData FSSInvestigation::GetSpotData(ESSInvestigationSpot Spot)
{
	// 임시값: 중요한 단서가 나오는 곳일수록 위험함
	switch (Spot)
	{
	case ESSInvestigationSpot::TerminalLog: return { 0.40f, 0.30f };
	case ESSInvestigationSpot::Vent:        return { 0.35f, 0.15f };
	case ESSInvestigationSpot::Door:        return { 0.30f, 0.15f };
	case ESSInvestigationSpot::PatrolNoise: return { 0.45f, 0.05f };
	case ESSInvestigationSpot::Storage:     return { 0.50f, 0.03f };
	default:                                return {};
	}
}

FText FSSInvestigation::GetSpotName(ESSInvestigationSpot Spot)
{
	switch (Spot)
	{
	case ESSInvestigationSpot::TerminalLog: return NSLOCTEXT("SSInvestigation", "TerminalLog", "단말 로그");
	case ESSInvestigationSpot::Vent:        return NSLOCTEXT("SSInvestigation", "Vent", "환풍구");
	case ESSInvestigationSpot::Door:        return NSLOCTEXT("SSInvestigation", "Door", "출입문");
	case ESSInvestigationSpot::PatrolNoise: return NSLOCTEXT("SSInvestigation", "PatrolNoise", "순찰 소리");
	case ESSInvestigationSpot::Storage:     return NSLOCTEXT("SSInvestigation", "Storage", "저장고");
	default:                                return FText::GetEmpty();
	}
}

float FSSInvestigation::FindChance(ESSInvestigationSpot Spot, float Thoroughness, float Boldness)
{
	const float Base = GetSpotData(Spot).BaseFindChance;
	return FMath::Clamp(Base * (0.5f + Thoroughness) * (1.f + 0.3f * Boldness), 0.f, 1.f);
}

float FSSInvestigation::DetectChance(ESSInvestigationSpot Spot, float Boldness)
{
	return FMath::Clamp(GetSpotData(Spot).Risk * (0.5f + Boldness), 0.f, 1.f);
}

float FSSInvestigation::GetSpotWeight(const USSSurvivorDefinition& Survivor, ESSInvestigationSpot Spot)
{
	// 선호 (정하지 않은 장소는 0)
	const float* Preference = Survivor.SpotPreference.Find(Spot);
	const float Liking = Preference ? FMath::Max(0.f, *Preference) : 0.f;

	// 위험한 장소를 피하는 정도: 대담할수록 덜 피함
	const float RiskFactor = FMath::Max(MinRiskFactor,
		1.f - GetSpotData(Spot).Risk * (1.f - Survivor.Boldness) * RiskAversion);

	return (BaseSpotWeight + Liking) * RiskFactor;
}

ESSInvestigationSpot FSSInvestigation::PickSpot(const USSSurvivorDefinition& Survivor, FRandomStream& Random,
	const TSet<ESSInvestigationSpot>& ExhaustedSpots)
{
	// 1. 장소마다 무게를 구하고 다 더함
	float Weights[int32(ESSInvestigationSpot::Count)];
	float TotalWeight = 0.f;
	for (int32 Index = 0; Index < int32(ESSInvestigationSpot::Count); ++Index)
	{
		Weights[Index] = GetSpotWeight(Survivor, ESSInvestigationSpot(Index));

		// 다 찾은 장소는 거의 안 감
		if (ExhaustedSpots.Contains(ESSInvestigationSpot(Index))) Weights[Index] *= ExhaustedSpotFactor;
		TotalWeight += Weights[Index];
	}

	// 2. 0 ~ 전체 무게 사이에서 한 점을 뽑고, 그 점이 들어간 장소를 고름
	//    (무게가 큰 장소일수록 차지하는 구간이 길어서 잘 뽑힘)
	float Roll = Random.FRandRange(0.f, TotalWeight);
	for (int32 Index = 0; Index < int32(ESSInvestigationSpot::Count); ++Index)
	{
		if (Roll < Weights[Index]) return ESSInvestigationSpot(Index);
		Roll -= Weights[Index];
	}

	// 소수점 오차로 끝까지 왔으면 마지막 장소
	return ESSInvestigationSpot(int32(ESSInvestigationSpot::Count) - 1);
}

FName FSSInvestigation::PickNextClue(const UDataTable& ClueTable, ESSInvestigationSpot Spot, const TSet<FName>& KnownClueIds)
{
	FName Best = NAME_None;
	int32 BestStage = TNumericLimits<int32>::Max();

	ClueTable.ForeachRow<FSSClueRow>(TEXT("PickNextClue"), [&](const FName& ClueId, const FSSClueRow& Row)
	{
		// 다른 장소 단서, 이미 아는 단서는 건너뜀
		if (Row.Spot != Spot || KnownClueIds.Contains(ClueId)) return;

		// 남은 것 중 단계가 가장 낮은 것
		if (Row.Stage < BestStage)
		{
			BestStage = Row.Stage;
			Best = ClueId;
		}
	});
	return Best;
}

int32 FSSInvestigation::CountClues(const UDataTable& ClueTable, ESSInvestigationSpot Spot)
{
	int32 Count = 0;
	ClueTable.ForeachRow<FSSClueRow>(TEXT("CountClues"), [&](const FName&, const FSSClueRow& Row)
	{
		if (Row.Spot == Spot) ++Count;
	});
	return Count;
}

FSSInvestigationResult FSSInvestigation::Resolve(const USSSurvivorDefinition& Survivor, ESSInvestigationSpot Spot, FRandomStream& Random)
{
	FSSInvestigationResult Result;
	Result.Spot = Spot;
	Result.FindChance = FindChance(Spot, Survivor.Thoroughness, Survivor.Boldness);
	Result.DetectChance = DetectChance(Spot, Survivor.Boldness);

	// 단서 발견과 들킴은 따로 굴림 (찾고도 들킬 수 있음)
	Result.bFoundClue = Random.FRand() < Result.FindChance;
	Result.bDetected = Random.FRand() < Result.DetectChance;
	return Result;
}
