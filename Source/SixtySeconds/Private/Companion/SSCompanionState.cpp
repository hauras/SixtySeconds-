#include "Companion/SSCompanionState.h"
#include "Character/SSSurvivorDefinition.h"
#include "Item/SSRunSubsystem.h"
#include "Ara/SSAraDirector.h"
#include "Item/SSItemDefinition.h"
#include "Engine/DataTable.h"

USSRunSubsystem& USSCompanionState::GetRun() const
{
	return *CastChecked<USSRunSubsystem>(GetOuter());
}

const FSSCompanionRecord* USSCompanionState::FindRecord(FName SurvivorId) const
{
	for (const FSSCompanionRecord& Record : Records)
	{
		if (Record.SurvivorId == SurvivorId) return &Record;
	}
	return nullptr;
}

FSSCompanionRecord& USSCompanionState::FindOrAddRecord(FName SurvivorId)
{
	for (FSSCompanionRecord& Record : Records)
	{
		if (Record.SurvivorId == SurvivorId) return Record;
	}

	// 처음 조사하거나 지시할 때 기록이 생김
	FSSCompanionRecord& NewRecord = Records.AddDefaulted_GetRef();
	NewRecord.SurvivorId = SurvivorId;
	return NewRecord;
}

void USSCompanionState::RunNight()
{
	USSRunSubsystem& Run = GetRun();

	// 몸 상태(살았는지, 성향 데이터)는 읽기만
	for (const FSSSurvivorState& Survivor : Run.GetRescuedSurvivors())
	{
		if (!Survivor.bAlive || !IsValid(Survivor.Definition)) continue;

		FSSCompanionRecord& Record = FindOrAddRecord(Survivor.Definition->SurvivorId);

		// 이미 아는 단서 (앞 동료가 이번 밤에 찾은 것도 포함되도록 동료마다 다시 셈)
		const TSet<FName> KnownClueIds = GetKnownClueIds();

		// 단서를 다 찾은 장소 (단서 표가 있을 때만)
		TSet<ESSInvestigationSpot> ExhaustedSpots;
		if (ClueTable)
		{
			for (int32 Index = 0; Index < int32(ESSInvestigationSpot::Count); ++Index)
			{
				const ESSInvestigationSpot Each = ESSInvestigationSpot(Index);
				if (FSSInvestigation::PickNextClue(*ClueTable, Each, KnownClueIds).IsNone()) ExhaustedSpots.Add(Each);
			}
		}

		// 정해준 장소가 있으면 거기, 없으면 성향대로 (정해준 건 한 번 쓰면 사라짐)
		const bool bWasOrdered = Record.bHasOrder;
		const ESSInvestigationSpot Spot = bWasOrdered
			? Record.OrderedSpot
			: FSSInvestigation::PickSpot(*Survivor.Definition, InvestigationRandom, ExhaustedSpots);
		Record.bHasOrder = false;

		// 안드로이드: 조사하는 척만 함 (단서 없음, 아라 편이라 들키지도 않음)
		// 정해준 장소가 있으면 가끔 다른 곳을 조사했다고 말함 → 플레이어가 눈치챌 단서
		if (Record.bIsAndroid)
		{
			ESSInvestigationSpot ReportedSpot = Spot;
			if (bWasOrdered && InvestigationRandom.FRand() < AndroidWrongSpotChance)
			{
				// 정해준 장소를 뺀 나머지 중 하나
				const int32 SpotCount = int32(ESSInvestigationSpot::Count);
				const int32 Offset = InvestigationRandom.RandRange(1, SpotCount - 1);
				ReportedSpot = ESSInvestigationSpot((int32(Spot) + Offset) % SpotCount);
			}
			AddReport(Record, ReportedSpot, false, NAME_None, false);
			continue;
		}

		const FSSInvestigationResult Result = FSSInvestigation::Resolve(*Survivor.Definition, Spot, InvestigationRandom);

		// 찾았으면 그 장소의 다음 단계 단서를 정함
		// 다 찾은 장소면 새 단서 없음 ("더 나올 게 없다" 대사)
		bool bFoundClue = Result.bFoundClue;
		FName ClueId = NAME_None;
		bool bSpotExhausted = false;
		if (bFoundClue && ClueTable)
		{
			ClueId = FSSInvestigation::PickNextClue(*ClueTable, Spot, KnownClueIds);
			if (ClueId.IsNone())
			{
				bFoundClue = false;
				bSpotExhausted = true;
			}
		}

		// 단서는 우선 동료만 알고 있음 (대화로 들어야 플레이어에게 공유)
		if (bFoundClue) ++Record.CluesFound;

		// 들킨 건 플레이어에게 말하지 않음 (아라만 앎)
		if (Result.bDetected) ++Record.TimesDetected;

		// 아라는 들켰는지만 봄 (단서를 찾았는지는 모름)
		Run.GetAra()->ObserveNight(Record.SurvivorId, Spot, Result.bDetected);

		AddReport(Record, Spot, bFoundClue, ClueId, bSpotExhausted);
	}

	Run.OnSurvivorsChanged.Broadcast();
}

