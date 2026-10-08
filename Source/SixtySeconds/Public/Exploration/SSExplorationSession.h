
#pragma once

#include "CoreMinimal.h"
#include "Exploration/SSExplorationTypes.h"
#include "UObject/Object.h"
#include "SSExplorationSession.generated.h"

class USSExplorationMapDefinition;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSSOnExplorationChanged);
/**
 * 
 */
UCLASS()
class SIXTYSECONDS_API USSExplorationSession : public UObject
{
	GENERATED_BODY()
public:
	// TurnBudget: 이번 탐사의 총 턴 (출발 시 행동력으로 계산). 0 이하면 지도의 MaxTurns
	bool Initialize(USSExplorationMapDefinition* InMap, int32 TurnBudget = 0);

	// 이동 함수
	bool CanMoveTo(int32 RoomIndex) const;
	bool MoveTo(int32 RoomIndex);

	// 대기 함수
	bool Wait();
	bool CanAct() const; // 이동·대기·수색 공통: 진행 중이고 턴이 남았는지

	// 수색 함수: 수색하면 방 물자가 목록 순서대로 운반함에 들어가는 만큼 자동으로 담긴다
	bool CanSearch() const;
	bool Search();

	//귀환 함수
	bool CanReturn() const;
	bool ReturnToShelter();

	const USSExplorationMapDefinition* GetMap() const { return Map; }
	int32 GetCurrentRoom() const { return State.CurrentRoom; }
	int32 GetRemainingTurns() const { return State.RemainingTurns; }
	int32 GetTurnBudget() const { return State.TurnBudget; }
	int32 GetGuardRoom() const { return State.GuardRoom; }
	ESSExplorationOutcome GetOutcome() const { return State.Outcome; }

	int32 GetCarriedLoad() const;

	const TArray<FSSItemStack>& GetCarried() const { return State.Carried; } // 운반함 표시용
	const TArray<FSSItemStack>& GetCurrentRoomLoot() const;
	bool IsCurrentRoomSearched() const { return State.SearchedRooms.Contains(State.CurrentRoom); }

	UPROPERTY(BlueprintAssignable, Category = "SS|Exploration")
	FSSOnExplorationChanged OnExplorationChanged;

	int32 GetGuardNextRoom() const;

	// 탐사 종료 후 결과창과 은신처 정산에 넘길 요약
	FSSExplorationResult MakeResult() const;

private:
	UPROPERTY()
	TObjectPtr<USSExplorationMapDefinition> Map;

	TArray<TArray<int32>> Adjacency; // 지도에서 만드는 준비물이라 State 밖에 둠

	// 탐사 한 판의 진행 상태. 시작값은 여기에만 적고, 비울 때는 State = FState()
	struct FState
	{
		int32 CurrentRoom = INDEX_NONE;
		int32 RemainingTurns = 0;
		int32 TurnBudget = 0; // 이번 탐사의 총 턴
		int32 GuardRoom = INDEX_NONE;
		int32 PatrolStep = 0; // 순찰 경로 배열 안의 순서 (방 번호 아님)
		ESSExplorationOutcome Outcome = ESSExplorationOutcome::InProgress;
		TSet<int32> SearchedRooms;
		TArray<TArray<FSSItemStack>> RoomLoot;
		TArray<FSSItemStack> Carried;
	};
	FState State;

	void EndTurn(int32 PlayerBefore, int32 GuardBefore);
	void AdvanceGuard();

	bool CheckCaught(int32 PlayerBefore, int32 GuardBefore) const;

	// 챙기기: Search 안에서만 사용 (방송은 Search의 EndTurn이 한 번에)
	bool CanTake(int32 LootIndex) const;
	bool TakeItem(int32 LootIndex);
};
