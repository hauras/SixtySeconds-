#include "Item/SSDepositZone.h"
#include "Item/SSCarryComponent.h"
#include "Item/SSRunSubsystem.h"
#include "Item/SSItemDefinition.h"
#include "Components/BoxComponent.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"

ASSDepositZone::ASSDepositZone()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	RootComponent = TriggerBox;
	TriggerBox->SetBoxExtent(FVector(200.f, 200.f, 200.f));
	TriggerBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}

void ASSDepositZone::BeginPlay()
{
	Super::BeginPlay();
	OnActorBeginOverlap.AddDynamic(this, &ASSDepositZone::OnActorEntered);
	OnActorEndOverlap.AddDynamic(this, &ASSDepositZone::OnActorExited);
}

void ASSDepositZone::OnActorEntered(AActor* OverlappedActor, AActor* OtherActor)
{
	APawn* PlayerPawn = Cast<APawn>(OtherActor);
	if (!IsValid(PlayerPawn) || !PlayerPawn->IsPlayerControlled())
	{
		return;
	}

	bIsPlayerInside = true;

	USSCarryComponent* Carry = PlayerPawn->FindComponentByClass<USSCarryComponent>();
	if (!Carry || Carry->GetItems().IsEmpty()) return;

	USSRunSubsystem* RunSub =
		UGameplayStatics::GetGameInstance(this)->GetSubsystem<USSRunSubsystem>();
	if (!IsValid(RunSub)) return;

	RunSub->DepositItems(Carry->GetItems());
	Carry->ClearItems();

	OnDeposited.Broadcast();
}

void ASSDepositZone::OnActorExited(AActor* OverlappedActor, AActor* OtherActor)
{
	APawn* PlayerPawn = Cast<APawn>(OtherActor);
	if (!IsValid(PlayerPawn) || !PlayerPawn->IsPlayerControlled())
	{
		return;
	}

	bIsPlayerInside = false;
}
