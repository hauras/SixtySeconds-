#include "GameMode/ASSGameMode.h"

AASSGameMode::AASSGameMode()
{
}

void AASSGameMode::BeginPlay()
{
	Super::BeginPlay();
	StartScramble();
}

void AASSGameMode::StartScramble()
{
	CurrentPhase = ESSGamePhase::Scramble;
	OnPhaseChanged.Broadcast(CurrentPhase);

	GetWorldTimerManager().SetTimer(
		ScrambleTimerHandle,
		this,
		&AASSGameMode::OnScrambleTimeUp,
		ScrambleDuration,
		false
	);
}

void AASSGameMode::StartShelter()
{
	GetWorldTimerManager().ClearTimer(ScrambleTimerHandle);

	CurrentPhase = ESSGamePhase::Shelter;
	OnPhaseChanged.Broadcast(CurrentPhase);
}

float AASSGameMode::GetScrambleTimeRemaining() const
{
	if (CurrentPhase != ESSGamePhase::Scramble) return 0.f;
	return GetWorldTimerManager().GetTimerRemaining(ScrambleTimerHandle);
}

void AASSGameMode::OnScrambleTimeUp()
{
	StartShelter();
}
