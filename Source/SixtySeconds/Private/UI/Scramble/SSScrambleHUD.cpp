#include "UI/Scramble/SSScrambleHUD.h"
#include "UI/Scramble/SSCarryInventoryWidget.h"
#include "UI/Scramble/SSScrambleTimerWidget.h"
#include "Item/SSCarryComponent.h"
#include "GameMode/SSGameMode.h"
#include "Kismet/GameplayStatics.h"

void USSScrambleHUD::InitHUD(USSCarryComponent* InCarry)
{
	CarryComponent = InCarry;
	InitializeChildren();
}

void USSScrambleHUD::NativeConstruct()
{
	Super::NativeConstruct();
	InitializeChildren();
}

void USSScrambleHUD::InitializeChildren()
{
	if (InventoryWidget) InventoryWidget->InitializeInventory(CarryComponent);
	if (TimerWidget) TimerWidget->InitializeTimer(Cast<ASSGameMode>(UGameplayStatics::GetGameMode(this)));
}