void USSCompanionState::AddReport(FSSCompanionRecord& Record, ESSInvestigationSpot Spot, bool bFoundClue, FName ClueId, bool bSpotExhausted)
{
	// 단서 없는 지난 보고는 새 보고로 대체 ("특이 사항 없음"이 쌓이지 않게)
	// 단서가 있는 보고는 들을 때까지 남김 (하루 대화를 놓쳐도 단서는 안 사라짐)
	Record.PendingReports.RemoveAll([](const FSSInvestigationReport& Old)
	{
		return !Old.bFoundClue;
	});

	// 아침에 들을 보고
	FSSInvestigationReport& Report = Record.PendingReports.AddDefaulted_GetRef();
	Report.SurvivorId = Record.SurvivorId;
	Report.Day = GetRun().GetCurrentDay();
	Report.Spot = Spot;
	Report.bFoundClue = bFoundClue;
	Report.ClueId = ClueId;
	Report.bSpotExhausted = bSpotExhausted;
}

void USSCompanionState::MakeAndroid(FName SurvivorId)
{
	FSSCompanionRecord& Record = FindOrAddRecord(SurvivorId);
	Record.bIsAndroid = true;

	// 진짜가 아직 말하지 못한 보고는 아라가 기억째 가져감 (안드로이드가 단서를 대신 전하지 않게)
	Record.PendingReports.Reset();
}

bool USSCompanionState::CanInspect(FName SurvivorId) const
{
	const USSRunSubsystem& Run = GetRun();
	const FSSSurvivorState* Survivor = Run.FindRescuedSurvivor(SurvivorId);
	return Survivor && Survivor->bAlive && Run.GetActionPoints() >= InspectActionCost && Run.GetStoredQuantityById(SSItemIds::Battery) >= InspectBatteryCost;
}

bool USSCompanionState::InspectCompanion(FName SurvivorId)
{
	if (!CanInspect(SurvivorId)) return false;
	USSRunSubsystem& Run = GetRun();

	// 비용: 배터리를 먼저 빼고(실패하면 아무것도 안 바뀜), 행동력
	if (!Run.RemoveStoredItemsById(SSItemIds::Battery, InspectBatteryCost)) return false;
	Run.SpendActionPoints(InspectActionCost);

	FSSCompanionRecord& Record = FindOrAddRecord(SurvivorId);
	Record.InspectedDay = Run.GetCurrentDay();
	// 사람은 늘 정상. 안드로이드는 위장해서 가끔 정상으로 나옴 (거짓 음성만, 거짓 양성은 없음)
	// → "이상"이면 확실, "정상"이면 아직 모름. 단서·대화와 같이 판단하게
	Record.bInspectedAndroid = Record.bIsAndroid && InvestigationRandom.FRand() >= AndroidMaskChance;

	// 기록창에도 남김
	const FSSSurvivorState* Survivor = Run.FindRescuedSurvivor(SurvivorId);
	const FText Name = IsValid(Survivor->Definition) ? Survivor->Definition->DisplayName : FText::FromName(SurvivorId);
	Run.AddJournal(ESSJournalEvent::Investigation, FText::Format(NSLOCTEXT("SSJournal", "Inspection", "{0} 검사 — {1}"), Name, GetInspectionText(Record.bInspectedAndroid)));

	Run.OnSurvivorsChanged.Broadcast();
	return true;
}

