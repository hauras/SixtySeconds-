#include "Ending/SSEndingState.h"
#include "Companion/SSCompanionState.h"
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
	if (!JournalLine.IsEmpty()) GetRun().AddJournal(ESSJournalEvent::Event, JournalLine);
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

	// 종료하러 데려간 하린이 사실 아라의 안드로이드면 종료는 실패 → 지배
	if (Requested == ESSEnding::Resolve && Run.GetCompanions()->IsAndroid(SSRescueIds::Researcher()))
	{
		NewReport.Ending = ESSEnding::Dominion;
		NewReport.bSabotaged = true;
	}

	Report = NewReport;
}

void USSEndingState::ResetRun()
{
	HiddenTruths.Reset();
	Report = FSSEndingReport{};
}
