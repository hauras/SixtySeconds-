#include "Exploration/SSExplorationSession.h"
#include "Exploration/SSExplorationMapDefinition.h"

bool USSExplorationSession::Initialize(USSExplorationMapDefinition* InMap, int32 TurnBudget)
{
	// 실패해도 이전 지도의 상태가 남지 않도록 먼저 비운다
	Map = nullptr;
	Adjacency.Reset();
	State = FState();

	if (!IsValid(InMap)) return false;
	TArray<FText> Errors;
	if (!InMap->Validate(Errors))
	{
		for (const FText& Error : Errors)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Exploration] %s"), *Error.ToString());
		}
		return false;
	}

	Map = InMap;
	Adjacency = Map->BuildAdjacency();
	State.CurrentRoom = Map->FindRoomIndex(Map->EntranceRoomId);
	State.TurnBudget = TurnBudget > 0 ? TurnBudget : Map->MaxTurns; // 0 이하면 지도 기본값 (테스트·디버그용)
	State.RemainingTurns = State.TurnBudget;
	State.PatrolStep = 0;
	State.GuardRoom = Map->FindRoomIndex(Map->PatrolRoute[State.PatrolStep]);

	for (const FSSExplorationRoom& Room : Map->Rooms)
	{
		State.RoomLoot.Add(Room.Loot);
	}
	return true;
}

// 조회
bool USSExplorationSession::CanMoveTo(int32 RoomIndex) const
{
	if (!CanAct()) return false;
	if (!Adjacency.IsValidIndex(State.CurrentRoom)) return false;
	if (!Adjacency.IsValidIndex(RoomIndex)) return false;
	if (!Adjacency[State.CurrentRoom].Contains(RoomIndex)) return false;

	return true;
}

int32 USSExplorationSession::GetCarriedLoad() const
{
	int32 Load = 0;
	for (const FSSItemStack& Stack : State.Carried)
	{
		if (!IsValid(Stack.Item)) continue;

		Load += Stack.Item->CarryCost * Stack.Quantity;
	}
	return Load;
}

const TArray<FSSItemStack>& USSExplorationSession::GetCurrentRoomLoot() const
{
	static const TArray<FSSItemStack> Empty;

	if (!State.RoomLoot.IsValidIndex(State.CurrentRoom)) return Empty;

	return State.RoomLoot[State.CurrentRoom];
}

int32 USSExplorationSession::GetGuardNextRoom() const
{
	if (!IsValid(Map) || Map->PatrolRoute.IsEmpty()) return INDEX_NONE;

	const int32 NextStep = (State.PatrolStep + 1) % Map->PatrolRoute.Num();
	return Map->FindRoomIndex(Map->PatrolRoute[NextStep]);
}

// 플레이어 행동 (한 턴 진행)
bool USSExplorationSession::MoveTo(int32 RoomIndex)
{
	if (!CanMoveTo(RoomIndex)) return false;

	const int32 PlayerBefore = State.CurrentRoom;
	const int32 GuardBefore = State.GuardRoom;

	State.CurrentRoom = RoomIndex;

	// 경비 있는 방에 들어갔는지
	if (State.CurrentRoom == State.GuardRoom)
	{
		State.Outcome = ESSExplorationOutcome::Caught;
	}

	EndTurn(PlayerBefore, GuardBefore);

	return true;
}

bool USSExplorationSession::Wait()
{
	if (!CanAct()) return false;

	const int32 PlayerBefore = State.CurrentRoom;
	const int32 GuardBefore = State.GuardRoom;
	EndTurn(PlayerBefore, GuardBefore);
	return true;
}

bool USSExplorationSession::CanAct() const
{
	return State.Outcome == ESSExplorationOutcome::InProgress && State.RemainingTurns > 0;
}

bool USSExplorationSession::CanSearch() const
{
	if (!CanAct()) return false;
	if (!Map->Rooms[State.CurrentRoom].bSearchable) return false;
	if (State.SearchedRooms.Contains(State.CurrentRoom)) return false;

	return true;
}

bool USSExplorationSession::Search()
{
	if (!CanSearch()) return false;

	const int32 PlayerBefore = State.CurrentRoom;
	const int32 GuardBefore = State.GuardRoom;
	State.SearchedRooms.Add(State.CurrentRoom);

	// 담을 수 있는 첫 물자를 하나씩 담는다. 담을 때마다 목록이 줄거나 당겨지므로 매번 처음부터 다시 찾음
	bool bTookAny = true;
	while (bTookAny)
	{
		bTookAny = false;
		const int32 Count = State.RoomLoot[State.CurrentRoom].Num();
		for (int32 i = 0; i < Count; ++i)
		{
			if (TakeItem(i))
			{
				bTookAny = true;
				break;
			}
		}
	}

	EndTurn(PlayerBefore, GuardBefore); // 방송은 여기서 한 번
	return true;
}

