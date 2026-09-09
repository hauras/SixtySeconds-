

#include "Controller/SSRPlayerController.h"
#include "UI/SSScrambleHUD.h"
#include "UI/SSShelterHUD.h"
#include "Item/SSCarryComponent.h"
#include "Character/SSCharacterStats.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "Item/SSRunSubsystem.h"
#include "Engine/GameInstance.h"

void ASSRPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (ASSGameMode* GameMode = GetWorld()->GetAuthGameMode<ASSGameMode>())
	{
		GameMode->OnPhaseChanged.AddUniqueDynamic(this, &ASSRPlayerController::HandlePhaseChanged);
		HandlePhaseChanged(GameMode->GetCurrentPhase());
	}
}

void ASSRPlayerController::HandlePhaseChanged(ESSGamePhase NewPhase)
{
	if (NewPhase == ESSGamePhase::Shelter)
	{
		CreateShelterHUD();
	}

	const bool bShouldBlock = NewPhase != ESSGamePhase::Scramble;
	if (bScrambleInputBlocked == bShouldBlock) return;

	bScrambleInputBlocked = bShouldBlock;
	SetIgnoreMoveInput(bShouldBlock);
	SetIgnoreLookInput(bShouldBlock);
	if (bShouldBlock)
	{
		if (ACharacter* ControlledCharacter = Cast<ACharacter>(GetPawn()))
		{
			ControlledCharacter->ConsumeMovementInputVector();
			ControlledCharacter->GetCharacterMovement()->StopMovementImmediately();
		}
	}
}

void ASSRPlayerController::CreateShelterHUD()
{
	if (!IsLocalController()) return;

	if (!ShelterHUDClass)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[UI] ShelterHUDClass is not assigned."));
		return;
	}

	UGameInstance* RunGameInstance = GetGameInstance();
	USSRunSubsystem* RunSubsystem = IsValid(RunGameInstance)
		? RunGameInstance->GetSubsystem<USSRunSubsystem>()
		: nullptr;

	if (!IsValid(RunSubsystem))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[UI] RunSubsystem is missing."));
		return;
	}

	if (!IsValid(ShelterHUD))
	{
		ShelterHUD = CreateWidget<USSShelterHUD>(
			this, ShelterHUDClass);
	}

	if (!IsValid(ShelterHUD)) return;

	ShelterHUD->InitHUD(RunSubsystem->GetCurrentDay());
	ShelterHUD->RefreshStats(
		RunSubsystem->GetHealth(),
		RunSubsystem->GetSatiety(),
		RunSubsystem->GetHydration());

	if (!ShelterHUD->IsInViewport())
	{
		ShelterHUD->AddToViewport();
	}

	if (IsValid(ScrambleHUD))
	{
		ScrambleHUD->RemoveFromParent();
		ScrambleHUD = nullptr;
	}

	SetShowMouseCursor(true);

	FInputModeUIOnly InputMode;
	SetInputMode(InputMode);
}

void ASSRPlayerController::ShowScrambleHUD(USSCarryComponent* CarryComponent)
{
	if (!ScrambleHUDClass) return;

	if (ScrambleHUD)
	{
		ScrambleHUD->RemoveFromParent();
	}

	ScrambleHUD = CreateWidget<USSScrambleHUD>(this, ScrambleHUDClass);
	if (ScrambleHUD)
	{
		ScrambleHUD->InitHUD(CarryComponent);
		ScrambleHUD->AddToViewport();
	}
}
