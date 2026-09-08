#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SSCharacter.generated.h"

class USSCharacterStats;
class USSStatusComponent;
class USSCarryComponent;
class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

UCLASS()
class SIXTYSECONDS_API ASSCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ASSCharacter();

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintPure, Category="SS|Character")
	USSCharacterStats* GetCharacterStats() const { return CharacterStats; }

	UFUNCTION(BlueprintPure, Category="SS|Character")
	USSStatusComponent* GetStatusComponent() const { return StatusComponent; }

protected:
	virtual void BeginPlay() override;

	// 3인칭 카메라
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SS|Camera")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SS|Camera")
	TObjectPtr<UCameraComponent> Camera;

	// Enhanced Input 에셋 (Blueprint에서 할당)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SS|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SS|Input")
	TObjectPtr<UInputAction> IA_Move;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SS|Input")
	TObjectPtr<UInputAction> IA_Look;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SS|Input")
	TObjectPtr<UInputAction> IA_Interact;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SS|Character")
	TObjectPtr<USSCharacterStats> CharacterStats;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SS|Character")
	TObjectPtr<USSStatusComponent> StatusComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SS|Character")
	TObjectPtr<USSCarryComponent> CarryComponent;

	UFUNCTION(BlueprintPure, Category="SS|Character")
	USSCarryComponent* GetCarryComponent() const { return CarryComponent; }

private:
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void Interact();
};
