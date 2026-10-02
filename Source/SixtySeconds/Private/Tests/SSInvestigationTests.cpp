#include "Companion/SSInvestigation.h"
#include "Character/SSSurvivorDefinition.h"
#include "Item/SSRunSubsystem.h"
#include "Companion/SSCompanionState.h"
#include "Companion/SSCompanionLines.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace SSInvestigationTest
{
	USSSurvivorDefinition* MakeSurvivor(UObject* Outer, FName Id, float Thoroughness, float Boldness)
	{
		USSSurvivorDefinition* Survivor = NewObject<USSSurvivorDefinition>(Outer);
		Survivor->SurvivorId = Id;
		Survivor->DisplayName = FText::FromName(Id);
		Survivor->Thoroughness = Thoroughness;
		Survivor->Boldness = Boldness;
		return Survivor;
	}

	// 단서 표 한 줄 추가
	void AddClue(UDataTable* Table, FName ClueId, ESSInvestigationSpot Spot, int32 Stage)
	{
		FSSClueRow Row;
		Row.Spot = Spot;
		Row.Stage = Stage;
		Row.Title = FText::FromName(ClueId);
		Row.Text = FText::FromName(ClueId);
		Table->AddRow(ClueId, Row);
	}

	// 빈 단서 표
	UDataTable* MakeClueTable(UObject* Outer)
	{
		UDataTable* Table = NewObject<UDataTable>(Outer);
		Table->RowStruct = FSSClueRow::StaticStruct();
		return Table;
	}

	// 대사 표 한 줄 추가
	void AddLine(UDataTable* Table, const TCHAR* RowName, const TCHAR* Line)
	{
		FSSCompanionLineRow Row;
		Row.Line = FText::FromString(Line);
		Table->AddRow(FName(RowName), Row);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSInvestigationChanceTest, "SS.Companion.InvestigationChance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSInvestigationChanceTest::RunTest(const FString& Parameters)
{
	// 공식 그대로: 단말 로그(기본 0.4, 위험 0.3)
	//   서하린(꼼꼼 0.8, 대담 0.6): 0.4 × 1.3 × 1.18 = 0.6136 / 0.3 × 1.1 = 0.33
	//   강태오(꼼꼼 0.5, 대담 0.3): 0.4 × 1.0 × 1.09 = 0.436  / 0.3 × 0.8 = 0.24
	TestTrue(TEXT("Researcher find chance"), FMath::IsNearlyEqual(FSSInvestigation::FindChance(ESSInvestigationSpot::TerminalLog, 0.8f, 0.6f), 0.6136f, 1e-4f));
	TestTrue(TEXT("Researcher detect chance"), FMath::IsNearlyEqual(FSSInvestigation::DetectChance(ESSInvestigationSpot::TerminalLog, 0.6f), 0.33f, 1e-4f));
	TestTrue(TEXT("Technician find chance"), FMath::IsNearlyEqual(FSSInvestigation::FindChance(ESSInvestigationSpot::TerminalLog, 0.5f, 0.3f), 0.436f, 1e-4f));
	TestTrue(TEXT("Technician detect chance"), FMath::IsNearlyEqual(FSSInvestigation::DetectChance(ESSInvestigationSpot::TerminalLog, 0.3f), 0.24f, 1e-4f));

	// 성향 방향: 꼼꼼할수록 잘 찾고, 대담할수록 잘 찾지만 더 들킴
	for (int32 Index = 0; Index < int32(ESSInvestigationSpot::Count); ++Index)
	{
		const ESSInvestigationSpot Spot = ESSInvestigationSpot(Index);
		TestTrue(TEXT("Thorough finds more"), FSSInvestigation::FindChance(Spot, 0.9f, 0.5f) > FSSInvestigation::FindChance(Spot, 0.2f, 0.5f));
		TestTrue(TEXT("Bold finds more"), FSSInvestigation::FindChance(Spot, 0.5f, 0.9f) > FSSInvestigation::FindChance(Spot, 0.5f, 0.1f));
		TestTrue(TEXT("Bold gets caught more"), FSSInvestigation::DetectChance(Spot, 0.9f) > FSSInvestigation::DetectChance(Spot, 0.1f));

		const float Find = FSSInvestigation::FindChance(Spot, 1.f, 1.f);
		const float Detect = FSSInvestigation::DetectChance(Spot, 1.f);
		TestTrue(TEXT("Chances stay in 0~1"), Find >= 0.f && Find <= 1.f && Detect >= 0.f && Detect <= 1.f);
		TestFalse(TEXT("Every spot has a name"), FSSInvestigation::GetSpotName(Spot).IsEmpty());
	}

	// 단말 로그가 가장 위험함
	TestTrue(TEXT("Terminal log is the riskiest"),
		FSSInvestigation::GetSpotData(ESSInvestigationSpot::TerminalLog).Risk > FSSInvestigation::GetSpotData(ESSInvestigationSpot::Storage).Risk);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSInvestigationPickTest, "SS.Companion.InvestigationPick",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSInvestigationPickTest::RunTest(const FString& Parameters)
{
	using namespace SSInvestigationTest;
	UObject* Outer = GetTransientPackage();
	constexpr int32 SpotCount = int32(ESSInvestigationSpot::Count);

	// ── 무게 (계산만, 난수 없음) ──

	// 환풍구를 아주 좋아하는 동료: 환풍구 무게가 다른 모든 장소보다 큼
	USSSurvivorDefinition* Fan = MakeSurvivor(Outer, TEXT("Fan"), 0.5f, 0.5f);
	Fan->SpotPreference.Add(ESSInvestigationSpot::Vent, 5.f);
	const float VentWeight = FSSInvestigation::GetSpotWeight(*Fan, ESSInvestigationSpot::Vent);
	bool bVentHeaviest = true;
	for (int32 Index = 0; Index < SpotCount; ++Index)
	{
		if (ESSInvestigationSpot(Index) == ESSInvestigationSpot::Vent) continue;
		if (FSSInvestigation::GetSpotWeight(*Fan, ESSInvestigationSpot(Index)) >= VentWeight) bVentHeaviest = false;
	}
	TestTrue(TEXT("Preferred spot has the largest weight"), bVentHeaviest);

	// 아무리 겁 많아도 모든 장소의 무게가 0보다 큼 (가끔은 뽑힘)
	USSSurvivorDefinition* Timid = MakeSurvivor(Outer, TEXT("Timid"), 0.5f, 0.f);
	bool bAllPositive = true;
	for (int32 Index = 0; Index < SpotCount; ++Index)
	{
		if (FSSInvestigation::GetSpotWeight(*Timid, ESSInvestigationSpot(Index)) <= 0.f) bAllPositive = false;
	}
	TestTrue(TEXT("Every spot can be picked"), bAllPositive);

	// 단말 로그를 똑같이 좋아해도 대담한 동료의 무게가 더 큼
	//   대담(1):  (0.5 + 1) × 1           = 1.5
	//   겁쟁이(0): (0.5 + 1) × max(0.1, 1 − 0.3 × 3) = 0.15
	USSSurvivorDefinition* Bold = MakeSurvivor(Outer, TEXT("Bold"), 0.5f, 1.f);
	Bold->SpotPreference.Add(ESSInvestigationSpot::TerminalLog, 1.f);
	USSSurvivorDefinition* TimidFan = MakeSurvivor(Outer, TEXT("TimidFan"), 0.5f, 0.f);
	TimidFan->SpotPreference.Add(ESSInvestigationSpot::TerminalLog, 1.f);
	TestTrue(TEXT("Boldness keeps the risky spot heavy"),
		FSSInvestigation::GetSpotWeight(*Bold, ESSInvestigationSpot::TerminalLog)
		> FSSInvestigation::GetSpotWeight(*TimidFan, ESSInvestigationSpot::TerminalLog));

	// ── 추첨 (1000밤) ──
	//   Fan 무게: 환풍구 약 4.26 / 나머지 0.28~0.48 → 환풍구 약 73%, 나머지 각 5~8%
	FRandomStream Random(42);
	int32 FanCounts[SpotCount] = {};
	for (int32 Night = 0; Night < 1000; ++Night)
	{
		++FanCounts[int32(FSSInvestigation::PickSpot(*Fan, Random))];
	}
	bool bVentMost = true;
	int32 VisitedSpots = 0;
	for (int32 Index = 0; Index < SpotCount; ++Index)
	{
		if (FanCounts[Index] > 0) ++VisitedSpots;
		if (Index != int32(ESSInvestigationSpot::Vent) && FanCounts[Index] >= FanCounts[int32(ESSInvestigationSpot::Vent)]) bVentMost = false;
	}
	TestTrue(TEXT("Preferred spot is picked most often"), bVentMost);
	// 예전 방식(점수 + 흔들림)은 여기서 환풍구만 1000번 골랐음
	TestTrue(TEXT("Other spots are visited too"), VisitedSpots >= 3);

	// 선호 없는 겁쟁이: 위험한 단말 로그(약 3%)보다 안전한 저장고(약 31%)를 훨씬 자주 고름
	int32 TimidCounts[SpotCount] = {};
	for (int32 Night = 0; Night < 1000; ++Night)
	{
		++TimidCounts[int32(FSSInvestigation::PickSpot(*Timid, Random))];
	}
	TestTrue(TEXT("Timid survivor prefers safe spots"),
		TimidCounts[int32(ESSInvestigationSpot::Storage)] > TimidCounts[int32(ESSInvestigationSpot::TerminalLog)]);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSInvestigationNightTest, "SS.Companion.InvestigationNight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSInvestigationNightTest::RunTest(const FString& Parameters)
{
	using namespace SSInvestigationTest;
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	USSRunSubsystem* Run = NewObject<USSRunSubsystem>(GameInstance);
	Run->InitializeShelterStats(100.f, 100.f, 100.f);
	USSCompanionState* Companions = Run->GetCompanions();
	Companions->SetInvestigationSeed(7);

	USSSurvivorDefinition* Researcher = MakeSurvivor(Run, TEXT("Researcher"), 0.8f, 0.6f);
	Researcher->SpotPreference.Add(ESSInvestigationSpot::TerminalLog, 3.f);
	Run->RecruitSurvivor(Researcher);
	Run->RescueFollowingSurvivors();

	const auto CountInvestigationRecords = [Run]()
	{
		int32 Count = 0;
		for (const FSSJournalEntry& Entry : Run->GetJournalEntries())
		{
			if (Entry.Event == ESSJournalEvent::Investigation) ++Count;
		}
		return Count;
	};

	FSSInvestigationReport Report;

	// 첫날 아침: 아직 조사 전이라 들을 보고 없음
	TestFalse(TEXT("No report before the first night"), Companions->HearInvestigationReport(TEXT("Researcher"), Report));

	// 1일째 밤을 넘기면 보고 1개. 기록에는 아직 없음 (들어야 남음)
	TestTrue(TEXT("Night 1"), Run->AdvanceDay(false, false));
	const FSSCompanionRecord* State = Companions->FindRecord(TEXT("Researcher"));
	if (!TestNotNull(TEXT("Companion record"), State)) return false;
	if (!TestEqual(TEXT("One report waiting"), State->PendingReports.Num(), 1)) return false;
	TestEqual(TEXT("Report remembers the night"), State->PendingReports[0].Day, 1);
	TestEqual(TEXT("Report remembers who"), State->PendingReports[0].SurvivorId, FName(TEXT("Researcher")));
	// 자동 장소는 추첨이라 어디든 될 수 있음 (선호는 InvestigationPick에서 확인)
	TestTrue(TEXT("Auto spot is a real spot"), State->PendingReports[0].Spot < ESSInvestigationSpot::Count);
	const ESSInvestigationSpot AutoSpot = State->PendingReports[0].Spot;
	TestEqual(TEXT("Nothing recorded before hearing"), CountInvestigationRecords(), 0);

	// 대화로 들으면 기록 1줄, "!" 꺼짐. 단서가 있었으면 들은 단서 목록에 들어감
	TestTrue(TEXT("Hear the report"), Companions->HearInvestigationReport(TEXT("Researcher"), Report));
	TestEqual(TEXT("Heard the spot investigated last night"), Report.Spot, AutoSpot);
	TestEqual(TEXT("Heard on day 2"), Report.HeardDay, 2);
	TestEqual(TEXT("One record after hearing"), CountInvestigationRecords(), 1);
	TestFalse(TEXT("Report cleared"), State->HasPendingReport());
	TestEqual(TEXT("Heard clue saved only if found"), Companions->GetHeardClues().Num(), Report.bFoundClue ? 1 : 0);
	const int32 HeardCluesAfterFirst = Companions->GetHeardClues().Num();

	// 다시 대화해도 중복 지급 없음
	TestFalse(TEXT("No second hearing"), Companions->HearInvestigationReport(TEXT("Researcher"), Report));
	TestEqual(TEXT("Still one record"), CountInvestigationRecords(), 1);

	// 지시: 행동력 1, 하루 한 번. 다음 밤엔 지시한 장소를 조사하고 지시는 사라짐
	const int32 ApBefore = Run->GetActionPoints();
	TestTrue(TEXT("Order storage"), Companions->OrderInvestigation(TEXT("Researcher"), ESSInvestigationSpot::Storage));
	TestEqual(TEXT("Order costs 1 AP"), Run->GetActionPoints(), ApBefore - USSCompanionState::InvestigationOrderCost);
	TestFalse(TEXT("Only one order per day"), Companions->OrderInvestigation(TEXT("Researcher"), ESSInvestigationSpot::Vent));
	TestEqual(TEXT("Rejected order costs nothing"), Run->GetActionPoints(), ApBefore - USSCompanionState::InvestigationOrderCost);
	TestFalse(TEXT("Unknown survivor cannot be ordered"), Companions->OrderInvestigation(TEXT("Nobody"), ESSInvestigationSpot::Vent));

	TestTrue(TEXT("Night 2"), Run->AdvanceDay(false, false));
	if (!TestEqual(TEXT("Night 2 report waiting"), State->PendingReports.Num(), 1)) return false;
	TestEqual(TEXT("Ordered spot investigated"), State->PendingReports[0].Spot, ESSInvestigationSpot::Storage);
	TestFalse(TEXT("Order used up"), State->bHasOrder);
	const bool bNight2Clue = State->PendingReports[0].bFoundClue;

	// 듣지 않고 3일째 밤: 단서 있는 보고는 남고, 단서 없는 보고는 새 보고로 대체됨
	TestTrue(TEXT("Night 3"), Run->AdvanceDay(false, false));
	TestEqual(TEXT("Clue report kept, empty report replaced"), State->PendingReports.Num(), bNight2Clue ? 2 : 1);
	TestEqual(TEXT("Newest report is last"), State->PendingReports.Last().Day, 3);
	TestTrue(TEXT("Back to an auto spot"), State->PendingReports.Last().Spot < ESSInvestigationSpot::Count);
	TestEqual(TEXT("Unheard report never recorded"), CountInvestigationRecords(), 1);

	// 밀린 보고는 오래된 것부터 들음
	TestTrue(TEXT("Hear the oldest"), Companions->HearInvestigationReport(TEXT("Researcher"), Report));
	TestEqual(TEXT("Oldest first"), Report.Day, bNight2Clue ? 2 : 3);
	TestEqual(TEXT("Two records after hearing again"), CountInvestigationRecords(), 2);
	TestEqual(TEXT("Heard clues grow only with clue reports"),
		Companions->GetHeardClues().Num(), HeardCluesAfterFirst + (Report.bFoundClue ? 1 : 0));

	// 알고 있는 단서·들킨 횟수는 밤 수 이하
	TestTrue(TEXT("Known clues within nights"), State->CluesFound >= 0 && State->CluesFound <= 3);
	TestTrue(TEXT("Detections within nights"), State->TimesDetected >= 0 && State->TimesDetected <= 3);

	// 새 게임이면 기록 전부 지움
	Run->ResetRun();
	TestNull(TEXT("Reset clears companion records"), Companions->FindRecord(TEXT("Researcher")));
	TestEqual(TEXT("Reset clears heard clues"), Companions->GetHeardClues().Num(), 0);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSCluePickTest, "SS.Companion.CluePick",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSCluePickTest::RunTest(const FString& Parameters)
{
	using namespace SSInvestigationTest;
	UDataTable* Table = MakeClueTable(GetTransientPackage());

	// 표에 넣는 순서가 섞여 있어도 Stage 순서대로 나와야 함
	AddClue(Table, TEXT("T3"), ESSInvestigationSpot::TerminalLog, 3);
	AddClue(Table, TEXT("T1"), ESSInvestigationSpot::TerminalLog, 1);
	AddClue(Table, TEXT("V1"), ESSInvestigationSpot::Vent, 1);
	AddClue(Table, TEXT("T2"), ESSInvestigationSpot::TerminalLog, 2);

	const auto Next = [Table](ESSInvestigationSpot Spot, const TSet<FName>& Known)
	{
		return FSSInvestigation::PickNextClue(*Table, Spot, Known);
	};
	const ESSInvestigationSpot Terminal = ESSInvestigationSpot::TerminalLog;

	TestEqual(TEXT("First stage first"), Next(Terminal, {}), FName(TEXT("T1")));
	TestEqual(TEXT("Then the next stage"), Next(Terminal, { TEXT("T1") }), FName(TEXT("T2")));
	TestEqual(TEXT("Lowest unknown stage"), Next(Terminal, { TEXT("T2") }), FName(TEXT("T1")));
	TestEqual(TEXT("Last stage"), Next(Terminal, { TEXT("T1"), TEXT("T2") }), FName(TEXT("T3")));
	TestTrue(TEXT("Nothing left"), Next(Terminal, { TEXT("T1"), TEXT("T2"), TEXT("T3") }).IsNone());
	TestEqual(TEXT("Other spots ignore terminal clues"), Next(ESSInvestigationSpot::Vent, { TEXT("T1") }), FName(TEXT("V1")));
	TestTrue(TEXT("Spot without clues"), Next(ESSInvestigationSpot::Door, {}).IsNone());

	TestEqual(TEXT("Terminal clue count"), FSSInvestigation::CountClues(*Table, Terminal), 3);
	TestEqual(TEXT("Door clue count"), FSSInvestigation::CountClues(*Table, ESSInvestigationSpot::Door), 0);

	// 다 찾은 장소는 추첨 무게가 줄어서, 좋아하는 장소여도 덜 감
	//   환풍구 무게 4.26 → 0.64: 약 73% → 약 29%
	USSSurvivorDefinition* Fan = MakeSurvivor(GetTransientPackage(), TEXT("Fan"), 0.5f, 0.5f);
	Fan->SpotPreference.Add(ESSInvestigationSpot::Vent, 5.f);
	const TSet<ESSInvestigationSpot> Exhausted = { ESSInvestigationSpot::Vent };
	FRandomStream Random(11);
	int32 VentNormal = 0;
	int32 VentExhausted = 0;
	for (int32 Night = 0; Night < 1000; ++Night)
	{
		if (FSSInvestigation::PickSpot(*Fan, Random) == ESSInvestigationSpot::Vent) ++VentNormal;
		if (FSSInvestigation::PickSpot(*Fan, Random, Exhausted) == ESSInvestigationSpot::Vent) ++VentExhausted;
	}
	TestTrue(TEXT("Exhausted spot is picked less"), VentExhausted < VentNormal);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSClueProgressionTest, "SS.Companion.ClueProgression",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSClueProgressionTest::RunTest(const FString& Parameters)
{
	using namespace SSInvestigationTest;
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	USSRunSubsystem* Run = NewObject<USSRunSubsystem>(GameInstance);
	Run->InitializeShelterStats(100.f, 100.f, 100.f);
	USSCompanionState* Companions = Run->GetCompanions();
	Companions->SetInvestigationSeed(3);

	// 동료 두 명 (같은 밤에 같은 장소를 뒤져도 단서가 겹치면 안 됨)
	USSSurvivorDefinition* First = MakeSurvivor(Run, TEXT("First"), 1.f, 1.f);
	USSSurvivorDefinition* Second = MakeSurvivor(Run, TEXT("Second"), 1.f, 1.f);
	Run->RecruitSurvivor(First);
	Run->RecruitSurvivor(Second);
	Run->RescueFollowingSurvivors();

	// 장소마다 단서 2개 = 10개
	UDataTable* Clues = MakeClueTable(Run);
	for (int32 Index = 0; Index < int32(ESSInvestigationSpot::Count); ++Index)
	{
		const ESSInvestigationSpot Spot = ESSInvestigationSpot(Index);
		AddClue(Clues, *FString::Printf(TEXT("C%d_1"), Index), Spot, 1);
		AddClue(Clues, *FString::Printf(TEXT("C%d_2"), Index), Spot, 2);
	}
	Companions->SetTables(Clues, nullptr);

	const TArray<FName> Ids = { TEXT("First"), TEXT("Second") };

	// 들은 단서 + 기다리는 단서를 모두 모아서 규칙 확인. 아는 단서 개수를 돌려줌
	const auto CheckClues = [this, Companions, &Ids](const TCHAR* When)
	{
		TSet<FName> Seen;
		bool bUnique = true;
		bool bMatchesSpot = true;
		bool bExhaustedHasNoClue = true;
		const auto Visit = [&](const FSSInvestigationReport& Report)
		{
			if (Report.bSpotExhausted && (Report.bFoundClue || !Report.ClueId.IsNone())) bExhaustedHasNoClue = false;
			if (!Report.bFoundClue) return;
			const FSSClueRow* Clue = Companions->FindClue(Report.ClueId);
			if (!Clue || Clue->Spot != Report.Spot) bMatchesSpot = false;
			if (Seen.Contains(Report.ClueId)) bUnique = false;
			Seen.Add(Report.ClueId);
		};
		for (const FSSInvestigationReport& Heard : Companions->GetHeardClues()) Visit(Heard);
		for (const FName& Id : Ids)
		{
			if (const FSSCompanionRecord* Record = Companions->FindRecord(Id))
			{
				for (const FSSInvestigationReport& Pending : Record->PendingReports) Visit(Pending);
			}
		}

		// 2단계를 알면 같은 장소 1단계도 알고 있어야 함 (순서대로 나옴)
		bool bInOrder = true;
		for (int32 Index = 0; Index < int32(ESSInvestigationSpot::Count); ++Index)
		{
			const FName Stage1(*FString::Printf(TEXT("C%d_1"), Index));
			const FName Stage2(*FString::Printf(TEXT("C%d_2"), Index));
			if (Seen.Contains(Stage2) && !Seen.Contains(Stage1)) bInOrder = false;
		}

		TestTrue(FString::Printf(TEXT("%s: no duplicate clue"), When), bUnique);
		TestTrue(FString::Printf(TEXT("%s: clue belongs to the spot"), When), bMatchesSpot);
		TestTrue(FString::Printf(TEXT("%s: stages in order"), When), bInOrder);
		TestTrue(FString::Printf(TEXT("%s: exhausted report carries no clue"), When), bExhaustedHasNoClue);
		return Seen.Num();
	};

	// 1) 보고를 안 듣고 밤만 넘김: 기다리는 보고끼리도 단서가 겹치지 않음
	//    (밤 조사만 직접 돌려서 배고픔·날짜는 안 바뀜)
	for (int32 Night = 0; Night < 15; ++Night)
	{
		Companions->RunNight();
	}
	CheckClues(TEXT("Unheard"));

	// 2) 매일 다 들으면서 계속: 들은 단서와도 겹치지 않음
	FSSInvestigationReport Report;
	for (int32 Night = 0; Night < 60; ++Night)
	{
		for (const FName& Id : Ids)
		{
			while (Companions->HearInvestigationReport(Id, Report)) {}
		}
		Companions->RunNight();
	}
	const int32 Known = CheckClues(TEXT("Heard"));
	TestTrue(TEXT("Never more clues than the table"), Known <= 10);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSCompanionLinesTest, "SS.Companion.Lines",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSCompanionLinesTest::RunTest(const FString& Parameters)
{
	using namespace SSInvestigationTest;
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	USSRunSubsystem* Run = NewObject<USSRunSubsystem>(GameInstance);
	USSCompanionState* Companions = Run->GetCompanions();

	UDataTable* Lines = NewObject<UDataTable>(Run);
	Lines->RowStruct = FSSCompanionLineRow::StaticStruct();
	AddLine(Lines, TEXT("Tester_Greet"), TEXT("Hello"));
	AddLine(Lines, TEXT("Tester_OrderAccept"), TEXT("{Spot} OK"));
	AddLine(Lines, TEXT("Tester_Report_Door_Found"), TEXT("Door clue"));
	AddLine(Lines, TEXT("Tester_Report_Door_None"), TEXT("Door nothing"));

	// 다른 형식의 표를 단서 표 자리에 넣으면 거부 (경고 1번)
	AddExpectedError(TEXT("Clue table row struct"), EAutomationExpectedErrorFlags::Contains, 1);
	Companions->SetTables(Lines, Lines);
	TestEqual(TEXT("Wrong table is not used as clue table"), Companions->CountClues(ESSInvestigationSpot::Door), 0);

	// 공통 대사, {Spot} 채우기
	TestEqual(TEXT("Greet line"),
		Companions->GetLine(TEXT("Tester"), ESSCompanionLine::Greet).ToString(),
		FString(TEXT("Hello")));
	TestEqual(TEXT("Spot filled"),
		Companions->GetLine(TEXT("Tester"), ESSCompanionLine::OrderAccept, ESSInvestigationSpot::Door).ToString(),
		FSSInvestigation::GetSpotName(ESSInvestigationSpot::Door).ToString() + TEXT(" OK"));

	// 보고 대사: 장소·찾음 여부로 줄 이름을 만듦
	FSSInvestigationReport Report;
	Report.SurvivorId = TEXT("Tester");
	Report.Spot = ESSInvestigationSpot::Door;
	Report.bFoundClue = true;
	TestEqual(TEXT("Found report line"), Companions->GetReportLine(Report).ToString(), FString(TEXT("Door clue")));
	Report.bFoundClue = false;
	TestEqual(TEXT("Empty report line"), Companions->GetReportLine(Report).ToString(), FString(TEXT("Door nothing")));

	// 빠진 대사는 행 이름이 괄호로 보임 (다 찾은 장소 대사가 표에 없음, 경고 1번)
	AddExpectedError(TEXT("Missing line row"), EAutomationExpectedErrorFlags::Contains, 1);
	Report.bSpotExhausted = true;
	TestEqual(TEXT("Missing line shows row name"),
		Companions->GetReportLine(Report).ToString(),
		FString(TEXT("(Tester_Exhausted)")));
	return true;
}

#endif
