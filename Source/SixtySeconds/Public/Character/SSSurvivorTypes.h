
#pragma once

#include "CoreMinimal.h"
#include "SSSurvivorTypes.generated.h"

class USSSurvivorDefinition;

USTRUCT(BlueprintType)
struct SIXTYSECONDS_API FSSSurvivorStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SS|Stats")
	float Health = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SS|Stats")
	float Satiety = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SS|Stats")
	float Hydration = 100.f;
};

USTRUCT(BlueprintType)
struct SIXTYSECONDS_API FSSSurvivorState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) TObjectPtr<USSSurvivorDefinition> Definition = nullptr;
    UPROPERTY(BlueprintReadOnly) FSSSurvivorStats Stats;
    UPROPERTY(BlueprintReadOnly) bool bAlive = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bGiveFood = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bGiveWater = false;
};
