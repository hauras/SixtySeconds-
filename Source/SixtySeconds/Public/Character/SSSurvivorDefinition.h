
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Character/SSSurvivorTypes.h"
#include "SSSurvivorDefinition.generated.h"

class UTexture2D;

/**
 * 
 */
UCLASS(BlueprintType)
class SIXTYSECONDS_API USSSurvivorDefinition : public UDataAsset
{
	GENERATED_BODY()
public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SS|Survivor")
	FName SurvivorId;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SS|Survivor")
	FText DisplayName;

	// 정보창에서 보여줄 초상화
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SS|Survivor")
	TObjectPtr<UTexture2D> Portrait;

	// 은신처 화면에 배치할 인물 이미지
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SS|Survivor")
	TObjectPtr<UTexture2D> ShelterImage;

	// 합류 시 사용할 기본 스탯
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SS|Survivor")
	FSSSurvivorStats InitialStats;
};
