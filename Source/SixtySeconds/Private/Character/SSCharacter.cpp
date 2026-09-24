#include "Character/SSCharacter.h"
#include "Character/SSCharacterStats.h"
#include "Character/SSStatusComponent.h"
#include "Item/SSCarryComponent.h"
#include "Item/SSPickupActor.h"
#include "Character/SSSurvivorPickup.h"
#include "Controller/SSRPlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Engine/World.h"

ASSCharacter::ASSCharacter()
{
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 400.f;
	SpringArm->bUsePawnControlRotation = true;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	CharacterStats = CreateDefaultSubobject<USSCharacterStats>(TEXT("CharacterStats"));
	StatusComponent = CreateDefaultSubobject<USSStatusComponent>(TEXT("StatusComponent"));
	CarryComponent = CreateDefaultSubobject<USSCarryComponent>(TEXT("CarryComponent"));
}

void ASSCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Input Mapping Context 등록
	if (ASSRPlayerController* PC = Cast<ASSRPlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}

		PC->ShowScrambleHUD(CarryComponent);
	}
}

void ASSCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EIC->BindAction(IA_Move, ETriggerEvent::Triggered, this, &ASSCharacter::Move);
		EIC->BindAction(IA_Look, ETriggerEvent::Triggered, this, &ASSCharacter::Look);
		EIC->BindAction(IA_Interact, ETriggerEvent::Started, this, &ASSCharacter::Interact);
	}
}

void ASSCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	if (Controller && Axis != FVector2D::ZeroVector)
	{
		const FRotator YawRotation(0, Controller->GetControlRotation().Yaw, 0);
		AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), Axis.Y);
		AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), Axis.X);
	}
}

void ASSCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(Axis.Y);
}

void ASSCharacter::Interact()
{
	const ASSGameMode* GameMode = GetWorld()->GetAuthGameMode<ASSGameMode>();
	if (!IsValid(GameMode) || GameMode->GetCurrentPhase() != ESSGamePhase::Scramble)
	{
		return;
	}

	// 현재 겹쳐 있는 SSPickupActor 중 가장 가까운 액터 탐색
	TArray<AActor*> Pickups, Survivors;
	GetOverlappingActors(Pickups, ASSPickupActor::StaticClass());
	GetOverlappingActors(Survivors, ASSSurvivorPickup::StaticClass());
	Pickups.Append(Survivors);

	float Closest = MAX_FLT;
	AActor* Target = nullptr;
	for (AActor* Actor : Pickups)
	{
		if (!IsValid(Actor) || Actor->IsHidden()) continue;
		const float Dist = FVector::Dist(GetActorLocation(), Actor->GetActorLocation());
		if (Dist < Closest)
		{
			Closest = Dist;
			Target = Actor;
		}
	}

	if (ASSSurvivorPickup* Survivor = Cast<ASSSurvivorPickup>(Target))
    {
        Survivor->TryRecruit(this);
    }
    else if (ASSPickupActor* Pickup = Cast<ASSPickupActor>(Target))
	{
		Pickup->TryPickup(CarryComponent);
	}
}
