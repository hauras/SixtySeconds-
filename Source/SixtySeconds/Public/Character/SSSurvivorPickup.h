#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Item/SSInteractable.h"
#include "SSSurvivorPickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class USSSurvivorDefinition;

UCLASS()
class SIXTYSECONDS_API ASSSurvivorPickup : public AActor, public ISSInteractable
{
	GENERATED_BODY()
public:
	ASSSurvivorPickup();
	bool TryRecruit(APawn* PlayerPawn);

	// ── ISSInteractable ── ("서하린 · [E] 데려가기")
	virtual bool CanInteract(const APawn* Interactor) const override;
	virtual FText GetInteractPrompt(const APawn* Interactor) const override;
	virtual bool TryInteract(APawn* Interactor) override { return TryRecruit(Interactor); }

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Survivor")
	TObjectPtr<USSSurvivorDefinition> Definition;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SS|Survivor")
	TObjectPtr<USphereComponent> InteractionSphere;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SS|Survivor")
	TObjectPtr<UStaticMeshComponent> BodyMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SS|Survivor")
	TObjectPtr<UTextRenderComponent> Prompt;

private:
	bool bRecruited = false;
};
