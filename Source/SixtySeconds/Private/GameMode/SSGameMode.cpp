#include "GameMode/SSGameMode.h"
#include "Kismet/GameplayStatics.h"

ASSGameMode::ASSGameMode()
{
}

void ASSGameMode::BeginPlay()
{
	Super::BeginPlay();
	CacheDepositZone();
	StartScramble();
}

void ASSGameMode::CacheDepositZone()
{
	DepositZone = Cast<ASSDepositZone>(
		UGameplayStatics::GetActorOfClass(this, ASSDepositZone::StaticClass()));
	if (!IsValid(DepositZone))
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameMode] DepositZone not found in level"));
	}
}

void ASSGameMode::StartScramble()
{
	CurrentPhase = ESSGamePhase::Scramble;
	OnPhaseChanged.Broadcast(CurrentPhase);

	GetWorldTimerManager().SetTimer(
		ScrambleTimerHandle,
		this,
		&ASSGameMode::OnScrambleTimeUp,
		ScrambleDuration,
		false
	);
}

void ASSGameMode::StartShelter()
{
	GetWorldTimerManager().ClearTimer(ScrambleTimerHandle);

	CurrentPhase = ESSGamePhase::Shelter;
	OnPhaseChanged.Broadcast(CurrentPhase);
}

float ASSGameMode::GetScrambleTimeRemaining() const
{
	if (CurrentPhase != ESSGamePhase::Scramble) return 0.f;
	return GetWorldTimerManager().GetTimerRemaining(ScrambleTimerHandle);
}

void ASSGameMode::OnScrambleTimeUp()
{
	const bool bInside = IsValid(DepositZone) && DepositZone->IsPlayerInside();
	UE_LOG(LogTemp, Warning, TEXT("[GameMode] 타이머 종료 — 존 안: %s"), bInside ? TEXT("생존") : TEXT("사망"));

	if (bInside)
	{
		StartShelter();
	}
	else
	{
		StartDeath();
	}
}

void ASSGameMode::StartDeath()
{
	GetWorldTimerManager().ClearTimer(ScrambleTimerHandle);
	UE_LOG(LogTemp, Warning, TEXT("[GameMode] 사망 처리"));
	OnPlayerDied.Broadcast(ESSGamePhase::Scramble);
}
