#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Exploration/SSExplorationTypes.h"
#include "SSExplorationMapDefinition.generated.h"

// 직접 탐사 지도 하나의 고정 데이터: 구역, 통로, 순찰 경로, 규칙 수치
UCLASS(BlueprintType)
class SIXTYSECONDS_API USSExplorationMapDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SS|Exploration")
	TArray<FSSExplorationRoom> Rooms;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SS|Exploration")
	TArray<FSSExplorationPassage> Passages;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SS|Exploration")
	FName EntranceRoomId = NAME_None;

	// 귀환은 출구에서만 가능
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SS|Exploration")
	FName ExitRoomId = NAME_None;

	// 첫 구역이 경비 로봇 시작 위치. 마지막 구역에서 첫 구역으로 되돌아가며 순환한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SS|Exploration|Guard")
	TArray<FName> PatrolRoute;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SS|Exploration|Rules", meta=(ClampMin="1"))
	int32 MaxTurns = 12;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SS|Exploration|Rules", meta=(ClampMin="1"))
	int32 CarryCapacity = 4;

	// 수색 소음이 들리는 거리 (통로 칸 수)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SS|Exploration|Guard", meta=(ClampMin="0"))
	int32 NoiseRange = 2;

	// 발각·턴 소진 시 체력 감소량
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SS|Exploration|Rules", meta=(ClampMin="0"))
	float EmergencyInjury = 30.f;

	int32 FindRoomIndex(FName RoomId) const;
	TArray<TArray<int32>> BuildAdjacency() const;

	// BFS 최단 경로 (시작·도착 포함). 연결되지 않으면 빈 배열.
	static TArray<int32> FindShortestPath(const TArray<TArray<int32>>& Adjacency, int32 From, int32 To);

	// 구역 ID 중복, 잘못된 통로, 입구-출구 연결, 순찰 경로 인접 여부 검사
	bool Validate(TArray<FText>& OutErrors) const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
