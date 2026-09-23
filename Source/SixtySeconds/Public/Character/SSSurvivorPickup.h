#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SSSurvivorPickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class USSSurvivorDefinition;

UCLASS()
class SIXTYSECONDS_API ASSSurvivorPickup : public AActor
{
    GENERATED_BODY()
public:
    ASSSurvivorPickup();
    bool TryRecruit(APawn* PlayerPawn);

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
