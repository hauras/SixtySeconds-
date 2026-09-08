#include "Phase/SSPhaseManager.h"
#include "Kismet/GameplayStatics.h"

USSPhaseManager::USSPhaseManager()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USSPhaseManager::BeginPlay()
{
	Super::BeginPlay();

	// GameMode의 페이즈 전환 델리게이트 구독
	if (AASSGameMode* GM = Cast<AASSGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GM->OnPhaseChanged.AddDynamic(this, &USSPhaseManager::OnPhaseChanged);
	}
}

void USSPhaseManager::OnScrambleStarted()
{
	CurrentPhase = ESSGamePhase::Scramble;
}

void USSPhaseManager::OnShelterStarted()
{
	CurrentPhase = ESSGamePhase::Shelter;
	CurrentDay = 1;
	CurrentTurn = 1;
}

void USSPhaseManager::AdvanceShelterTurn()
{
	if (CurrentPhase != ESSGamePhase::Shelter) return;

	CurrentTurn++;
	if (CurrentTurn > TurnsPerDay)
	{
		CurrentTurn = 1;
		CurrentDay++;
	}
}

void USSPhaseManager::OnPhaseChanged(ESSGamePhase NewPhase)
{
	if (NewPhase == ESSGamePhase::Scramble)
		OnScrambleStarted();
	else
		OnShelterStarted();
}