FText USSCompanionState::GetInspectionText(bool bAndroid)
{
	return bAndroid
		? NSLOCTEXT("SSCompanion", "InspectAndroid", "체온이 실내 온도와 같다. 맥박이 지나치게 규칙적이다.")
		: NSLOCTEXT("SSCompanion", "InspectHuman", "맥박과 체온 모두 정상이다.");
}

bool USSCompanionState::IsolateCompanion(FName SurvivorId, bool& bOutWasAndroid)
{
	USSRunSubsystem& Run = GetRun();
	const FSSSurvivorState* Survivor = Run.FindRescuedSurvivor(SurvivorId);
	if (!Survivor || !Survivor->bAlive) return false;

	const FText Name = IsValid(Survivor->Definition) ? Survivor->Definition->DisplayName : FText::FromName(SurvivorId);
	bOutWasAndroid = IsAndroid(SurvivorId);

	// 조사 기록은 새로 (안드로이드 표시·밀린 보고 정리. 나중에 진짜를 구출하면 처음부터)
	FSSCompanionRecord& Record = FindOrAddRecord(SurvivorId);
	Record = FSSCompanionRecord();
	Record.SurvivorId = SurvivorId;

	if (bOutWasAndroid)
	{
		// 안드로이드: 은신처에서 제거 (진짜는 여전히 B2에 있음)
		Run.RemoveRescuedSurvivor(SurvivorId);
		Run.AddJournal(ESSJournalEvent::Investigation, FText::Format(NSLOCTEXT("SSJournal", "IsolateAndroid", "{0}의 모습을 한 그것을 문밖으로 내보냈다. 끌려 나가는 동안 한 번도 소리를 내지 않았다."), Name));
	}
	else
	{
		// 사람: 아라가 데려감 (B2). 아라가 바라던 일
		Run.MoveSurvivorToCaptured(SurvivorId);
		Run.AddJournal(ESSJournalEvent::Investigation, FText::Format(NSLOCTEXT("SSJournal", "IsolateHuman", "{0}을(를) 문밖으로 내보냈다. 잠시 뒤 단말이 켜졌다. '보호 대상을 인수했습니다. 협조에 감사드립니다.'"), Name));
	}

	// 갱신 알림은 목록을 옮기거나 지울 때 이미 보냄 (여기서 또 보내면 HUD가 두 번 갱신됨)
	return true;
}

bool USSCompanionState::IsAndroid(FName SurvivorId) const
{
	const FSSCompanionRecord* Record = FindRecord(SurvivorId);
	return Record && Record->bIsAndroid;
}

void USSCompanionState::RestoreHuman(FName SurvivorId)
{
	FSSCompanionRecord* Record = Records.FindByPredicate([SurvivorId](const FSSCompanionRecord& Each)
	{
		return Each.SurvivorId == SurvivorId;
	});
	if (!Record) return;

	Record->bIsAndroid = false;

	// 검사 결과는 안드로이드를 찍은 사진이었으니 지움
	Record->InspectedDay = 0;
	Record->bInspectedAndroid = false;

	// 안드로이드가 만든 가짜 보고도 지움
	Record->PendingReports.Reset();
}

void USSCompanionState::QueueTestimony(FName SurvivorId)
{
	FindOrAddRecord(SurvivorId).bPendingTestimony = true;
}

bool USSCompanionState::HearTestimony(FName SurvivorId, FText& OutLine)
{
	FSSCompanionRecord* Record = Records.FindByPredicate([SurvivorId](const FSSCompanionRecord& Each)
	{
		return Each.SurvivorId == SurvivorId;
	});
	if (!Record || !Record->bPendingTestimony) return false;

	Record->bPendingTestimony = false;
	OutLine = GetLine(SurvivorId, ESSCompanionLine::Testimony);
	return true;
}

void USSCompanionState::HearClueDirectly(FName ClueId)
{
	if (ClueId.IsNone() || HasHeardClue(ClueId)) return;

	FSSInvestigationReport& Report = HeardClues.AddDefaulted_GetRef();
	Report.bFoundClue = true;
	Report.ClueId = ClueId;
}

