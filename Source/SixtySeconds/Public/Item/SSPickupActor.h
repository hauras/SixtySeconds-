#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Item/SSInventoryTypes.h"
#include "SSPickupActor.generated.h"

class USSCarryComponent;
class UStaticMeshComponent;
class USphereComponent;

// 맵에 놓인 아이템. 캐릭터가 E키로 상호작용하면 가방에 추가되고 사라짐
UCLASS()
class SIXTYSECONDS_API ASSPickupActor : public AActor
{
	GENERATED_BODY()

public:
	ASSPickupActor();

	// ASSCharacter에서 호출. 성공 시 true 반환하고 액터 제거
	UFUNCTION(BlueprintCallable, Category="SS|Pickup")
	bool TryPickup(USSCarryComponent* CarryComponent);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Pickup")
	FSSItemStack ItemStack;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SS|Pickup")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	// 상호작용 감지 범위
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SS|Pickup")
	TObjectPtr<USphereComponent> InteractionSphere;
};
