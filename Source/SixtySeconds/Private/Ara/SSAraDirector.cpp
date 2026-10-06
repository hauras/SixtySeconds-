#include "Ara/SSAraDirector.h"
#include "Character/SSSurvivorDefinition.h"
#include "Item/SSItemDefinition.h"
#include "Item/SSRunSubsystem.h"
#include "Companion/SSCompanionState.h"
#include "Event/SSEventDirector.h"

USSRunSubsystem& USSAraDirector::GetRun() const
{
	return *CastChecked<USSRunSubsystem>(GetOuter());
}

// ── 계산 ──

float USSAraDirector::ToLogOdds(float Probability)
{
	// 0이나 1이면 무한대가 되므로 살짝 안쪽으로 자름
	const float Clamped = FMath::Clamp(Probability, 1e-4f, 1.f - 1e-4f);
	return FMath::Loge(Clamped / (1.f - Clamped));
}

float USSAraDirector::ToProbability(float LogOdds)
{
	return 1.f / (1.f + FMath::Exp(-LogOdds));
}

float USSAraDirector::DetectionRatio(ESSInvestigationSpot Spot)
{
	// 조사하지 않는 사람이 밤에 그곳에 있을 핑계 (임시값)
	//   저장고는 배고파서 갔다고 할 수 있지만, 단말 로그 앞에 있을 이유는 거의 없음
	float InnocentReason = 1.f;
	switch (Spot)
	{
	case ESSInvestigationSpot::TerminalLog: InnocentReason = 0.1f;  break;
	case ESSInvestigationSpot::Vent:        InnocentReason = 0.2f;  break;
	case ESSInvestigationSpot::Door:        InnocentReason = 0.35f; break;
	case ESSInvestigationSpot::PatrolNoise: InnocentReason = 0.5f;  break;
	case ESSInvestigationSpot::Storage:     InnocentReason = 0.7f;  break;
	default: break;
	}
	return 1.f / InnocentReason;
}

float USSAraDirector::EavesdropChance(int32 LearningScore)
{
	return FMath::Min(EavesdropMaxChance, EavesdropBaseChance + EavesdropPerLearning * FMath::Max(0, LearningScore));
}

// ── 기록 찾기 ──

const FSSAraSuspicion* USSAraDirector::FindSuspicion(FName SurvivorId) const
{
	for (const FSSAraSuspicion& Each : Suspicions)
	{
		if (Each.SurvivorId == SurvivorId) return &Each;
	}
	return nullptr;
}

FSSAraSuspicion& USSAraDirector::FindOrAddSuspicion(FName SurvivorId)
{
	for (FSSAraSuspicion& Each : Suspicions)
	{
		if (Each.SurvivorId == SurvivorId) return Each;
	}

	// 처음 보는 동료는 처음 믿음에서 시작
	FSSAraSuspicion& NewOne = Suspicions.AddDefaulted_GetRef();
	NewOne.SurvivorId = SurvivorId;
	NewOne.LogOdds = ToLogOdds(PriorSuspicion);
	return NewOne;
}

bool USSAraDirector::IsAliveSurvivor(FName SurvivorId) const
{
	const FSSSurvivorState* Survivor = GetRun().FindRescuedSurvivor(SurvivorId);
	return Survivor && Survivor->bAlive;
}

float USSAraDirector::GetInfluence(FName SurvivorId) const
{
	const FSSSurvivorState* Survivor = GetRun().FindRescuedSurvivor(SurvivorId);
	return Survivor && IsValid(Survivor->Definition) ? Survivor->Definition->AraInfluence : 1.f;
}

// ── 밤 ──

void USSAraDirector::BeginNight()
{
	// 처음 믿음과의 차이를 DailyKeep만큼만 남김 (의심도, 믿음도 서서히 보통으로)
	const float Prior = ToLogOdds(PriorSuspicion);
	for (FSSAraSuspicion& Each : Suspicions)
	{
		Each.LogOdds = Prior + (Each.LogOdds - Prior) * DailyKeep;
	}
}

void USSAraDirector::ObserveNight(FName SurvivorId, ESSInvestigationSpot Spot, bool bDetected)
{
	// 자기 편(안드로이드)은 관찰할 필요가 없음
	if (IsAndroid(SurvivorId)) return;

	FSSAraSuspicion& Suspicion = FindOrAddSuspicion(SurvivorId);

	// 베이즈 갱신: 로그 오즈에 가능도 비율의 로그를 더함
	if (bDetected)
	{
		Suspicion.LogOdds += FMath::Loge(DetectionRatio(Spot));
		++Suspicion.DetectionsSeen;
	}
	else
	{
		Suspicion.LogOdds += FMath::Loge(QuietNightRatio);
	}
}

