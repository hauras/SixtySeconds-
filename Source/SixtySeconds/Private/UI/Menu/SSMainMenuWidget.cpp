#include "UI/Menu/SSMainMenuWidget.h"
#include "Item/SSRunSubsystem.h"
#include "Components/Button.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

void USSMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (StartButton) StartButton->OnClicked.AddUniqueDynamic(this, &ThisClass::OnStartClicked);
	if (QuitButton) QuitButton->OnClicked.AddUniqueDynamic(this, &ThisClass::OnQuitClicked);
}

void USSMainMenuWidget::NativeDestruct()
{
	if (StartButton) StartButton->OnClicked.RemoveDynamic(this, &ThisClass::OnStartClicked);
	if (QuitButton) QuitButton->OnClicked.RemoveDynamic(this, &ThisClass::OnQuitClicked);
	Super::NativeDestruct();
}

void USSMainMenuWidget::OnStartClicked()
{
	if (bStarting) return;
	bStarting = true;

	// 새 판: 지난 판의 날짜·물자·동료·엔딩을 모두 지움
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (USSRunSubsystem* Run = GameInstance->GetSubsystem<USSRunSubsystem>()) Run->ResetRun();
	}

	// 스크램블은 캐릭터 조작이라 입력을 게임으로 돌려줌
	if (APlayerController* Controller = GetOwningPlayer())
	{
		Controller->SetInputMode(FInputModeGameOnly());
		Controller->SetShowMouseCursor(false);
	}
	UGameplayStatics::OpenLevel(this, GameLevel);
}

void USSMainMenuWidget::OnQuitClicked()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}
