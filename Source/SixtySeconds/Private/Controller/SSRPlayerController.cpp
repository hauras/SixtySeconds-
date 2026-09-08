

#include "Controller/SSRPlayerController.h"
#include "UI/SSScrambleHUD.h"
#include "Item/SSCarryComponent.h"

void ASSRPlayerController::BeginPlay()
{
	Super::BeginPlay();
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

