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

	// 성공 확률 (0.0 = 항상 실패, 1.0 = 항상 성공)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SS|Expedition",
		meta=(ClampMin="0.0", ClampMax="1.0", UIMin="0.0", UIMax="1.0"))
	float SuccessRate = 1.0f;

	// 귀환 후 로봇 고장 확률 (0.0 = 고장 없음, 1.0 = 항상 고장)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SS|Expedition",
		meta=(ClampMin="0.0", ClampMax="1.0", UIMin="0.0", UIMax="1.0"))
	float BreakdownChance = 0.0f;

	// 귀환 시 지급되는 보상 (Min~Max 범위에서 랜덤 지급)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SS|Expedition")
	TArray<FSSItemStackRange> Rewards;
};
