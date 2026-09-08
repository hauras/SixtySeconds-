#include "Item/SSPickupActor.h"
#include "Item/SSCarryComponent.h"
#include "Item/SSItemDefinition.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"

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

bool ASSPickupActor::TryPickup(USSCarryComponent* CarryComponent)
{
	if (!IsValid(CarryComponent)) return false;
	if (!IsValid(ItemStack.Item) || ItemStack.Quantity <= 0) return false;

	if (!CarryComponent->TryAddItem(ItemStack.Item, ItemStack.Quantity)) return false;

	Destroy();
	return true;
}
