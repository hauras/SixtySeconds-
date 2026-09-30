#include "Trace/SSTraceMessage.h"

FName FSSTraceMessagePicker::Pick(const UDataTable* Table, int32 Day, const TSet<FName>& Received,
	int32 CompletedCount, int32 TruthEvery, FRandomStream& Random)
{
	if (!IsValid(Table) || Table->GetRowStruct() != FSSTraceMessageRow::StaticStruct()) return NAME_None;

	// 진실 차례면 진실 먼저
	const bool bTruthTurn = TruthEvery > 0 && CompletedCount > 0 && CompletedCount % TruthEvery == 0;
	if (bTruthTurn)
	{
		const FName Truth = PickTruth(Table, Day, Received);
		if (!Truth.IsNone()) return Truth;
	}

	// 실용 정보, 그것도 없으면 남은 진실
	const FName Info = PickInfo(Table, Day, Received, Random);
	return Info.IsNone() ? PickTruth(Table, Day, Received) : Info;
}

FName FSSTraceMessagePicker::PickTruth(const UDataTable* Table, int32 Day, const TSet<FName>& Received)
{
	FName Best = NAME_None;
	int32 BestOrder = TNumericLimits<int32>::Max();

	Table->ForeachRow<FSSTraceMessageRow>(TEXT("PickTruth"), [&](const FName& Id, const FSSTraceMessageRow& Row)
	{
		if (Row.Kind != ESSTraceMessageKind::Truth || Row.MinDay > Day || Received.Contains(Id)) return;
		if (Row.Order < BestOrder)
		{
			BestOrder = Row.Order;
			Best = Id;
		}
	});
	return Best;
}

FName FSSTraceMessagePicker::PickInfo(const UDataTable* Table, int32 Day, const TSet<FName>& Received, FRandomStream& Random)
{
	// 조건이 맞는 후보와 가중치 합
	TArray<TPair<FName, int32>> Candidates;
	int32 TotalWeight = 0;

	Table->ForeachRow<FSSTraceMessageRow>(TEXT("PickInfo"), [&](const FName& Id, const FSSTraceMessageRow& Row)
	{
		if (Row.Kind != ESSTraceMessageKind::Info || Row.MinDay > Day || Received.Contains(Id) || Row.Weight <= 0) return;
		Candidates.Add({ Id, Row.Weight });
		TotalWeight += Row.Weight;
	});
	if (TotalWeight <= 0) return NAME_None;

	// 0 ~ 합-1 사이 숫자를 뽑아, 그 숫자가 떨어지는 칸의 메시지
	int32 Roll = Random.RandRange(0, TotalWeight - 1);
	for (const TPair<FName, int32>& Candidate : Candidates)
	{
		if (Roll < Candidate.Value) return Candidate.Key;
		Roll -= Candidate.Value;
	}
	return Candidates.Last().Key;
}
