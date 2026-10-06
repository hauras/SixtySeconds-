#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Item/SSInventoryTypes.h"
#include "Item/SSInteractable.h"
#include "SSPickupActor.generated.h"

class USSCarryComponent;
class UStaticMeshComponent;
class USphereComponent;

// 맵에 놓인 아이템. 캐릭터가 E키로 상호작용하면 가방에 추가되고 사라짐
UCLASS()
class SIXTYSECONDS_API ASSPickupActor : public AActor, public ISSInteractable
{
	GENERATED_BODY()

public:
	ASSPickupActor();

	// ASSCharacter에서 호출. 성공 시 true 반환하고 액터 제거
	UFUNCTION(BlueprintCallable, Category="SS|Pickup")
	bool TryPickup(USSCarryComponent* CarryComponent);

	// 이 가방에 들어갈 수 있나 (칸이 남았나)
	bool CanPickupWith(const USSCarryComponent* CarryComponent) const;

	// 안내 문장: "식량 · [E] 줍기" / 가방이 꽉 찼으면 "식량 · 가방이 가득 찼다"
	FText GetPickupPrompt(const USSCarryComponent* CarryComponent) const;

	// 놓인 아이템을 정함 (무작위 배치·테스트에서 생성 직후에 부름)
	void SetItemStack(const FSSItemStack& InStack) { ItemStack = InStack; }
	const FSSItemStack& GetItemStack() const { return ItemStack; }

	// ── ISSInteractable ──
	virtual bool CanInteract(const APawn* Interactor) const override;
	virtual FText GetInteractPrompt(const APawn* Interactor) const override;
	virtual bool TryInteract(APawn* Interactor) override;

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
