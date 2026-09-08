// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SSRPlayerController.generated.h"

class USSScrambleHUD;
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

private:
	UPROPERTY()
	TObjectPtr<USSScrambleHUD> ScrambleHUD;
};
