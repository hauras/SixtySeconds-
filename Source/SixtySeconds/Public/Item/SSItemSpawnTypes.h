#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Math/RandomStream.h"
#include "SSItemSpawnTypes.generated.h"

class USSItemDefinition;

// 스폰 자리의 종류
UENUM(BlueprintType)
enum class ESSSpawnPointKind : uint8
{
	Item,     // 아이템이 놓이는 자리 (선반·책상 위)
	Survivor, // 동료가 서 있는 자리 (바닥)
};

// 무작위로 나올 수 있는 아이템 하나
USTRUCT(BlueprintType)
struct SIXTYSECONDS_API FSSItemSpawnEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Spawn")
	TObjectPtr<USSItemDefinition> Item;

	// 뽑힐 비율 (클수록 자주. 식량 4, 배터리 1 → 식량이 네 배 자주)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Spawn", meta=(ClampMin="0"))
	float Weight = 1.f;

	// 한 자리에 놓이는 개수 범위
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Spawn", meta=(ClampMin="1"))
	int32 MinQuantity = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Spawn", meta=(ClampMin="1"))
	int32 MaxQuantity = 1;
};

// ─────────────────────────────────────────────
// 스크램블 아이템 배치 규칙 (데이터 에셋, 예: DA_ItemSpawn_Lab)
// 맵에 놓인 스폰 자리 중 몇 곳에 무엇을 놓을지
// ─────────────────────────────────────────────
UCLASS(BlueprintType)
class SIXTYSECONDS_API USSItemSpawnTable : public UDataAsset
{
	GENERATED_BODY()

public:
	// 나올 수 있는 아이템과 비율
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Spawn")
	TArray<FSSItemSpawnEntry> Entries;

	// 한 판에 채울 자리 수 (자리가 더 적으면 전부)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Spawn", meta=(ClampMin="0"))
	int32 SpawnCount = 10;
};

// 배치 결과 하나: 몇 번 자리에 어떤 아이템 몇 개
struct FSSItemPlacement
{
	int32 PointIndex = INDEX_NONE;
	int32 EntryIndex = INDEX_NONE;
	int32 Quantity = 1;
};

// ─────────────────────────────────────────────
// 배치 계산 (월드 없이 숫자만. 테스트하기 쉽게 분리)
// 1) 방마다 자리 하나씩 먼저 뽑음 → 어느 방에 가도 뭔가 있음
// 2) 남은 자리를 섞어서 SpawnCount까지 채움
// 3) 자리마다 아이템은 비율대로 추첨, 개수는 범위 안에서
// ─────────────────────────────────────────────
class SIXTYSECONDS_API FSSItemSpawnPlanner
{
public:
	// PointRooms[i] = i번 자리가 속한 방. 같은 시드면 같은 결과
	static TArray<FSSItemPlacement> Plan(const TArray<FName>& PointRooms, const USSItemSpawnTable& Table, FRandomStream& Random);

	// 동료 자리 고르기: Count명을 가능한 한 서로 다른 방에 (방이 모자라면 같은 방도). 같은 시드면 같은 결과
	// 돌려주는 건 자리 번호 목록 (자리가 모자라면 자리 수만큼)
	static TArray<int32> PlanSurvivors(const TArray<FName>& PointRooms, int32 Count, FRandomStream& Random);

	// 비율대로 아이템 하나 추첨 (비율 합이 0이면 INDEX_NONE)
	static int32 PickEntry(const USSItemSpawnTable& Table, FRandomStream& Random);
};
