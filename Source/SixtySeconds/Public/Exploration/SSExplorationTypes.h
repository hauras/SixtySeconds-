#pragma once

#include "CoreMinimal.h"
#include "Item/SSInventoryTypes.h"
#include "SSExplorationTypes.generated.h"

UENUM(BlueprintType)
enum class ESSExplorationAction : uint8
{
	Move,
	Search,
	Wait,
	Return,
};

UENUM(BlueprintType)
enum class ESSGuardState : uint8
{
	Patrol,      // 경계도 낮음
	Investigate, // 경계도 높음 — 소음 위치로 이동
	Alerted,     // 발각
};

UENUM(BlueprintType)
enum class ESSExplorationOutcome : uint8
{
	InProgress,
	Returned, // 출구에서 정상 귀환
	Caught,   // 발각 — 비상 귀환
	TimeOut,  // 턴 소진 — 비상 귀환
};

// 지도 위 구역 하나. 화면 위치는 탐사 WBP에 배치한 방 위젯이 정한다.
USTRUCT(BlueprintType)
struct SIXTYSECONDS_API FSSExplorationRoom
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Exploration")
	FName RoomId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Exploration")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Exploration")
	bool bSearchable = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Exploration", meta=(EditCondition="bSearchable"))
	TArray<FSSItemStack> Loot;
};

// 두 구역을 잇는 양방향 통로
USTRUCT(BlueprintType)
struct SIXTYSECONDS_API FSSExplorationPassage
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Exploration")
	FName RoomA = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Exploration")
	FName RoomB = NAME_None;
};

// 탐사 종료 후 은신처에 반영할 결과
USTRUCT(BlueprintType)
struct SIXTYSECONDS_API FSSExplorationResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) ESSExplorationOutcome Outcome = ESSExplorationOutcome::InProgress;
	UPROPERTY(BlueprintReadOnly) TArray<FSSItemStack> Items;
	UPROPERTY(BlueprintReadOnly) float Injury = 0.f;
	UPROPERTY(BlueprintReadOnly) int32 TurnsUsed = 0;
};
