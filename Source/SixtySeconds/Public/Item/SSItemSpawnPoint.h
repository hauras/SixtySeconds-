#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Item/SSItemSpawnTypes.h"
#include "SSItemSpawnPoint.generated.h"

class UArrowComponent;
class UBillboardComponent;

// ─────────────────────────────────────────────
// 아이템이 놓일 수 있는 자리 (선반 위, 책상 위 등. 사람이 맵에 직접 놓음)
// 게임 중엔 안 보임. 판이 시작되면 배치 관리자가 이 자리들 중 일부를 골라 아이템을 놓음
// 자리를 표면 살짝 위에 두면, 놓을 때 아래로 쏴서 표면에 딱 붙임
// ─────────────────────────────────────────────
UCLASS()
class SIXTYSECONDS_API ASSItemSpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	ASSItemSpawnPoint();

	// 어느 방의 자리인지 (방마다 최소 하나는 채움. 비우면 방 보장에서 빠짐)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Spawn")
	FName RoomId = NAME_None;

	// 아이템 자리(선반·책상 위)인지 동료 자리(바닥)인지
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Spawn")
	ESSSpawnPointKind Kind = ESSSpawnPointKind::Item;

private:
	// 에디터에서만 보이는 표시 (위치·방향 확인용)
	UPROPERTY(VisibleAnywhere, Category="SS|Spawn")
	TObjectPtr<UBillboardComponent> Icon;

	UPROPERTY(VisibleAnywhere, Category="SS|Spawn")
	TObjectPtr<UArrowComponent> Arrow;
};
