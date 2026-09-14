#pragma once

#include "CoreMinimal.h"
#include "Item/SSItemDefinition.h"
#include "SSInventoryTypes.generated.h"

// 운반 가방과 은신처 보관 목록에서 공통으로 사용하는 아이템/수량 기록.
// 기본값은 빈 기록이며, 실제 목록에 추가할 때 유효한 Item과 양수 수량을 검사한다.
USTRUCT(BlueprintType)
struct SIXTYSECONDS_API FSSItemStack
{
	GENERATED_BODY()

	FSSItemStack();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Inventory")
	TObjectPtr<USSItemDefinition> Item = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Inventory", meta=(ClampMin="0", UIMin="0"))
	int32 Quantity = 0;
};

// 탐사 보상처럼 수량이 범위로 지정되는 아이템 스택
USTRUCT(BlueprintType)
struct SIXTYSECONDS_API FSSItemStackRange
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SS|Inventory")
	TObjectPtr<USSItemDefinition> Item = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SS|Inventory", meta=(ClampMin="0"))
	int32 MinQuantity = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SS|Inventory", meta=(ClampMin="0"))
	int32 MaxQuantity = 1;
};
