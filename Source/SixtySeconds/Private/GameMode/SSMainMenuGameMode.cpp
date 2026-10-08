#include "GameMode/SSMainMenuGameMode.h"
#include "UI/Menu/SSMainMenuWidget.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

ASSMainMenuGameMode::ASSMainMenuGameMode()
{
	// 메인 화면에는 움직일 캐릭터가 없음
	DefaultPawnClass = nullptr;
}

void ASSMainMenuGameMode::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* Controller = UGameplayStatics::GetPlayerController(this, 0);
	if (!Controller) return;

	if (!MenuWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MainMenu] Assign WBP_MainMenu in the main menu game mode defaults."));
		return;
	}

	MenuWidget = CreateWidget<USSMainMenuWidget>(Controller, MenuWidgetClass);
	if (!MenuWidget) return;
	MenuWidget->AddToViewport();

	// 마우스로만 고름
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(MenuWidget->TakeWidget());
	Controller->SetInputMode(InputMode);
	Controller->SetShowMouseCursor(true);
}