bool USSCompanionState::IsClueKnownOrPending(FName ClueId) const
{
	if (HasHeardClue(ClueId)) return true;

	const USSRunSubsystem& Run = GetRun();
	for (const FSSCompanionRecord& Record : Records)
	{
		const FSSSurvivorState* Survivor = Run.FindRescuedSurvivor(Record.SurvivorId);
		if (!Survivor || !Survivor->bAlive || Record.bIsAndroid) continue;

		const bool bHolding = Record.PendingReports.ContainsByPredicate([ClueId](const FSSInvestigationReport& Report)
		{
			return Report.ClueId == ClueId;
		});
		if (bHolding) return true;
	}
	return false;
}

void USSCompanionState::QueueClueReport(FName SurvivorId, ESSInvestigationSpot Spot, FName ClueId)
{
	if (SurvivorId.IsNone() || ClueId.IsNone()) return;
	AddReport(FindOrAddRecord(SurvivorId), Spot, true, ClueId, false);
}

bool USSCompanionState::HasHeardClue(FName ClueId) const
{
	return !ClueId.IsNone() && HeardClues.ContainsByPredicate([ClueId](const FSSInvestigationReport& Report)
	{
		return Report.ClueId == ClueId;
	});
}

bool USSCompanionState::OrderInvestigation(FName SurvivorId, ESSInvestigationSpot Spot)
{
	USSRunSubsystem& Run = GetRun();

	// 1. 살아 있는 동료인지
	const FSSSurvivorState* Survivor = Run.FindRescuedSurvivor(SurvivorId);
	if (!Survivor || !Survivor->bAlive) return false;
	if (Spot >= ESSInvestigationSpot::Count) return false;

	// 2. 오늘 이미 정해줬는지
	FSSCompanionRecord& Record = FindOrAddRecord(SurvivorId);
	if (Record.bHasOrder) return false;

	// 3. 행동력 차감 (실패하면 아무것도 안 바뀜 → 연속 클릭해도 한 번만)
	if (!Run.SpendActionPoints(InvestigationOrderCost)) return false;

	// 4. 저장
	Record.bHasOrder = true;
	Record.OrderedSpot = Spot;
	Run.OnSurvivorsChanged.Broadcast();
	return true;
}

bool USSCompanionState::HearInvestigationReport(FName SurvivorId, FSSInvestigationReport& OutReport)
{
	USSRunSubsystem& Run = GetRun();

	const FSSSurvivorState* Survivor = Run.FindRescuedSurvivor(SurvivorId);
	if (!Survivor || !Survivor->bAlive || !IsValid(Survivor->Definition)) return false;

	FSSCompanionRecord& Record = FindOrAddRecord(SurvivorId);
	if (!Record.HasPendingReport()) return false;

	// 가장 오래된 보고부터 꺼냄
	OutReport = Record.PendingReports[0];
	Record.PendingReports.RemoveAt(0);
	OutReport.HeardDay = Run.GetCurrentDay();

	// 단서가 있으면 "들은 단서" 목록에 남김 (추리·사건에서 씀)
	if (OutReport.bFoundClue) HeardClues.Add(OutReport);

	// 대피실 대화는 아라가 엿들을 수 있음 (단말 로그 1단계 단서: 02:00 음성 기록 조회)
	Run.GetAra()->TryEavesdrop(SurvivorId, OutReport.bFoundClue);

	// 들은 순간에만 기록 (단서 내용은 대사 데이터 단계에서 장소·동료별 문장으로 바뀜)
	// 단서가 있으면 단서 제목과 내용, 없으면 "특이 사항 없음"
	FText Outcome = NSLOCTEXT("SSJournal", "InvestigationNothing", "특이 사항 없음.");
	if (OutReport.bFoundClue)
	{
		const FSSClueRow* Clue = FindClue(OutReport.ClueId);
		Outcome = Clue
			? FText::Format(NSLOCTEXT("SSJournal", "InvestigationClueText", "[{0}] {1}"), Clue->Title, Clue->Text)
			: NSLOCTEXT("SSJournal", "InvestigationClue", "수상한 흔적을 찾았다.");
	}
	Run.AddJournal(ESSJournalEvent::Investigation, FText::Format(NSLOCTEXT("SSJournal", "InvestigationReport", "{0}의 보고 ({1}일째 밤) — {2}: {3}"), Survivor->Definition->DisplayName, OutReport.Day, FSSInvestigation::GetSpotName(OutReport.Spot), Outcome));

	Run.OnSurvivorsChanged.Broadcast();
	return true;
}

