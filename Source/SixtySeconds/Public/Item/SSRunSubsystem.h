#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Item/SSInventoryTypes.h"
#include "SSRunSubsystem.generated.h"

// 씬 전환(스크램블→은신처)에도 유지되는 플레이 데이터 관리
UCLASS()
class SIXTYSECONDS_API USSRunSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// 운반 목록을 은신처 보관함으로 이전
	UFUNCTION(BlueprintCallable, Category="SS|Run")
	void DepositItems(const TArray<FSSItemStack>& CarriedItems);

	// 보관된 아이템 수량 반환 (없으면 0)
	UFUNCTION(BlueprintPure, Category="SS|Run")
	int32 GetStoredQuantity(USSItemDefinition* Item) const;

	UFUNCTION(BlueprintPure, Category="SS|Run")
	const TArray<FSSItemStack>& GetStoredItems() const { return StoredItems; }

	// 새 게임 시작 시 초기화
	UFUNCTION(BlueprintCallable, Category="SS|Run")
	void ResetRun();

	void InitializeShelterStats(float InHealth, float InSatiety, float InHydration);
	
	int32 GetCurrentDay() const { return CurrentDay; }
	float GetHealth() const { return Health; }
	float GetSatiety() const { return Satiety; }
	float GetHydration() const { return Hydration; }

	bool AdvanceDay(bool bGiveFood, bool bGiveWater);

	int32 GetStoredQuantityById(FName ItemId) const;
private:
	bool ConsumeItem(FName ItemID);

	UPROPERTY()
	TArray<FSSItemStack> StoredItems;

	UPROPERTY(Transient)
	int32 CurrentDay = 1;

	UPROPERTY(Transient)
	float Health = 100.f;

	UPROPERTY(Transient)
	float Satiety = 100.f;

	UPROPERTY(Transient)
	float Hydration = 100.f;

};
