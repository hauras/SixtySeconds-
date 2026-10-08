#include "Rescue/SSRescueState.h"
#include "Ara/SSAraDirector.h"
#include "Character/SSSurvivorDefinition.h"
#include "Companion/SSCompanionState.h"
#include "Ending/SSEndingState.h"
#include "Item/SSRunSubsystem.h"

USSRunSubsystem& USSRescueState::GetRun() const
{
	return *CastChecked<USSRunSubsystem>(GetOuter());
}

ESSRescueBlock USSRescueState::GetBlock() const
{
	const USSRunSubsystem& Run = GetRun();
	const USSCompanionState* Companions = Run.FindCompanions();

	if (IsValid(Active) && !Active->IsFinished()) return ESSRescueBlock::InProgress;
	if (Run.GetCapturedSurvivors().IsEmpty()) return ESSRescueBlock::NobodyCaptured;
	if (!Companions || !Companions->HasHeardClue(SSRescueIds::RouteClue())) return ESSRescueBlock::NoRoute;
	if (LastDay == Run.GetCurrentDay()) return ESSRescueBlock::AlreadyToday;
	if (Run.GetActionPoints() < MinActionPoints) return ESSRescueBlock::NotEnoughActionPoints;
	return ESSRescueBlock::None;
}

USSRescueSession* USSRescueState::Start(FName TargetId, int32 Seed)
{
	USSRunSubsystem& Run = GetRun();
	if (GetBlock() != ESSRescueBlock::None || !Run.IsSurvivorCaptured(TargetId)) return nullptr;

	// 남은 행동력을 전부 씀
	const int32 Points = Run.GetActionPoints();
	if (!Run.SpendActionPoints(Points)) return nullptr;
	SpentActionPoints = Points;

	// 태오가 은신처에 살아 있으면 도움 (안드로이드여도 겉으로는 똑같이 도움, 대신 아라가 바로 앎)
	const FName TechnicianId = SSRescueIds::Technician();
	const FSSSurvivorState* Technician = Run.FindRescuedSurvivor(TechnicianId);
	const bool bTechnicianHelps = Technician && Technician->bAlive;
	const bool bAndroidHelper = bTechnicianHelps && Run.GetCompanions()->IsAndroid(TechnicianId);
	HelperId = bTechnicianHelps ? TechnicianId : NAME_None;

	Active = NewObject<USSRescueSession>(this);
	Active->Start(TargetId,
		USSRescueSession::CalcMoveBudget(Points, bTechnicianHelps),
		bAndroidHelper,
		Seed != 0 ? Seed : FMath::Rand());

	LastDay = Run.GetCurrentDay();

	// 패널을 처음 열면 전력 예약 기록을 봄 (숨은 진실 1)
	Run.GetEnding()->AddHiddenTruth(SSRescueIds::PanelLogTruth(), NSLOCTEXT("SSRescue", "PanelLogJournal",
		"B2 정비 패널 기록: 정화 프로토콜 전력 예약 · 대상 구역: 제7연구소 전 층 · 상태: 대기. 정화가 무엇을 태우는지는 적혀 있지 않다."));
	return Active;
}

bool USSRescueState::Finish(FSSRescueReport& OutReport)
{
	if (!IsValid(Active) || !Active->IsFinished()) return false;
	USSRunSubsystem& Run = GetRun();

	OutReport = FSSRescueReport{};
	OutReport.Outcome = Active->GetOutcome();
	OutReport.TargetId = Active->GetTargetId();
	OutReport.AlarmCount = Active->GetAlarmCount();
	OutReport.MovesUsed = Active->GetMoveBudget() - Active->GetRemainingMoves();
	OutReport.ActionPointsSpent = SpentActionPoints;
	AlarmTotal += OutReport.AlarmCount;

	// 이름은 귀환 전 붙잡힌 목록에서 (귀환하면 목록에서 빠지므로 먼저)
	FText Name = FText::FromName(OutReport.TargetId);
	for (const FSSSurvivorState& Captured : Run.GetCapturedSurvivors())
	{
		if (IsValid(Captured.Definition) && Captured.Definition->SurvivorId == OutReport.TargetId) Name = Captured.Definition->DisplayName;
	}

	if (OutReport.Outcome == ESSRescueOutcome::Unlocked)
	{
		Run.ReleaseCapturedSurvivor(OutReport.TargetId, OutReport.bReplacedAndroid);
		++SuccessCount;
	}

	// 경보: 도운 사람이 있으면 그 사람이 강하게 의심받고,
	// 혼자였으면 아라는 "B1의 누군가"까지만 알아서 은신처의 살아 있는 동료 전원이 조금씩 의심받음
	if (!HelperId.IsNone())
	{
		Run.GetAra()->ObserveAlarm(HelperId, OutReport.AlarmCount);
	}
	else
	{
		for (const FSSSurvivorState& Survivor : Run.GetRescuedSurvivors())
		{
			if (!Survivor.bAlive || !IsValid(Survivor.Definition)) continue;
			Run.GetAra()->ObserveAlarm(Survivor.Definition->SurvivorId, OutReport.AlarmCount, false);
		}
	}

	// 기록 (건조한 기록체)
	FText Message;
	if (OutReport.Outcome == ESSRescueOutcome::Unlocked && OutReport.bReplacedAndroid)
	{
		Message = FText::Format(NSLOCTEXT("SSRescue", "UnlockedReplaced",
			"B2 정비 패널: 격리실 잠금 해제. {0} 귀환. 은신처에 있던 같은 얼굴의 {0}: 작동 정지."), Name);
	}
	else if (OutReport.Outcome == ESSRescueOutcome::Unlocked)
	{
		Message = FText::Format(NSLOCTEXT("SSRescue", "Unlocked", "B2 정비 패널: 격리실 잠금 해제. {0} 귀환."), Name);
	}
	else if (OutReport.Outcome == ESSRescueOutcome::OutOfMoves)
	{
		Message = NSLOCTEXT("SSRescue", "OutOfMoves", "B2 정비 패널: 배선을 다 잇지 못하고 철수.");
	}
	else
	{
		Message = NSLOCTEXT("SSRescue", "Aborted", "B2 정비 패널: 작업 중단, 철수.");
	}
	if (OutReport.AlarmCount > 0)
	{
		Message = FText::Format(NSLOCTEXT("SSRescue", "WithAlarm", "{0} 경보 전송 {1}회."), Message, OutReport.AlarmCount);
	}
	Run.AddJournal(ESSJournalEvent::Event, Message);

	Active = nullptr;
	HelperId = NAME_None;
	SpentActionPoints = 0;
	return true;
}

void USSRescueState::ResetRun()
{
	Active = nullptr;
	HelperId = NAME_None;
	LastDay = 0;
	SpentActionPoints = 0;
	SuccessCount = 0;
	AlarmTotal = 0;
}
