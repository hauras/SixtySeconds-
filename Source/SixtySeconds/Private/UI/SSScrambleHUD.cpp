#include "UI/SSScrambleHUD.h"
#include "UI/SSCarryInventoryWidget.h"
#include "UI/SSScrambleTimerWidget.h"
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