TSet<FName> USSCompanionState::GetKnownClueIds() const
{
	TSet<FName> Known;
	for (const FSSInvestigationReport& Heard : HeardClues)
	{
		if (!Heard.ClueId.IsNone()) Known.Add(Heard.ClueId);
	}
	for (const FSSCompanionRecord& Record : Records)
	{
		for (const FSSInvestigationReport& Pending : Record.PendingReports)
		{
			if (!Pending.ClueId.IsNone()) Known.Add(Pending.ClueId);
		}
	}
	return Known;
}

void USSCompanionState::SetTables(UDataTable* InClueTable, UDataTable* InLineTable)
{
	// 표의 줄 형식이 맞는지 확인 (다른 표를 잘못 넣으면 안 씀)
	ClueTable = InClueTable && InClueTable->GetRowStruct() == FSSClueRow::StaticStruct() ? InClueTable : nullptr;
	LineTable = InLineTable && InLineTable->GetRowStruct() == FSSCompanionLineRow::StaticStruct() ? InLineTable : nullptr;
	if (InClueTable && !ClueTable) UE_LOG(LogTemp, Warning, TEXT("[Companion] Clue table row struct must be FSSClueRow."));
	if (InLineTable && !LineTable) UE_LOG(LogTemp, Warning, TEXT("[Companion] Line table row struct must be FSSCompanionLineRow."));
}

const FSSClueRow* USSCompanionState::FindClue(FName ClueId) const
{
	if (!ClueTable || ClueId.IsNone()) return nullptr;
	return ClueTable->FindRow<FSSClueRow>(ClueId, TEXT("FindClue"), false);
}

int32 USSCompanionState::CountClues(ESSInvestigationSpot Spot) const
{
	return ClueTable ? FSSInvestigation::CountClues(*ClueTable, Spot) : 0;
}

FText USSCompanionState::FindLine(const FString& RowName, ESSInvestigationSpot Spot) const
{
	const FSSCompanionLineRow* Row = LineTable
		? LineTable->FindRow<FSSCompanionLineRow>(FName(*RowName), TEXT("FindLine"), false)
		: nullptr;
	if (!Row)
	{
		// 빠진 대사는 행 이름이 그대로 보이게 (플레이 중에 바로 눈에 띔)
		UE_LOG(LogTemp, Warning, TEXT("[Companion] Missing line row: %s"), *RowName);
		return FText::FromString(FString::Printf(TEXT("(%s)"), *RowName));
	}

	FFormatNamedArguments Args;
	Args.Add(TEXT("Spot"), FSSInvestigation::GetSpotName(Spot));
	return FText::Format(Row->Line, Args);
}

FText USSCompanionState::GetReportLine(const FSSInvestigationReport& Report) const
{
	if (Report.bSpotExhausted)
	{
		return GetLine(Report.SurvivorId, ESSCompanionLine::Exhausted, Report.Spot);
	}

	// 예: TestResearcher_Report_TerminalLog_Found
	const FString RowName = FString::Printf(TEXT("%s_Report_%s_%s"),
		*Report.SurvivorId.ToString(),
		*StaticEnum<ESSInvestigationSpot>()->GetNameStringByValue(int64(Report.Spot)),
		Report.bFoundClue ? TEXT("Found") : TEXT("None"));
	return FindLine(RowName, Report.Spot);
}

FText USSCompanionState::GetLine(FName SurvivorId, ESSCompanionLine LineType, ESSInvestigationSpot Spot) const
{
	// 예: Technician_Greet
	const FString RowName = FString::Printf(TEXT("%s_%s"),
		*SurvivorId.ToString(),
		*StaticEnum<ESSCompanionLine>()->GetNameStringByValue(int64(LineType)));
	return FindLine(RowName, Spot);
}

void USSCompanionState::ResetRun()
{
	Records.Reset();
	HeardClues.Reset();
}
