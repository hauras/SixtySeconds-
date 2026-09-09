// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameMode/SSGameMode.h"
#include "SSRPlayerController.generated.h"

class USSScrambleHUD;
class USSShelterHUD;
class USSCarryComponent;

UCLASS()
class SIXTYSECONDS_API ASSRPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	void ShowScrambleHUD(USSCarryComponent* CarryComponent);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category="SS|UI")
	TSubclassOf<USSScrambleHUD> ScrambleHUDClass;

	UPROPERTY(EditDefaultsOnly, Category="SS|UI")
	TSubclassOf<USSShelterHUD> ShelterHUDClass;

private:
	UFUNCTION()
	void HandlePhaseChanged(ESSGamePhase NewPhase);

	void CreateShelterHUD();

	bool bScrambleInputBlocked = false;

	UPROPERTY()
	TObjectPtr<USSScrambleHUD> ScrambleHUD;

	UPROPERTY(Transient)
	TObjectPtr<USSShelterHUD> ShelterHUD;
};
