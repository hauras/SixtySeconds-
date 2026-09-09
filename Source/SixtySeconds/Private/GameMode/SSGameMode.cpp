#include "GameMode/SSGameMode.h"
#include "Character/SSCharacterStats.h"
#include "Kismet/GameplayStatics.h"
#include "Item/SSRunSubsystem.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Pawn.h"

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
	if (CurrentPhase != ESSGamePhase::Scramble)
	{
		return;
	}

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	UGameInstance* RunGameInstance = GetGameInstance();

	if (!IsValid(PlayerPawn) || !IsValid(RunGameInstance))
	{
		UE_LOG(LogTemp, Error, TEXT("[Shelter] Player or GameInstance is missing."));
		return;
	}

	USSCharacterStats* PlayerStats = PlayerPawn->FindComponentByClass<USSCharacterStats>();

	USSRunSubsystem* RunSubsystem  = RunGameInstance->GetSubsystem<USSRunSubsystem>();

	if (!IsValid(PlayerStats) || !IsValid(RunSubsystem ))
	{
		UE_LOG(LogTemp, Error, TEXT("[Shelter] Stats or RunSubsystem is missing."));
		return;
	}

	RunSubsystem->InitializeShelterStats(
		PlayerStats->GetStat(ESSStatType::HP),
		PlayerStats->GetStat(ESSStatType::Hunger),
		PlayerStats->GetStat(ESSStatType::Thirst));

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
	CurrentPhase = ESSGamePhase::Dead;
	OnPhaseChanged.Broadcast(CurrentPhase);
	UE_LOG(LogTemp, Warning, TEXT("[GameMode] 사망 처리"));
	OnPlayerDied.Broadcast(CurrentPhase);
}
