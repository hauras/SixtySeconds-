#include "Item/SSDepositZone.h"
#include "Item/SSCarryComponent.h"
#include "Item/SSRunSubsystem.h"
#include "Item/SSItemDefinition.h"
#include "Components/BoxComponent.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "GameMode/SSGameMode.h"
#include "Engine/Engine.h"

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
    const ASSGameMode* Mode = GetWorld()->GetAuthGameMode<ASSGameMode>();
    if (!IsValid(Mode) || Mode->GetCurrentPhase() != ESSGamePhase::Scramble
        || Mode->GetScrambleTimeRemaining() <= 0.f) return;

	USSRunSubsystem* RunSub =
		UGameplayStatics::GetGameInstance(this)->GetSubsystem<USSRunSubsystem>();
	if (!IsValid(RunSub)) return;

    const int32 RescuedCount = RunSub->RescueFollowingSurvivors();
    if (RescuedCount > 0 && GEngine)
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
            FString::Printf(TEXT("동료 %d명 구조 완료!"), RescuedCount));

    USSCarryComponent* Carry = PlayerPawn->FindComponentByClass<USSCarryComponent>();
    if (!Carry || Carry->GetItems().IsEmpty()) return;

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
