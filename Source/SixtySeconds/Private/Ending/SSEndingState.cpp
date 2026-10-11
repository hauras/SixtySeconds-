#include "Ending/SSEndingState.h"
#include "Character/SSSurvivorDefinition.h"
#include "Companion/SSCompanionState.h"
#include "Event/SSEventDirector.h"
#include "Item/SSRunSubsystem.h"
#include "Rescue/SSRescueState.h"

USSRunSubsystem& USSEndingState::GetRun() const
{
	return *CastChecked<USSRunSubsystem>(GetOuter());
}

bool USSEndingState::AddHiddenTruth(FName TruthId, const FText& JournalLine)
{
	if (TruthId.IsNone() || HiddenTruths.Contains(TruthId)) return false;

	HiddenTruths.Add(TruthId);
	USSRunSubsystem& Run = GetRun();
	if (!JournalLine.IsEmpty()) Run.AddJournal(ESSJournalEvent::Event, JournalLine);

	// 진실을 다 모은 날: 그날 밤 아라와 마주함 → 다음 날 밤 서버실 (12일까지 버티기만 하는 구간을 없앰)
	if (CountHiddenTruths() == TruthsForConfront && !HasEnded())
	{
		Run.GetEventDirector()->ScheduleEvent(GetConfrontEventId(), Run.GetCurrentDay());
	}
	return true;
}

bool USSEndingState::HasSeenPanelLog() const
{
	return HasHiddenTruth(SSRescueIds::PanelLogTruth());
}

void USSEndingState::ReachEnding(ESSEnding Requested)
{
	if (HasEnded() || Requested == ESSEnding::None) return;
	USSRunSubsystem& Run = GetRun();

	FSSEndingReport NewReport;
	NewReport.Ending = Requested;
	NewReport.bKnewTruth = CountHiddenTruths() > 0;
	NewReport.DaysSurvived = Run.GetCurrentDay();
	NewReport.RescuedCount = Run.GetRescue()->GetSuccessCount();
	NewReport.AlarmCount = Run.GetRescue()->GetAlarmTotal();
	CountSurvivors(NewReport.LeavingCount, NewReport.PartySize);

	// 종료하러 데려간 하린이 사실 아라의 안드로이드면 종료는 실패 → 지배
	if (Requested == ESSEnding::Resolve && Run.GetCompanions()->IsAndroid(SSRescueIds::Researcher()))
	{
		NewReport.Ending = ESSEnding::Dominion;
		NewReport.bSabotaged = true;
	}

	Report = NewReport;
}

void USSEndingState::CountSurvivors(int32& OutLeaving, int32& OutPartySize) const
{
	USSRunSubsystem& Run = GetRun();

	// 플레이어 (엔딩까지 왔으면 살아 있음)
	OutLeaving = 1;
	OutPartySize = 1;

	// 대피실에 있었던 사람: 은신처 명단 + B2에 붙잡힌 사람. 바꿔치기면 같은 사람이 양쪽에 있으므로 한 번만
	TSet<FName> Members;
	for (const FSSSurvivorState& Survivor : Run.GetRescuedSurvivors())
	{
		if (IsValid(Survivor.Definition)) Members.Add(Survivor.Definition->SurvivorId);
	}
	for (const FSSSurvivorState& Survivor : Run.GetCapturedSurvivors())
	{
		if (IsValid(Survivor.Definition)) Members.Add(Survivor.Definition->SurvivorId);
	}
	OutPartySize += Members.Num();

	// 함께 나가는 사람: 은신처에 살아 있는 진짜 사람 (안드로이드와 B2에 남은 사람은 빠짐)
	for (const FSSSurvivorState& Survivor : Run.GetRescuedSurvivors())
	{
		if (!Survivor.bAlive || !IsValid(Survivor.Definition)) continue;
		if (Run.GetCompanions()->IsAndroid(Survivor.Definition->SurvivorId)) continue;
		++OutLeaving;
	}
}

void USSEndingState::ResetRun()
{
	HiddenTruths.Reset();
	Report = FSSEndingReport{};
}
