#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SSItemDefinition.generated.h"

class UStaticMesh;
class UTexture2D;

// 아이템 종류 하나의 공용 데이터. 보유 수량은 FSSItemStack에서 관리한다.
UCLASS(BlueprintType)
class SIXTYSECONDS_API USSItemDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	USSItemDefinition();

	// 저장/불러오기 등에 사용할 고유 ID. 아이템마다 중복 없이 지정한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SS|Item")
	FName ItemId = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SS|Item")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SS|Item|Visual")
	TObjectPtr<UStaticMesh> WorldMesh = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SS|Item|Visual")
	TObjectPtr<UTexture2D> Icon = nullptr;

	// 아이템 한 개가 차지하는 운반 칸 수. 가방의 최대 칸 수와는 별개다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SS|Item|Carry", meta=(ClampMin="1", UIMin="1"))
	int32 CarryCost = 1;
};
