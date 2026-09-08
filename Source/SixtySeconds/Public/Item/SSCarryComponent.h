#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Item/SSInventoryTypes.h"
#include "SSCarryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSSOnCarryChanged);

// 수집 중 들고 있는 물건만 관리한다. 은신처 보관 목록은 별도로 관리한다.
UCLASS(ClassGroup=(SixtySeconds), meta=(BlueprintSpawnableComponent))
class SIXTYSECONDS_API USSCarryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USSCarryComponent();

	UFUNCTION(BlueprintPure, Category="SS|Carry")
	int32 GetCapacity() const { return FMath::Max(0, Capacity); }

	UFUNCTION(BlueprintPure, Category="SS|Carry")
	int32 GetUsedSlots() const;

	UFUNCTION(BlueprintPure, Category="SS|Carry")
	int32 GetRemainingSlots() const;

	UFUNCTION(BlueprintPure, Category="SS|Carry")
	int32 GetQuantity(USSItemDefinition* Item) const;

	UFUNCTION(BlueprintPure, Category="SS|Carry")
	const TArray<FSSItemStack>& GetItems() const { return Items; }

	UFUNCTION(BlueprintPure, Category="SS|Carry")
	bool CanAddItem(USSItemDefinition* Item, int32 Quantity = 1) const;

	// 요청 수량 전체가 들어갈 때만 변경한다. 부분 획득은 하지 않는다.
	UFUNCTION(BlueprintCallable, Category="SS|Carry")
	bool TryAddItem(USSItemDefinition* Item, int32 Quantity = 1);

	UFUNCTION(BlueprintCallable, Category="SS|Carry")
	bool TryRemoveItem(USSItemDefinition* Item, int32 Quantity = 1);

	// 물자 이전 시에는 보관 성공을 확인한 뒤 호출한다.
	UFUNCTION(BlueprintCallable, Category="SS|Carry")
	void ClearItems();

	UPROPERTY(BlueprintAssignable, Category="SS|Carry")
	FSSOnCarryChanged OnCarryChanged;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SS|Carry", meta=(ClampMin="0", UIMin="0"))
	int32 Capacity = 4;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="SS|Carry", meta=(AllowPrivateAccess="true"))
	TArray<FSSItemStack> Items;
};
