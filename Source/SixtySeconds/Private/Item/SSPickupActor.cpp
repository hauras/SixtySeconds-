#include "Item/SSPickupActor.h"
#include "Item/SSCarryComponent.h"
#include "Item/SSItemDefinition.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Pawn.h"

ASSPickupActor::ASSPickupActor()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = MeshComponent;

	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetupAttachment(RootComponent);
	InteractionSphere->SetSphereRadius(120.f);
	InteractionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}

void ASSPickupActor::BeginPlay()
{
	Super::BeginPlay();

	// 아이템 정의에 메시가 있으면 자동으로 적용
	if (IsValid(ItemStack.Item) && IsValid(ItemStack.Item->WorldMesh))
	{
		MeshComponent->SetStaticMesh(ItemStack.Item->WorldMesh);
	}
}

bool ASSPickupActor::CanPickupWith(const USSCarryComponent* CarryComponent) const
{
	return IsValid(CarryComponent) && IsValid(ItemStack.Item) && ItemStack.Quantity > 0 && CarryComponent->CanAddItem(ItemStack.Item.Get(), ItemStack.Quantity);
}

FText ASSPickupActor::GetPickupPrompt(const USSCarryComponent* CarryComponent) const
{
	const FText Name = IsValid(ItemStack.Item) ? ItemStack.Item->DisplayName : FText::GetEmpty();

	// 두 개 이상이면 개수도 ("물 ×2")
	const FText Label = ItemStack.Quantity > 1
		? FText::Format(NSLOCTEXT("SSPickup", "NameWithCount", "{0} ×{1}"), Name, ItemStack.Quantity)
		: Name;

	return CanPickupWith(CarryComponent)
		? FText::Format(NSLOCTEXT("SSPickup", "Prompt", "{0} · [E] 줍기"), Label)
		: FText::Format(NSLOCTEXT("SSPickup", "PromptFull", "{0} · 가방이 가득 찼다"), Label);
}

bool ASSPickupActor::CanInteract(const APawn* Interactor) const
{
	return IsValid(Interactor) && CanPickupWith(Interactor->FindComponentByClass<USSCarryComponent>());
}

FText ASSPickupActor::GetInteractPrompt(const APawn* Interactor) const
{
	return GetPickupPrompt(IsValid(Interactor) ? Interactor->FindComponentByClass<USSCarryComponent>() : nullptr);
}

bool ASSPickupActor::TryInteract(APawn* Interactor)
{
	return IsValid(Interactor) && TryPickup(Interactor->FindComponentByClass<USSCarryComponent>());
}

bool ASSPickupActor::TryPickup(USSCarryComponent* CarryComponent)
{
	if (!IsValid(CarryComponent)) return false;
	if (!IsValid(ItemStack.Item) || ItemStack.Quantity <= 0) return false;

	if (!CarryComponent->TryAddItem(ItemStack.Item, ItemStack.Quantity)) return false;

	Destroy();
	return true;
}
