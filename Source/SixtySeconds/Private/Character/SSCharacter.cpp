#include "Character/SSCharacter.h"
#include "Character/SSCharacterStats.h"
#include "Character/SSStatusComponent.h"
#include "Item/SSCarryComponent.h"
#include "Item/SSInteractable.h"
#include "GameMode/SSGameMode.h"
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

AActor* ASSCharacter::FindInteractTarget() const
{
	TArray<AActor*> Overlapping;
	GetOverlappingActors(Overlapping);

	float Closest = MAX_FLT;
	AActor* Target = nullptr;
	for (AActor* Actor : Overlapping)
	{
		// 상호작용 대상만, 숨겨진 것(이미 주운 것·구조된 동료)은 빼고
		if (!IsValid(Actor) || Actor->IsHidden() || !Actor->Implements<USSInteractable>()) continue;

		const float Dist = FVector::DistSquared(GetActorLocation(), Actor->GetActorLocation());
		if (Dist < Closest)
		{
			Closest = Dist;
			Target = Actor;
		}
	}
	return Target;
}

void ASSCharacter::Interact()
{
	const ASSGameMode* GameMode = GetWorld()->GetAuthGameMode<ASSGameMode>();
	if (!IsValid(GameMode) || GameMode->GetCurrentPhase() != ESSGamePhase::Scramble)
	{
		return;
	}

	// 가장 가까운 대상과 상호작용 (아이템이면 줍기, 동료면 데려가기)
	if (ISSInteractable* Target = Cast<ISSInteractable>(FindInteractTarget()))
	{
		Target->TryInteract(this);
	}
}
