#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SSItemDefinition.generated.h"

class UStaticMesh;
class UTexture2D;

USTRUCT(BlueprintType)
struct FSSStockVisual
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Stock", meta=(ClampMin="1"))
	int32 MinQuantity = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Stock")
	TObjectPtr<UTexture2D> Texture = nullptr;
};

UENUM(BlueprintType)
enum class ESSItemType : uint8
{
	Consumable,
	Tool,
	ExplorationResource,
	Clue
};

UENUM(BlueprintType)
enum class ESSItemUseEffect : uint8
{
	None,
	RestoreHealth,
	RestoreSatiety,
	RestoreHydration
};

// 아이템 종류 하나의 공용 데이터. 보유 수량은 FSSItemStack에서 관리한다.
UCLASS(BlueprintType)
class SIXTYSECONDS_API USSItemDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	USSItemDefinition();
	virtual void PostLoad() override;

	// Highest valid threshold <= Quantity; array order does not matter.
	UTexture2D* GetStockTexture(int32 Quantity) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SS|Item|Shelter")
	TArray<FSSStockVisual> ShelterVisuals;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SS|Item")
	ESSItemType ItemType = ESSItemType::Consumable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SS|Item|Use")
	ESSItemUseEffect UseEffect = ESSItemUseEffect::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SS|Item|Use", meta=(ClampMin="0", EditCondition="UseEffect != ESSItemUseEffect::None"))
	float EffectAmount = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SS|Item|Use")
	bool bConsumeOnUse = true;

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

private:
	// One-time migration for assets authored before use settings existed.
	UPROPERTY()
	bool bUseSettingsInitialized = false;
};
