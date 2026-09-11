#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Item/SSInventoryTypes.h"
#include "SSExpeditionDefinition.generated.h"

UCLASS(BlueprintType)
class SIXTYSECONDS_API USSExpeditionDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SS|Expedition")
	FText RegionName;

	// 파견 후 귀환까지 걸리는 날 수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SS|Expedition", meta=(ClampMin="1"))
	int32 DurationDays = 1;

	// 파견 비용 (배터리 등)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SS|Expedition")
	TArray<FSSItemStack> Cost;

	// TODO: 성공 확률 추가 (0.0~1.0). 실패 시 보상 없이 귀환.
	// float SuccessRate = 1.0f;

	// TODO: 보상 수량을 고정값 대신 Min/Max 범위로 변경.
	// FSSItemStack → FSSItemStackRange { MinQty, MaxQty } 로 교체 후 랜덤 지급.

	// 귀환 시 지급되는 보상 (현재 고정값)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SS|Expedition")
	TArray<FSSItemStack> Rewards;
};
