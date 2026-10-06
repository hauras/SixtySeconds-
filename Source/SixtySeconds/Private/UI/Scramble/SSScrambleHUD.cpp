#include "UI/Scramble/SSScrambleHUD.h"
#include "UI/Scramble/SSCarryInventoryWidget.h"
#include "UI/Scramble/SSScrambleTimerWidget.h"
#include "Item/SSCarryComponent.h"
#include "GameMode/SSGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "UI/Scramble/SSInteractPromptWidget.h"
#include "Character/SSCharacter.h"
#include "Item/SSInteractable.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/MeshComponent.h"

void USSScrambleHUD::InitHUD(USSCarryComponent* InCarry)
{
	CarryComponent = InCarry;
	InitializeChildren();
}

void USSScrambleHUD::NativeConstruct()
{
	Super::NativeConstruct();
	InitializeChildren();

	// 안내 위젯은 HUD와 따로 화면에 올림 (HUD 배치와 상관없이 화면 어디든 뜨게)
	if (!IsValid(PromptWidget))
	{
		PromptWidget = CreateWidget<USSInteractPromptWidget>(GetOwningPlayer(), USSInteractPromptWidget::StaticClass());
	}
	if (IsValid(PromptWidget) && !PromptWidget->IsInViewport())
	{
		PromptWidget->AddToViewport(5);
	}
}

void USSScrambleHUD::NativeDestruct()
{
	// 스크램블 HUD가 사라지면(격벽 연출·은신처) 안내도 같이 치움
	if (IsValid(PromptWidget)) PromptWidget->RemoveFromParent();
	Super::NativeDestruct();
}

void USSScrambleHUD::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!IsValid(PromptWidget)) return;

	// 스크램블 중일 때만
	const ASSGameMode* Mode = Cast<ASSGameMode>(UGameplayStatics::GetGameMode(this));
	const ASSCharacter* Character = Cast<ASSCharacter>(GetOwningPlayerPawn());
	AActor* Target = IsValid(Mode) && Mode->GetCurrentPhase() == ESSGamePhase::Scramble && IsValid(Character)
		? Character->FindInteractTarget()
		: nullptr;
	const ISSInteractable* Interactable = Cast<ISSInteractable>(Target);
	if (!Interactable)
	{
		PromptWidget->HidePrompt();
		return;
	}

	// 대상의 맨 위보다 조금 높은 곳을 화면 좌표로 (화면 배율까지 반영된 위젯 좌표)
	// 보이는 메시만 기준 (감지용 구체는 반지름이 커서 빼야 함)
	FBox MeshBox(ForceInit);
	TArray<UMeshComponent*> Meshes;
	Target->GetComponents(Meshes);
	for (const UMeshComponent* Mesh : Meshes)
	{
		if (Mesh->IsVisible()) MeshBox += Mesh->Bounds.GetBox();
	}
	const FVector Top = MeshBox.IsValid
		? FVector(MeshBox.GetCenter().X, MeshBox.GetCenter().Y, MeshBox.Max.Z + PromptHeightAboveTarget)
		: Target->GetActorLocation() + FVector(0.f, 0.f, PromptHeightAboveTarget);
	FVector2D ScreenPosition;
	if (!UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(GetOwningPlayer(), Top, ScreenPosition, false))
	{
		// 화면 뒤쪽이면 숨김
		PromptWidget->HidePrompt();
		return;
	}

	PromptWidget->ShowAt(ScreenPosition, Interactable->GetInteractPrompt(Character), Interactable->CanInteract(Character));
}

void USSScrambleHUD::InitializeChildren()
{
	if (InventoryWidget) InventoryWidget->InitializeInventory(CarryComponent);
	if (TimerWidget) TimerWidget->InitializeTimer(Cast<ASSGameMode>(UGameplayStatics::GetGameMode(this)));
}
