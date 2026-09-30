#include "Trace/SSTraceSession.h"
#include "Trace/SSTraceConfig.h"
#include "Item/SSRunSubsystem.h"
#include "Comms/SSCommsState.h"
#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace SSTraceSessionTest
{
	// 목업과 같은 맵: 센서 세 대가 은신처(360, 190)를 둘러쌈
	// 통신 단자 두 곳도 센서 삼각형 안쪽, 은신처에서 100 이상 떨어진 곳
	USSTraceConfig* MakeConfig()
	{
		USSTraceConfig* Config = NewObject<USSTraceConfig>();
		Config->SensorPositions = { FVector2D(60.0, 50.0), FVector2D(550.0, 55.0), FVector2D(300.0, 360.0) };
		Config->ShelterPosition = FVector2D(360.0, 190.0);
		Config->RelayPositions = { FVector2D(250.0, 150.0), FVector2D(450.0, 120.0) };
		return Config;
	}

	USSTraceSession* MakeSession(USSTraceConfig* Config, int32 Received, const TArray<int32>& BlockedRelays = {})
	{
		USSTraceSession* Session = NewObject<USSTraceSession>();
		Session->Initialize(Config, {}, BlockedRelays, Received);
		Session->SetSeed(1234);   // 매번 같은 잡음
		return Session;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSTraceSessionRulesTest, "SS.Trace.SessionRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSTraceSessionRulesTest::RunTest(const FString& Parameters)
{
	using namespace SSTraceSessionTest;

	// 규칙이 없으면 판이 열리지 않음
	USSTraceSession* NoConfig = MakeSession(nullptr, 0);
	TestEqual(TEXT("No config stops the session"), NoConfig->GetOutcome(), ESSTraceOutcome::Stopped);
	TestFalse(TEXT("Cannot send without config"), NoConfig->SendDirect());

	// 이미 다 받았으면 바로 완료
	USSTraceConfig* Config = MakeConfig();
	TestEqual(TEXT("Already received everything"), MakeSession(Config, Config->ReceiveGoal)->GetOutcome(), ESSTraceOutcome::Completed);

	// 단자 고르기: 없는 번호 거부, 차단된 단자 거부, 다른 단자로 바꾸기 가능
	USSTraceSession* Relay = MakeSession(Config, 0, { 1 });
	TestFalse(TEXT("No relay selected at start"), Relay->HasRelay());
	TestFalse(TEXT("Cannot send via relay without one"), Relay->SendViaRelay());
	TestFalse(TEXT("Invalid relay index is rejected"), Relay->SelectRelay(5));
	TestFalse(TEXT("Blocked relay is rejected"), Relay->SelectRelay(1));
	TestFalse(TEXT("Rejected selection leaves no relay"), Relay->HasRelay());
	TestTrue(TEXT("Open relay is selected"), Relay->SelectRelay(0));
	TestTrue(TEXT("Selected relay position comes from the map"), Relay->GetRelayPosition().Equals(Config->RelayPositions[0]));

	USSTraceSession* Switch = MakeSession(Config, 0);
	Switch->SelectRelay(0);
	TestTrue(TEXT("Switching relays is allowed"), Switch->SelectRelay(1));
	TestEqual(TEXT("Switched relay"), Switch->GetSelectedRelay(), 1);

	// 한 번만 더 받으면 되는 상태 → 송신 한 번으로 완료
	USSTraceSession* AlmostDone = MakeSession(Config, Config->ReceiveGoal - 1);
	TestTrue(TEXT("Send works"), AlmostDone->SendDirect());
	TestEqual(TEXT("Reaching the goal completes"), AlmostDone->GetOutcome(), ESSTraceOutcome::Completed);
	TestFalse(TEXT("No more sending after completion"), AlmostDone->SendDirect());

	// 종료하면 받은 만큼 남기고 끝
	USSTraceSession* Quit = MakeSession(Config, 0);
	Quit->SendDirect();
	Quit->EndSession();
	TestEqual(TEXT("Ending stops the session"), Quit->GetOutcome(), ESSTraceOutcome::Stopped);
	TestEqual(TEXT("Received count kept"), Quit->GetReceived(), 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSTraceSessionExposeTest, "SS.Trace.SessionExpose",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSTraceSessionExposeTest::RunTest(const FString& Parameters)
{
	using namespace SSTraceSessionTest;
	USSTraceConfig* Config = MakeConfig();
	Config->ReceiveGoal = 99;   // 완료로 끝나지 않게 (들킴만 보려고)
	Config->MaxTurns = 10;

	// 직접 송신만 계속하면: 의심 장소는 하나(은신처)로 모이고, 결국 들킴
	USSTraceSession* Session = MakeSession(Config, 0);
	int32 Guard = 0;
	while (Session->GetOutcome() == ESSTraceOutcome::InProgress && Guard++ < 20)
	{
		Session->SendDirect();
		TestTrue(TEXT("Direct sends gather into a single spot"), Session->GetSpots().Num() <= 1);
	}
	TestEqual(TEXT("Only-direct sending ends in exposure"), Session->GetOutcome(), ESSTraceOutcome::Exposed);
	TestTrue(TEXT("Exposure takes more than one send"), Session->GetTurn() > 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSTraceRelayBlockTest, "SS.Trace.RelayBlock",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSTraceRelayBlockTest::RunTest(const FString& Parameters)
{
	using namespace SSTraceSessionTest;
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	USSRunSubsystem* Run = NewObject<USSRunSubsystem>(GameInstance);
	Run->InitializeShelterStats(100.f, 100.f, 100.f);
	USSTraceConfig* Config = MakeConfig();
	Config->ReceiveGoal = 99;   // 완료로 끝나지 않게 (차단만 보려고)
	Config->MaxTurns = 10;

	// 같은 단자로만 계속 보내면: 적이 그 단자를 확신하고 차단
	USSTraceSession* Session = MakeSession(Config, 0);
	for (int32 i = 0; i < 10 && Session->GetOutcome() == ESSTraceOutcome::InProgress && !Session->IsRelayBlocked(0); ++i)
	{
		Session->SelectRelay(0);
		Session->SendViaRelay();
	}
	TestTrue(TEXT("Repeated relay use gets the relay blocked"), Session->IsRelayBlocked(0));
	TestTrue(TEXT("Blocked relay is reported as newly blocked"), Session->GetNewlyBlockedRelays().Contains(0));
	TestFalse(TEXT("Blocking clears the selection"), Session->HasRelay());
	TestFalse(TEXT("Blocked relay cannot be selected again"), Session->SelectRelay(0));
	TestNotEqual(TEXT("Relay far from the shelter does not expose it"), Session->GetOutcome(), ESSTraceOutcome::Exposed);

	// 차단은 RelayBlockDays(2)일 동안 이어짐
	Session->EndSession();
	Run->GetComms()->FinishTrace(*Session);
	TestTrue(TEXT("Run remembers the blocked relay"), Run->GetComms()->GetBlockedTraceRelays().Contains(0));
	TestTrue(TEXT("Day 1 passes"), Run->AdvanceDay(false, false));
	TestTrue(TEXT("Still blocked the next day"), Run->GetComms()->GetBlockedTraceRelays().Contains(0));
	TestTrue(TEXT("Day 2 passes"), Run->AdvanceDay(false, false));
	TestFalse(TEXT("Unblocked after RelayBlockDays"), Run->GetComms()->GetBlockedTraceRelays().Contains(0));

	// 새 게임이면 차단도 초기화
	Run->GetComms()->FinishTrace(*Session);
	Run->ResetRun();
	TestEqual(TEXT("Reset clears blocked relays"), Run->GetComms()->GetBlockedTraceRelays().Num(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSTraceCarryOverTest, "SS.Trace.RunCarryOver",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSTraceCarryOverTest::RunTest(const FString& Parameters)
{
	using namespace SSTraceSessionTest;
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	USSRunSubsystem* Run = NewObject<USSRunSubsystem>(GameInstance);
	Run->InitializeShelterStats(100.f, 100.f, 100.f);
	USSTraceConfig* Config = MakeConfig();

	// 하루 1회, 행동력 1
	const int32 ApBefore = Run->GetActionPoints();
	TestTrue(TEXT("Can start trace on a fresh day"), Run->GetComms()->CanStartTrace());
	TestTrue(TEXT("Begin trace"), Run->GetComms()->BeginTrace());
	TestEqual(TEXT("Trace costs one action point"), Run->GetActionPoints(), ApBefore - USSCommsState::TraceActionCost);
	TestFalse(TEXT("Only once per day"), Run->GetComms()->BeginTrace());

	// 직접 두 번 받고 종료 → 결과가 RunSubsystem에 남음
	USSTraceSession* Session = NewObject<USSTraceSession>();
	Session->Initialize(Config, Run->GetComms()->GetTraceSpots(), Run->GetComms()->GetBlockedTraceRelays(), Run->GetComms()->GetTraceReceived());
	Session->SetSeed(1234);
	Session->SendDirect();
	Session->SendDirect();
	Session->EndSession();
	Run->GetComms()->FinishTrace(*Session);

	TestEqual(TEXT("Received count carried over"), Run->GetComms()->GetTraceReceived(), 2);
	TestEqual(TEXT("Direct sending blocks no relay"), Run->GetComms()->GetBlockedTraceRelays().Num(), 0);
	TestEqual(TEXT("Enemy remembers one spot"), Run->GetComms()->GetTraceSpots().Num(), 1);
	const float RadiusToday = Run->GetComms()->GetTraceSpots().Num() == 1 ? Run->GetComms()->GetTraceSpots()[0].Radius : 0.f;

	// 하룻밤 지나면 기억이 흐려짐: 믿음 절반 → 다음 판에서 다시 계산하면 반경이 커짐
	TestTrue(TEXT("Day advances"), Run->AdvanceDay(false, false));
	TestTrue(TEXT("New day allows trace again"), Run->GetComms()->CanStartTrace());
	TestTrue(TEXT("Weights halved overnight"),
		Run->GetComms()->GetTraceSpots().Num() == 1 && FMath::IsNearlyEqual(Run->GetComms()->GetTraceSpots()[0].Readings[0].Weight, 0.5f));

	USSTraceSession* Tomorrow = NewObject<USSTraceSession>();
	Tomorrow->Initialize(Config, Run->GetComms()->GetTraceSpots(), Run->GetComms()->GetBlockedTraceRelays(), Run->GetComms()->GetTraceReceived());
	TestTrue(TEXT("Faded memory is less certain (bigger radius)"),
		Tomorrow->GetSpots().Num() == 1 && Tomorrow->GetSpots()[0].Radius > RadiusToday);
	TestEqual(TEXT("Tomorrow continues from the received count"), Tomorrow->GetReceived(), 2);

	// 들키면: 적의 기억과 받던 메시지가 비워짐
	Config->ReceiveGoal = 99;
	Config->MaxTurns = 20;
	USSTraceSession* Reckless = NewObject<USSTraceSession>();
	Reckless->Initialize(Config, Run->GetComms()->GetTraceSpots(), Run->GetComms()->GetBlockedTraceRelays(), Run->GetComms()->GetTraceReceived());
	Reckless->SetSeed(99);
	for (int32 i = 0; i < 20 && Reckless->GetOutcome() == ESSTraceOutcome::InProgress; ++i)
	{
		Reckless->SendDirect();
	}
	TestEqual(TEXT("Reckless sending gets exposed"), Reckless->GetOutcome(), ESSTraceOutcome::Exposed);
	Run->GetComms()->FinishTrace(*Reckless);
	TestEqual(TEXT("Exposure wipes enemy memory"), Run->GetComms()->GetTraceSpots().Num(), 0);
	TestEqual(TEXT("Exposure cuts the message"), Run->GetComms()->GetTraceReceived(), 0);

	// 새 게임이면 전부 초기화
	Run->ResetRun();
	TestEqual(TEXT("Reset clears messages"), Run->GetComms()->GetCompletedMessages(), 0);

	return true;
}
#endif