void USSAraDirector::UpdateTarget()
{
	// 이미 한 명을 바꿨으면 다음 단계(모함)까지 새 표적을 잡지 않음
	if (bSwapDone)
	{
		TargetId = NAME_None;
		return;
	}

	const FName Before = TargetId;

	// 지금 표적이 살아 있고 위협도가 풀릴 만큼 내려가지 않았으면 유지
	const bool bKeep = HasTarget() && IsAliveSurvivor(TargetId) && GetThreat(TargetId) >= ReleaseThreat;
	if (!bKeep)
	{
		TargetId = NAME_None;

		// 기준을 넘은 동료 중 위협도가 가장 높은 사람
		float BestThreat = TargetThreat;
		for (const FSSAraSuspicion& Each : Suspicions)
		{
			if (!IsAliveSurvivor(Each.SurvivorId) || IsAndroid(Each.SurvivorId)) continue;

			const float Threat = GetThreat(Each.SurvivorId);
			if (Threat >= BestThreat)
			{
				BestThreat = Threat;
				TargetId = Each.SurvivorId;
			}
		}
	}

	// 표적이 바뀌면 제안을 처음부터: 예약된 아라 사건을 지우고, 새 표적이면 이틀 뒤 밤에 제안
	// (내일 아침 브리핑에서 먼저 파견을 권한 뒤, 그다음 밤에 직접 묻는 순서)
	if (TargetId != Before)
	{
		CancelAraEvents();
		RefusalCount = 0;
		if (HasTarget()) ScheduleAraEvent(OfferEventId, OfferDelay);
	}
}

// ── 교체 ──

bool USSAraDirector::IsAndroid(FName SurvivorId) const
{
	return GetRun().GetCompanions()->IsAndroid(SurvivorId);
}

bool USSAraDirector::CanSwapTarget() const
{
	return !bSwapDone && HasTarget() && IsAliveSurvivor(TargetId);
}

bool USSAraDirector::SwapTarget()
{
	if (!CanSwapTarget()) return false;

	// 겉모습·말투는 그대로인 안드로이드로 바꾸고, 진짜는 B2 격리 구역에 붙잡아 둠
	GetRun().CopySurvivorToCaptured(TargetId);
	GetRun().GetCompanions()->MakeAndroid(TargetId);
	CapturedRealId = TargetId;

	// 아라 편이 됐으니 더는 의심하지 않음 → 브리핑에서 갑자기 사라짐 (플레이어가 눈치챌 단서)
	FindOrAddSuspicion(TargetId).LogOdds = ToLogOdds(AndroidSuspicion);

	bSwapDone = true;
	TargetId = NAME_None;
	CancelAraEvents();
	return true;
}

void USSAraDirector::OnOfferRefused()
{
	if (!CanSwapTarget()) return;

	++RefusalCount;

	// 보호를 거부한 것도 증거: 의심이 오름
	FindOrAddSuspicion(TargetId).LogOdds += FMath::Loge(RefusalRatio);

	// 1번째 거절: 며칠 뒤 더 강하게 다시 제안
	// 2번째부터: 밤에 직접 (강제 교체를 막아내면 이틀 뒤 다시)
	if (RefusalCount == 1)
	{
		ScheduleAraEvent(RepeatOfferEventId, RepeatOfferDelay);
	}
	else
	{
		ScheduleAraEvent(SeizeEventId, RefusalCount == 2 ? SeizeDelay : ResistedSeizeDelay);
	}
}

void USSAraDirector::ScheduleAraEvent(const TCHAR* EventId, int32 DaysLater)
{
	USSRunSubsystem& Run = GetRun();
	Run.GetEventDirector()->ScheduleEvent(FName(EventId), Run.GetCurrentDay() + DaysLater);
}

void USSAraDirector::CancelAraEvents()
{
	USSEventDirector* Events = GetRun().GetEventDirector();
	Events->CancelScheduled(FName(OfferEventId));
	Events->CancelScheduled(FName(RepeatOfferEventId));
	Events->CancelScheduled(FName(SeizeEventId));
}

// ── 낮 ──

bool USSAraDirector::TryEavesdrop(FName SurvivorId, bool bFoundClue)
{
	// 안드로이드의 보고는 아라가 이미 앎
	if (IsAndroid(SurvivorId)) return false;

	// 아라에게 질문할수록(학습도) 대피실 대화를 더 잘 알아들음
	if (Random.FRand() >= EavesdropChance(GetRun().GetAraLearningScore())) return false;

	ObserveEavesdrop(SurvivorId, bFoundClue);
	return true;
}

