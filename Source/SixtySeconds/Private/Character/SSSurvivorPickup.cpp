#include "Character/SSSurvivorPickup.h"
#include "Character/SSSurvivorDefinition.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameMode/SSGameMode.h"
#include "Item/SSRunSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "GameFramework/Pawn.h"
#include "UObject/ConstructorHelpers.h"

ASSSurvivorPickup::ASSSurvivorPickup()
{
    PrimaryActorTick.bCanEverTick = false;
    InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
    RootComponent = InteractionSphere;
    InteractionSphere->InitSphereRadius(160.f);
    InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    InteractionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
    InteractionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
    BodyMesh->SetupAttachment(RootComponent);
    BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Capsule(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (Capsule.Succeeded()) BodyMesh->SetStaticMesh(Capsule.Object);
    BodyMesh->SetRelativeScale3D(FVector(.55f, .55f, 1.4f));
    BodyMesh->SetRelativeLocation(FVector(0, 0, 70));
    Prompt = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Prompt"));
    Prompt->SetupAttachment(RootComponent);
    Prompt->SetRelativeLocation(FVector(0, 0, 185));
    Prompt->SetHorizontalAlignment(EHTA_Center);
    Prompt->SetWorldSize(24);
    Prompt->SetText(FText::FromString(TEXT("SURVIVOR - [E] RESCUE")));
    Prompt->SetTextRenderColor(FColor::Green);
}

bool ASSSurvivorPickup::TryRecruit(APawn* PlayerPawn)
{
    const ASSGameMode* Mode = GetWorld()->GetAuthGameMode<ASSGameMode>();
    if (bRecruited || !IsValid(PlayerPawn) || !PlayerPawn->IsPlayerControlled()
        || !InteractionSphere->IsOverlappingActor(PlayerPawn) || !IsValid(Mode)
        || Mode->GetCurrentPhase() != ESSGamePhase::Scramble
        || Mode->GetScrambleTimeRemaining() <= 0.f) return false;
    UGameInstance* GI = GetGameInstance();
    USSRunSubsystem* Run = IsValid(GI) ? GI->GetSubsystem<USSRunSubsystem>() : nullptr;
    if (!IsValid(Run) || !Run->RecruitSurvivor(Definition)) return false;
    bRecruited = true;
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);
    // Already inside the shelter zone: there may be no new overlap event.
    if (Mode->IsPlayerInShelter()) Run->RescueFollowingSurvivors();
    if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
        TEXT("동료 합류! 은신처 표시존으로 돌아가세요."));
    return true;
}
