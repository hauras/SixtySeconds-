#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Item/SSItemSpawnTypes.h"
#include "SSItemSpawnManager.generated.h"

class ASSPickupActor;
class USSItemSpawnTable;

// ─────────────────────────────────────────────
// 스크램블 아이템 배치 관리자 (맵에 하나)
// 판이 시작되면 맵의 스폰 자리(ASSItemSpawnPoint)를 모아 배치 규칙(USSItemSpawnTable)대로
// 일부를 골라 아이템을 놓음. 놓을 땐 아래로 쏴서 표면을 찾고 메시 바닥을 그 표면에 맞춤
// ─────────────────────────────────────────────
UCLASS()
class SIXTYSECONDS_API ASSItemSpawnManager : public AActor
{
	GENERATED_BODY()

public:
	ASSItemSpawnManager();

	// 배치 규칙 (어떤 아이템을 몇 자리에)
	UPROPERTY(EditAnywhere, Category="SS|Spawn")
	TObjectPtr<USSItemSpawnTable> SpawnTable;

	// 놓을 아이템 액터 클래스 (블루프린트로 모양을 바꿨다면 그것)
	UPROPERTY(EditAnywhere, Category="SS|Spawn")
	TSubclassOf<ASSPickupActor> PickupClass;

	// 0이면 판마다 다름. 숫자를 넣으면 늘 같은 배치 (테스트·시연용)
	UPROPERTY(EditAnywhere, Category="SS|Spawn")
	int32 FixedSeed = 0;

	// 표면을 찾을 때 자리에서 아래로 쏘는 거리 (cm)
	UPROPERTY(EditAnywhere, Category="SS|Spawn", meta=(ClampMin="0"))
	float SurfaceSearchDistance = 150.f;

	// 동료(ASSSurvivorPickup)도 판마다 동료 자리 중 서로 다른 방으로 옮길지
	UPROPERTY(EditAnywhere, Category="SS|Spawn")
	bool bPlaceSurvivors = true;

	// 지금 맵의 아이템 자리를 모아 아이템을 놓음 (BeginPlay에서 부름). 놓은 아이템들을 돌려줌
	TArray<ASSPickupActor*> SpawnItems(int32 Seed);

	// 맵의 동료들을 동료 자리로 옮김 (서로 다른 방 우선). 옮긴 동료 수
	int32 PlaceSurvivors(int32 Seed);

protected:
	virtual void BeginPlay() override;

private:
	// 아이템을 자리에 놓고, 메시 바닥을 아래 표면에 맞춤
	void PlaceOnSurface(ASSPickupActor* Pickup, const FTransform& PointTransform) const;

	// 그 종류의 자리 모으기 (이름순: 같은 시드면 같은 결과)
	TArray<class ASSItemSpawnPoint*> GatherPoints(ESSSpawnPointKind Kind) const;
};