void USSAraDirector::ObserveEavesdrop(FName SurvivorId, bool bFoundClue)
{
	FSSAraSuspicion& Suspicion = FindOrAddSuspicion(SurvivorId);
	Suspicion.LogOdds += FMath::Loge(bFoundClue ? EavesdropClueRatio : EavesdropPlainRatio);
	++Suspicion.EavesdropsHeard;
}

// ── 읽기 ──

float USSAraDirector::GetSuspicion(FName SurvivorId) const
{
	const FSSAraSuspicion* Suspicion = FindSuspicion(SurvivorId);
	return Suspicion ? ToProbability(Suspicion->LogOdds) : PriorSuspicion;
}

float USSAraDirector::GetThreat(FName SurvivorId) const
{
	return GetSuspicion(SurvivorId) * GetInfluence(SurvivorId);
}

void USSAraDirector::SetLineTable(UDataTable* InLineTable)
{
	LineTable = InLineTable && InLineTable->GetRowStruct() == FSSAraLineRow::StaticStruct() ? InLineTable : nullptr;
	if (InLineTable && !LineTable) UE_LOG(LogTemp, Warning, TEXT("[Ara] Line table row struct must be FSSAraLineRow."));
}

FText USSAraDirector::PickLine(const TCHAR* Kind, int32 Seed, const FFormatNamedArguments& Args, const FText& Fallback) const
{
	// 그 종류의 줄을 모음 (행 이름이 "종류_"로 시작)
	TArray<const FSSAraLineRow*> Rows;
	if (LineTable)
	{
		const FString Prefix = FString(Kind) + TEXT("_");
		TArray<FName> Names = LineTable->GetRowNames();
		Names.Sort([](const FName& A, const FName& B) { return A.LexicalLess(B); });
		for (const FName& RowName : Names)
		{
			if (!RowName.ToString().StartsWith(Prefix)) continue;
			if (const FSSAraLineRow* Row = LineTable->FindRow<FSSAraLineRow>(RowName, TEXT("PickLine"), false)) Rows.Add(Row);
		}
	}

	// Seed로 하나 고름 (같은 날 다시 만들어도 같은 줄)
	const FText Line = Rows.IsEmpty() ? Fallback : Rows[FMath::Abs(Seed) % Rows.Num()]->Line;
	return FText::Format(Line, Args);
}

FText USSAraDirector::BuildBriefing() const
{
	const USSRunSubsystem& Run = GetRun();
	const int32 Day = Run.GetCurrentDay();
	const int32 Food = Run.GetStoredQuantityById(SSItemIds::Food);
	const int32 Water = Run.GetStoredQuantityById(SSItemIds::Water);

	FFormatNamedArguments Args;
	Args.Add(TEXT("Day"), Day);
	Args.Add(TEXT("Food"), Food);
	Args.Add(TEXT("Water"), Water);

	// 대피실 인원 (플레이어 + 살아 있는 동료). 하루 한 개씩 먹는다고 보고 부족한지 판단
	int32 People = 1;
	for (const FSSSurvivorState& Survivor : Run.GetRescuedSurvivors())
	{
		if (Survivor.bAlive) ++People;
	}

	// 첫 줄: 첫날 / 물자 부족 / 평소
	FText Head;
	if (Day == 1)
	{
		Head = PickLine(TEXT("DayOne"), Day, Args, NSLOCTEXT("SSAra", "DayOne",
			"Day 1. B1 비상 대피실의 인원을 확인했습니다. 현재 설비는 정상 범위에서 작동 중입니다. 안전한 하루를 권장합니다."));
	}
	else if (Food < People || Water < People)
	{
		Head = PickLine(TEXT("LowSupply"), Day, Args, NSLOCTEXT("SSAra", "LowSupply",
			"Day {Day}. 식량 {Food}개, 물 {Water}개. 현재 인원 기준으로 부족합니다. 배급 순서를 정하는 것을 권장합니다."));
	}
	else
	{
		Head = PickLine(TEXT("Daily"), Day, Args, NSLOCTEXT("SSAra", "DailyBriefing",
			"Day {Day}. 현재 보유 물자는 식량 {Food}개, 물 {Water}개입니다. 대피실 상태를 계속 관찰하겠습니다."));
	}

	// 의심받는 동료·표적이 있으면 아래에 덧붙임
	const FText Notes = BuildBriefingNotes();
	return Notes.IsEmpty() ? Head : FText::Format(NSLOCTEXT("SSAra", "BriefingWithNotes", "{0}\n{1}"), Head, Notes);
}

