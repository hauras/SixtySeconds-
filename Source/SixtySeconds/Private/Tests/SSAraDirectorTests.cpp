#include "Ara/SSAraDirector.h"
#include "Character/SSSurvivorDefinition.h"
#include "Item/SSRunSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/DataTable.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace SSAraDirectorTest
{
	// 구조된 동료 둘(연구원 영향력 1.2, 정비사 1.0)이 있는 판
	USSRunSubsystem* MakeRun()
	{
		UGameInstance* GameInstance = NewObject<UGameInstance>();
		USSRunSubsystem* Run = NewObject<USSRunSubsystem>(GameInstance);
		Run->InitializeShelterStats(100.f, 100.f, 100.f);

		const auto AddSurvivor = [Run](const TCHAR* Id, float Influence)
		{
			USSSurvivorDefinition* Survivor = NewObject<USSSurvivorDefinition>(Run);
			Survivor->SurvivorId = Id;
			Survivor->DisplayName = FText::FromString(Id);
			Survivor->AraInfluence = Influence;
			Run->RecruitSurvivor(Survivor);
		};
		AddSurvivor(TEXT("Researcher"), 1.2f);
		AddSurvivor(TEXT("Technician"), 1.f);
		Run->RescueFollowingSurvivors();
		return Run;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSAraSuspicionMathTest, "SS.Ara.SuspicionMath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSAraSuspicionMathTest::RunTest(const FString& Parameters)
{
	// 확률 ↔ 로그 오즈 왕복
	TestTrue(TEXT("Round trip"), FMath::IsNearlyEqual(USSAraDirector::ToProbability(USSAraDirector::ToLogOdds(0.1f)), 0.1f, 1e-4f));
	TestTrue(TEXT("Even odds"), FMath::IsNearlyEqual(USSAraDirector::ToProbability(0.f), 0.5f, 1e-6f));

	// 핑계가 없는 장소일수록 들켰을 때 의심이 큼. 어디서 들켜도 의심은 오름(비율 > 1)
	const float Terminal = USSAraDirector::DetectionRatio(ESSInvestigationSpot::TerminalLog);
	const float Door = USSAraDirector::DetectionRatio(ESSInvestigationSpot::Door);
	const float Storage = USSAraDirector::DetectionRatio(ESSInvestigationSpot::Storage);
	TestTrue(TEXT("Terminal is the worst place to be caught"), Terminal > Door && Door > Storage);
	TestTrue(TEXT("Any detection raises suspicion"), Storage > 1.f);
	TestTrue(TEXT("Terminal ratio"), FMath::IsNearlyEqual(Terminal, 10.f, 1e-4f));

	// 엿듣기 확률: 기본 25%, 학습도 1마다 +5%, 최대 60%
	TestTrue(TEXT("Base eavesdrop"), FMath::IsNearlyEqual(USSAraDirector::EavesdropChance(0), 0.25f, 1e-6f));
	TestTrue(TEXT("Learning raises eavesdrop"), FMath::IsNearlyEqual(USSAraDirector::EavesdropChance(2), 0.35f, 1e-6f));
	TestTrue(TEXT("Eavesdrop capped"), FMath::IsNearlyEqual(USSAraDirector::EavesdropChance(100), 0.6f, 1e-6f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSAraSuspicionUpdateTest, "SS.Ara.SuspicionUpdate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSAraSuspicionUpdateTest::RunTest(const FString& Parameters)
{
	using namespace SSAraDirectorTest;
	USSRunSubsystem* Run = MakeRun();
	USSAraDirector* Ara = Run->GetAra();
	const FName Researcher = TEXT("Researcher");
	const FName Technician = TEXT("Technician");

	// 처음엔 누구든 10%
	TestTrue(TEXT("Prior suspicion"), FMath::IsNearlyEqual(Ara->GetSuspicion(Researcher), 0.1f, 1e-4f));

	// 단말 로그에서 들킴: 오즈 1/9 × 10 = 10/9 → 확률 10/19 ≈ 0.526
	Ara->ObserveNight(Researcher, ESSInvestigationSpot::TerminalLog, true);
	TestTrue(TEXT("Bayes update after terminal detection"), FMath::IsNearlyEqual(Ara->GetSuspicion(Researcher), 10.f / 19.f, 1e-3f));

	// 저장고에서 들킨 사람은 덜 의심받음
	Ara->ObserveNight(Technician, ESSInvestigationSpot::Storage, true);
	const float TechAfterStorage = Ara->GetSuspicion(Technician);
	TestTrue(TEXT("Storage detection raises a little"), TechAfterStorage > 0.1f && TechAfterStorage < Ara->GetSuspicion(Researcher));

	// 조용한 밤은 조금 덜 의심
	Ara->ObserveNight(Technician, ESSInvestigationSpot::Storage, false);
	TestTrue(TEXT("Quiet night lowers suspicion"), Ara->GetSuspicion(Technician) < TechAfterStorage);

	// 위협도 = 의심 × 영향력
	TestTrue(TEXT("Threat uses influence"),
		FMath::IsNearlyEqual(Ara->GetThreat(Researcher), Ara->GetSuspicion(Researcher) * 1.2f, 1e-4f));

	// 하룻밤 지나면 처음 믿음 쪽으로: 높은 의심은 내려가되 처음보다는 높음
	const float Before = Ara->GetSuspicion(Researcher);
	Ara->BeginNight();
	const float After = Ara->GetSuspicion(Researcher);
	TestTrue(TEXT("Forget pulls high suspicion down"), After < Before && After > 0.1f);

	// 처음보다 낮은 믿음은 올라감 (조용한 밤만 여러 번)
	for (int32 Night = 0; Night < 10; ++Night) Ara->ObserveNight(Technician, ESSInvestigationSpot::Storage, false);
	const float Low = Ara->GetSuspicion(Technician);
	Ara->BeginNight();
	TestTrue(TEXT("Forget pulls low suspicion up"), Low < 0.1f && Ara->GetSuspicion(Technician) > Low);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSAraTargetTest, "SS.Ara.Target",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSAraTargetTest::RunTest(const FString& Parameters)
{
	using namespace SSAraDirectorTest;
	USSRunSubsystem* Run = MakeRun();
	USSAraDirector* Ara = Run->GetAra();
	const FName Researcher = TEXT("Researcher");

	// 아무 일도 없으면 표적 없음, 브리핑 덧붙임 없음
	Ara->UpdateTarget();
	TestFalse(TEXT("No target at start"), Ara->HasTarget());
	TestTrue(TEXT("No notes at start"), Ara->BuildBriefingNotes().IsEmpty());

	// 단말에서 한 번 들킴: 위협 0.526 × 1.2 = 0.63 < 0.7 → 아직 표적 아님, 브리핑에는 언급
	Ara->ObserveNight(Researcher, ESSInvestigationSpot::TerminalLog, true);
	Ara->UpdateTarget();
	TestFalse(TEXT("One detection is not enough"), Ara->HasTarget());
	TestFalse(TEXT("Briefing mentions the night activity"), Ara->BuildBriefingNotes().IsEmpty());

	// 단서 보고를 엿들음: 오즈 10/9 × 8 → 확률 0.899, 위협 1.08 → 표적
	Ara->ObserveEavesdrop(Researcher, true);
	Ara->UpdateTarget();
	TestEqual(TEXT("Researcher becomes the target"), Ara->GetTarget(), Researcher);

	// 며칠 조용해도 풀림 기준(0.4)보다 높으면 표적 유지 (매일 바뀌지 않게)
	for (int32 Night = 0; Night < 3; ++Night)
	{
		Ara->BeginNight();
		Ara->UpdateTarget();
	}
	TestTrue(TEXT("Threat dropped below target line"), Ara->GetThreat(Researcher) < 1.08f);
	TestEqual(TEXT("Target kept above release line"), Ara->GetTarget(), Researcher);

	// 오래 조용하면 잊고 풀림
	for (int32 Night = 0; Night < 20; ++Night)
	{
		Ara->BeginNight();
		Ara->UpdateTarget();
	}
	TestFalse(TEXT("Target released after long quiet"), Ara->HasTarget());

	// 새 게임이면 전부 지움
	Ara->ObserveEavesdrop(Researcher, true);
	Ara->ResetRun();
	TestTrue(TEXT("Reset clears suspicion"), FMath::IsNearlyEqual(Ara->GetSuspicion(Researcher), 0.1f, 1e-4f));
	TestFalse(TEXT("Reset clears target"), Ara->HasTarget());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSAraBriefingLinesTest, "SS.Ara.BriefingLines",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSAraBriefingLinesTest::RunTest(const FString& Parameters)
{
	using namespace SSAraDirectorTest;
	USSRunSubsystem* Run = MakeRun();
	USSAraDirector* Ara = Run->GetAra();

	// 표 없이: 코드 안의 기본 문장
	TestTrue(TEXT("Fallback day one"), Ara->BuildBriefing().ToString().StartsWith(TEXT("Day 1.")));

	// 다른 형식의 표는 거부
	UDataTable* Wrong = NewObject<UDataTable>(Run);
	Wrong->RowStruct = FTableRowBase::StaticStruct();
	AddExpectedError(TEXT("Line table row struct"), EAutomationExpectedErrorFlags::Contains, 1);
	Ara->SetLineTable(Wrong);
	TestFalse(TEXT("Wrong table rejected"), Ara->HasLineTable());

	// 물자가 없으니 '부족' 줄. 두 줄이 날짜마다 번갈아 나옴, {Day} 채움
	UDataTable* Lines = NewObject<UDataTable>(Run);
	Lines->RowStruct = FSSAraLineRow::StaticStruct();
	const auto AddLine = [Lines](const TCHAR* RowName, const TCHAR* Text)
	{
		FSSAraLineRow Row;
		Row.Line = FText::FromString(Text);
		Lines->AddRow(FName(RowName), Row);
	};
	AddLine(TEXT("LowSupply_1"), TEXT("A {Day}"));
	AddLine(TEXT("LowSupply_2"), TEXT("B {Day}"));
	Ara->SetLineTable(Lines);
	TestTrue(TEXT("Table connected"), Ara->HasLineTable());

	TestTrue(TEXT("Day 2"), Run->AdvanceDay(false, false));
	const FString Day2 = Ara->BuildBriefing().ToString();
	TestEqual(TEXT("Same day gives the same line"), Ara->BuildBriefing().ToString(), Day2);
	TestTrue(TEXT("Day 3"), Run->AdvanceDay(false, false));
	const FString Day3 = Ara->BuildBriefing().ToString();

	// 첫 줄만 확인 (밤사이 동료가 들켰으면 아래에 의심 문장이 붙을 수 있음)
	TestTrue(TEXT("Day 2 filled"), Day2.StartsWith(TEXT("A 2")) || Day2.StartsWith(TEXT("B 2")));
	TestTrue(TEXT("Day 3 filled"), Day3.StartsWith(TEXT("A 3")) || Day3.StartsWith(TEXT("B 3")));
	TestNotEqual(TEXT("Different line on the next day"), Day2.Left(1), Day3.Left(1));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSAraEavesdropTest, "SS.Ara.Eavesdrop",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSAraEavesdropTest::RunTest(const FString& Parameters)
{
	using namespace SSAraDirectorTest;
	USSRunSubsystem* Run = MakeRun();
	USSAraDirector* Ara = Run->GetAra();
	Ara->SetSeed(5);

	// 학습도 0 → 25%. 1000번 중 엿들은 횟수가 그 근처
	int32 Heard = 0;
	for (int32 Try = 0; Try < 1000; ++Try)
	{
		if (Ara->TryEavesdrop(TEXT("Technician"), false)) ++Heard;
	}
	TestTrue(TEXT("Eavesdrop rate near 25%"), Heard > 200 && Heard < 300);

	// 엿들은 만큼만 의심이 오름 (못 들은 대화는 반영 안 됨)
	USSAraDirector* Fresh = MakeRun()->GetAra();
	for (int32 Each = 0; Each < Heard; ++Each) Fresh->ObserveEavesdrop(TEXT("Technician"), false);
	TestTrue(TEXT("Only heard talks count"),
		FMath::IsNearlyEqual(Ara->GetSuspicion(TEXT("Technician")), Fresh->GetSuspicion(TEXT("Technician")), 1e-4f));
	return true;
}
#endif
