#include "Ara/SSAraDirector.h"
#include "Character/SSSurvivorDefinition.h"
#include "Companion/SSCompanionState.h"
#include "Event/SSEventDirector.h"
#include "Item/SSRunSubsystem.h"
#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace SSAraSwapTest
{
	// 구조된 동료 둘(연구원 영향력 1.2, 정비사 1.0)이 있는 판. 꼼꼼함·대담함은 최대 (조사 결과가 잘 나오게)
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
			Survivor->Thoroughness = 1.f;
			Survivor->Boldness = 1.f;
			Run->RecruitSurvivor(Survivor);
		};
		AddSurvivor(TEXT("Researcher"), 1.2f);
		AddSurvivor(TEXT("Technician"), 1.f);
		Run->RescueFollowingSurvivors();
		return Run;
	}

	// 연구원을 표적으로 만듦 (단말에서 들킴 + 단서 보고를 엿들음 → 위협 1.08)
	void MakeResearcherTarget(USSAraDirector* Ara)
	{
		Ara->ObserveNight(TEXT("Researcher"), ESSInvestigationSpot::TerminalLog, true);
		Ara->ObserveEavesdrop(TEXT("Researcher"), true);
		Ara->UpdateTarget();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSAraSwapEscalationTest, "SS.Ara.SwapEscalation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSAraSwapEscalationTest::RunTest(const FString& Parameters)
{
	using namespace SSAraSwapTest;
	USSRunSubsystem* Run = MakeRun();
	USSAraDirector* Ara = Run->GetAra();
	USSEventDirector* Events = Run->GetEventDirector();
	const FName Researcher = TEXT("Researcher");
	const FName Offer(USSAraDirector::OfferEventId);
	const FName Repeat(USSAraDirector::RepeatOfferEventId);
	const FName Seize(USSAraDirector::SeizeEventId);
	const int32 Today = Run->GetCurrentDay();

	// 표적이 없으면 교체할 수 없고, 예약도 없음
	TestFalse(TEXT("No target, no swap"), Ara->CanSwapTarget());
	TestEqual(TEXT("Nothing scheduled"), Events->FindScheduledDay(Offer), -1);

	// 새 표적 → 이틀 뒤 밤에 제안 (내일 아침 브리핑이 먼저 경고)
	MakeResearcherTarget(Ara);
	if (!TestEqual(TEXT("Researcher is the target"), Ara->GetTarget(), Researcher)) return false;
	TestTrue(TEXT("Target can be swapped"), Ara->CanSwapTarget());
	TestEqual(TEXT("Offer two nights later"), Events->FindScheduledDay(Offer), Today + USSAraDirector::OfferDelay);

	// 같은 표적이 유지되면 다시 예약하지 않음
	Ara->UpdateTarget();
	Events->CancelScheduled(Offer);
	Ara->UpdateTarget();
	TestEqual(TEXT("Kept target is not rescheduled"), Events->FindScheduledDay(Offer), -1);

	// 거절 1번 → 사흘 뒤 재제안, 2번 → 다음 밤 강제, 막아내면 이틀 뒤 다시 강제
	Ara->OnOfferRefused();
	TestEqual(TEXT("One refusal"), Ara->GetRefusalCount(), 1);
	TestEqual(TEXT("Repeat offer three nights later"), Events->FindScheduledDay(Repeat), Today + USSAraDirector::RepeatOfferDelay);
	Ara->OnOfferRefused();
	TestEqual(TEXT("Seize next night"), Events->FindScheduledDay(Seize), Today + USSAraDirector::SeizeDelay);
	Events->CancelScheduled(Seize);
	Ara->OnOfferRefused();
	TestEqual(TEXT("Resisted seize comes back later"), Events->FindScheduledDay(Seize), Today + USSAraDirector::ResistedSeizeDelay);

	// 교체: 안드로이드가 되고, 진짜는 붙잡히고, 예약은 모두 지워지고, 의심은 최저 → 브리핑에서 사라짐
	TestTrue(TEXT("Swap succeeds"), Ara->SwapTarget());
	TestTrue(TEXT("Researcher is now an android"), Run->GetCompanions()->IsAndroid(Researcher));
	TestEqual(TEXT("Real researcher is captured"), Ara->GetCapturedRealId(), Researcher);
	TestFalse(TEXT("Target cleared"), Ara->HasTarget());
	TestEqual(TEXT("Offer cancelled"), Events->FindScheduledDay(Offer), -1);
	TestEqual(TEXT("Repeat cancelled"), Events->FindScheduledDay(Repeat), -1);
	TestEqual(TEXT("Seize cancelled"), Events->FindScheduledDay(Seize), -1);
	TestTrue(TEXT("Android is barely suspected"), Ara->GetSuspicion(Researcher) < USSAraDirector::NoticeSuspicion);
	TestTrue(TEXT("Briefing no longer mentions the swapped one"), Ara->BuildBriefingNotes().IsEmpty());

	// 한 판에 한 명: 다른 동료가 아무리 의심받아도 표적이 되지 않음
	TestFalse(TEXT("Second swap fails"), Ara->SwapTarget());
	for (int32 Each = 0; Each < 3; ++Each) Ara->ObserveEavesdrop(TEXT("Technician"), true);
	Ara->UpdateTarget();
	TestFalse(TEXT("No new target after a swap"), Ara->HasTarget());

	// 안드로이드는 엿듣지도, 관찰하지도 않음
	const float AndroidBefore = Ara->GetSuspicion(Researcher);
	TestFalse(TEXT("Android talk is not eavesdropped"), Ara->TryEavesdrop(Researcher, true));
	Ara->ObserveNight(Researcher, ESSInvestigationSpot::TerminalLog, true);
	TestTrue(TEXT("Android is not observed"), FMath::IsNearlyEqual(Ara->GetSuspicion(Researcher), AndroidBefore, 1e-6f));

	// 새 게임이면 교체 기록도 지움
	Run->ResetRun();
	TestFalse(TEXT("Reset clears swap"), Ara->IsSwapDone());
	TestFalse(TEXT("Reset clears android"), Run->GetCompanions()->IsAndroid(Researcher));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSAraReleaseCancelsOfferTest, "SS.Ara.ReleaseCancelsOffer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSAraReleaseCancelsOfferTest::RunTest(const FString& Parameters)
{
	using namespace SSAraSwapTest;
	USSRunSubsystem* Run = MakeRun();
	USSAraDirector* Ara = Run->GetAra();
	USSEventDirector* Events = Run->GetEventDirector();
	const FName Offer(USSAraDirector::OfferEventId);

	MakeResearcherTarget(Ara);
	TestTrue(TEXT("Offer scheduled"), Events->FindScheduledDay(Offer) > 0);
	Ara->OnOfferRefused();

	// 오래 조용하면 표적이 풀리고, 예약된 제안과 거절 횟수도 지워짐
	for (int32 Night = 0; Night < 30; ++Night)
	{
		Ara->BeginNight();
		Ara->UpdateTarget();
	}
	TestFalse(TEXT("Target released"), Ara->HasTarget());
	TestEqual(TEXT("Offer cancelled on release"), Events->FindScheduledDay(Offer), -1);
	TestEqual(TEXT("Repeat cancelled on release"), Events->FindScheduledDay(FName(USSAraDirector::RepeatOfferEventId)), -1);
	TestEqual(TEXT("Refusals reset on release"), Ara->GetRefusalCount(), 0);

	// 표적이 없을 때 거절은 아무 일도 안 함
	Ara->OnOfferRefused();
	TestEqual(TEXT("Refusal without target does nothing"), Ara->GetRefusalCount(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSAndroidReportTest, "SS.Ara.AndroidReports",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSAndroidReportTest::RunTest(const FString& Parameters)
{
	using namespace SSAraSwapTest;
	USSRunSubsystem* Run = MakeRun();
	USSCompanionState* Companions = Run->GetCompanions();
	Companions->SetInvestigationSeed(21);
	const FName Researcher = TEXT("Researcher");
	Companions->MakeAndroid(Researcher);

	// 매일 저장고를 지시하고 밤을 보낸 뒤 보고를 들음 (밤 조사만 직접 돌려서 배고픔은 안 바뀜)
	int32 WrongSpot = 0;
	bool bNeverClue = true;
	constexpr int32 Nights = 40;
	for (int32 Night = 0; Night < Nights; ++Night)
	{
		Run->AdjustActionPoints(USSRunSubsystem::MaxActionPoints);
		TestTrue(TEXT("Android accepts orders"), Companions->OrderInvestigation(Researcher, ESSInvestigationSpot::Storage));
		Companions->RunNight();

		FSSInvestigationReport Report;
		if (!TestTrue(TEXT("Android reports every morning"), Companions->HearInvestigationReport(Researcher, Report))) return false;
		if (Report.bFoundClue || !Report.ClueId.IsNone()) bNeverClue = false;
		if (Report.Spot != ESSInvestigationSpot::Storage) ++WrongSpot;
	}

	// 단서는 절대 없음, 들키지도 않음
	TestTrue(TEXT("Android never finds a clue"), bNeverClue);
	const FSSCompanionRecord* Record = Companions->FindRecord(Researcher);
	TestTrue(TEXT("Android is never detected"), Record && Record->TimesDetected == 0 && Record->CluesFound == 0);

	// 30% 확률로 지시와 다른 장소를 말함 (40밤이면 평균 12번): 가끔은 다르고, 늘 다르지는 않음
	TestTrue(TEXT("Sometimes reports a different spot"), WrongSpot > 0);
	TestTrue(TEXT("Usually reports the ordered spot"), WrongSpot < Nights / 2);

	// 진짜 동료는 그대로 조사함 (꼼꼼함·대담함 최대라 40밤 중 한 번은 찾음)
	const FSSCompanionRecord* Human = Companions->FindRecord(TEXT("Technician"));
	TestTrue(TEXT("Human keeps investigating"), Human && Human->CluesFound > 0);
	return true;
}
#endif