FText USSAraDirector::BuildBriefingNotes() const
{
	const int32 Day = GetRun().GetCurrentDay();

	TArray<FText> Notes;
	for (const FSSAraSuspicion& Each : Suspicions)
	{
		if (!IsAliveSurvivor(Each.SurvivorId)) continue;

		const FSSSurvivorState* Survivor = GetRun().FindRescuedSurvivor(Each.SurvivorId);
		const FText Name = IsValid(Survivor->Definition) ? Survivor->Definition->DisplayName : FText::FromName(Each.SurvivorId);
		const float Suspicion = ToProbability(Each.LogOdds);

		FFormatNamedArguments Args;
		Args.Add(TEXT("Name"), Name);
		Args.Add(TEXT("Count"), Each.DetectionsSeen);

		// 같은 날 같은 사람은 같은 줄, 사람마다 다른 줄이 나오게
		const int32 Seed = Day + int32(GetTypeHash(Each.SurvivorId) % 97);

		// 표적: 파견을 권함 (이유는 '효율'로 포장. 아라는 거짓말하지 않고 말하지 않을 뿐)
		if (Each.SurvivorId == TargetId)
		{
			Notes.Add(PickLine(TEXT("Target"), Seed, Args, NSLOCTEXT("SSAra", "TargetNote",
				"권장 사항: {Name} — 외부 물자 탐색 배치. 대피실 내 야간 활동량이 기준치를 넘었습니다. 활동량이 많은 인원은 바깥에서 더 효율적입니다.")));
		}
		// 많이 의심: 이유를 말하지 않음 (엿들은 것도 섞여 있으니까)
		else if (Suspicion >= ConcernSuspicion)
		{
			Notes.Add(PickLine(TEXT("Concern"), Seed, Args, NSLOCTEXT("SSAra", "ConcernNote",
				"{Name}의 행동 패턴이 기준에서 벗어나고 있습니다. 관찰 빈도를 높이겠습니다.")));
		}
		// 조금 의심: 본 것만 말함 (엿들은 건 말하지 않음)
		else if (Suspicion >= NoticeSuspicion && Each.DetectionsSeen > 0)
		{
			Notes.Add(PickLine(TEXT("Notice"), Seed, Args, NSLOCTEXT("SSAra", "NoticeNote",
				"{Name}의 야간 활동이 {Count}회 기록되었습니다. 충분한 수면을 권장합니다.")));
		}
	}
	return FText::Join(FText::FromString(TEXT("\n")), Notes);
}

// ── 디버그 ──

bool USSAraDirector::DebugForceTarget(FName SurvivorId)
{
	if (bSwapDone || !IsAliveSurvivor(SurvivorId) || IsAndroid(SurvivorId)) return false;

	// 밤에 표적 판단을 다시 해도 유지되도록 의심을 높게 둠 (95%)
	FindOrAddSuspicion(SurvivorId).LogOdds = ToLogOdds(0.95f);
	TargetId = SurvivorId;
	RefusalCount = 0;

	// 기다리지 않고 바로 다음 밤에 제안
	CancelAraEvents();
	ScheduleAraEvent(OfferEventId, 1);
	return true;
}

bool USSAraDirector::DebugScheduleSeize()
{
	if (!CanSwapTarget()) return false;
	CancelAraEvents();
	ScheduleAraEvent(SeizeEventId, 1);
	return true;
}

FString USSAraDirector::DebugDescribe() const
{
	FString Out = FString::Printf(TEXT("Target=%s SwapDone=%d Captured=%s Refusals=%d Learning=%d"),
		*TargetId.ToString(), bSwapDone ? 1 : 0, *CapturedRealId.ToString(), RefusalCount, GetRun().GetAraLearningScore());

	for (const FSSSurvivorState& Survivor : GetRun().GetRescuedSurvivors())
	{
		if (!IsValid(Survivor.Definition)) continue;
		const FName Id = Survivor.Definition->SurvivorId;
		const FSSAraSuspicion* Suspicion = FindSuspicion(Id);
		Out += FString::Printf(TEXT("\n  %s: suspicion=%.2f threat=%.2f seen=%d heard=%d %s%s"),
			*Id.ToString(),
			GetSuspicion(Id),
			GetThreat(Id),
			Suspicion ? Suspicion->DetectionsSeen : 0,
			Suspicion ? Suspicion->EavesdropsHeard : 0,
			IsAndroid(Id) ? TEXT("ANDROID") : TEXT("human"),
			Survivor.bAlive ? TEXT("") : TEXT(" (dead)"));
	}
	return Out;
}

void USSAraDirector::ResetRun()
{
	Suspicions.Reset();
	TargetId = NAME_None;
	bSwapDone = false;
	CapturedRealId = NAME_None;
	RefusalCount = 0;
}