bool USSExplorationSession::CanTake(int32 LootIndex) const
{
	if (State.Outcome != ESSExplorationOutcome::InProgress) return false;
	if (!State.SearchedRooms.Contains(State.CurrentRoom)) return false;
	if (!State.RoomLoot.IsValidIndex(State.CurrentRoom)) return false;
	if (!State.RoomLoot[State.CurrentRoom].IsValidIndex(LootIndex)) return false; // 그 방 목록에 이 번호가 있나

	const FSSItemStack& Stack = State.RoomLoot[State.CurrentRoom][LootIndex];
	if (!IsValid(Stack.Item) || Stack.Quantity <= 0) return false;

	if (GetCarriedLoad() + Stack.Item->CarryCost > Map->CarryCapacity) return false;

	return true;
}

bool USSExplorationSession::TakeItem(int32 LootIndex)
{
	if (!CanTake(LootIndex)) return false;

	FSSItemStack& Source = State.RoomLoot[State.CurrentRoom][LootIndex];

	// 운반함에 같은 아이템이 있으면 개수 +1, 없으면 개수 1짜리 새 칸
	FSSItemStack* Existing = State.Carried.FindByPredicate([&Source](const FSSItemStack& S)
	{
		return S.Item == Source.Item;
	});
	if (Existing)
	{
		++Existing->Quantity;
	}
	else
	{
		FSSItemStack NewStack;
		NewStack.Item = Source.Item;
		NewStack.Quantity = 1;
		State.Carried.Add(NewStack);
	}

	// 방 물자 -1, 다 가져갔으면 목록에서 제거 (Source는 이 뒤로 쓰지 않음)
	--Source.Quantity;
	if (Source.Quantity <= 0)
	{
		State.RoomLoot[State.CurrentRoom].RemoveAt(LootIndex);
	}

	return true; // 방송은 부른 쪽(Search)의 EndTurn이 한 번에 함
}

bool USSExplorationSession::CanReturn() const
{
	if (State.Outcome != ESSExplorationOutcome::InProgress) return false;
	if (State.CurrentRoom != Map->FindRoomIndex(Map->ExitRoomId)) return false;
	return true;
}

bool USSExplorationSession::ReturnToShelter()
{
	if (!CanReturn()) return false;
	State.Outcome = ESSExplorationOutcome::Returned;
	OnExplorationChanged.Broadcast();
	return true;
}

FSSExplorationResult USSExplorationSession::MakeResult() const
{
	FSSExplorationResult Result;
	Result.Outcome = State.Outcome;
	Result.Items = State.Carried; // 실패해도 채움: 결과창이 잃은 물품을 흐리게 보여줌. 입고 여부는 Outcome으로 판단
	const bool bFailed = State.Outcome == ESSExplorationOutcome::Caught || State.Outcome == ESSExplorationOutcome::TimeOut;
	Result.Injury = bFailed && IsValid(Map) ? Map->EmergencyInjury : 0.f;
	Result.TurnsUsed = State.TurnBudget - State.RemainingTurns;
	return Result;
}

void USSExplorationSession::EndTurn(int32 PlayerBefore, int32 GuardBefore)
{
	if (State.Outcome == ESSExplorationOutcome::InProgress)
	{
		AdvanceGuard();
		if (CheckCaught(PlayerBefore, GuardBefore))
		{
			State.Outcome = ESSExplorationOutcome::Caught;
		}
	}

	--State.RemainingTurns;

	if (State.RemainingTurns == 0 && State.Outcome == ESSExplorationOutcome::InProgress)
	{
		State.Outcome = ESSExplorationOutcome::TimeOut;
	}

	OnExplorationChanged.Broadcast();
}

// 내부 도우미
void USSExplorationSession::AdvanceGuard()
{
	if (!IsValid(Map) || Map->PatrolRoute.IsEmpty()) return;

	State.PatrolStep = (State.PatrolStep + 1) % Map->PatrolRoute.Num();
	State.GuardRoom = Map->FindRoomIndex(Map->PatrolRoute[State.PatrolStep]);
}

// 발각 검사: 같은 방이거나, 서로 자리를 바꿨으면(통로에서 스침) 발각
bool USSExplorationSession::CheckCaught(int32 PlayerBefore, int32 GuardBefore) const
{
	if (State.CurrentRoom == State.GuardRoom || (State.CurrentRoom == GuardBefore && State.GuardRoom == PlayerBefore)) return true;
	return false;
}
