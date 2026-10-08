#include "Decode/SSDialSession.h"
#include "Comms/SSCommsState.h"
#include "Trace/SSTraceSession.h"
#include "Trace/SSTraceConfig.h"
#include "Trace/SSTraceMessage.h"
#include "Item/SSRunSubsystem.h"
#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace SSDialSessionTest
{
	const TCHAR* const Plain = TEXT("PATROL UNIT SHIFT CHANGE AT SIX HUNDRED HOURS. CORRIDOR B ONE IS UNWATCHED FOR TWELVE MINUTES.");

	// 실용 정보 하나(체력 -5)를 대기함에 넣은 RunSubsystem
	USSRunSubsystem* MakeRunWithMessage(ESSTraceMessageKind Kind = ESSTraceMessageKind::Info)
	{
		UGameInstance* GameInstance = NewObject<UGameInstance>();
		USSRunSubsystem* Run = NewObject<USSRunSubsystem>(GameInstance);
		Run->InitializeShelterStats(100.f, 100.f, 100.f);
		Run->GetComms()->SetMessageSeed(3);

		UDataTable* Table = NewObject<UDataTable>();
		Table->RowStruct = FSSTraceMessageRow::StaticStruct();
		FSSTraceMessageRow Row;
		Row.Kind = Kind;
		Row.Title = FText::FromString(TEXT("Patrol"));
		Row.Text = FText::FromString(TEXT("Patrol"));
		Row.CipherSource = Plain;
		Row.EffectType = ESSEventEffect::PlayerHealth;
		Row.EffectAmount = -5;
		Table->AddRow(TEXT("Info_Patrol"), Row);

		USSTraceConfig* Config = NewObject<USSTraceConfig>();
		Config->SensorPositions = {FVector2D(60.0, 50.0), FVector2D(550.0, 55.0), FVector2D(300.0, 360.0)};
		Config->ShelterPosition = FVector2D(360.0, 190.0);
		Config->MessageTable = Table;
		Config->TruthEvery = Kind == ESSTraceMessageKind::Truth ? 1 : 0;
		Config->DialCount = 3;

		// 한 번만 더 받으면 되는 판 → 직접 송신 한 번으로 완료 → 대기함에 들어감
		USSTraceSession* Session = NewObject<USSTraceSession>();
		Session->Initialize(Config, {}, {}, Config->ReceiveGoal - 1);
		Session->SetSeed(1234);
		Session->SendDirect();
		Run->GetComms()->FinishTrace(*Session);
		return Run;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSDialSessionTest, "SS.Decode.DialSession",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSDialSessionTest::RunTest(const FString& Parameters)
{
	using namespace SSDialSessionTest;
	USSRunSubsystem* Run = MakeRunWithMessage();
	USSCommsState* Comms = Run->GetComms();

	if (!TestEqual(TEXT("One message waiting"), Comms->GetPendingMessages().Num(), 1)) return false;
	const TArray<int32> Key = Comms->GetPendingMessages()[0].Key;
	TestEqual(TEXT("Key has DialCount values"), Key.Num(), 3);
	for (const int32 K : Key)
	{
		TestTrue(TEXT("Key values are 1~25 (never 0)"), K >= 1 && K <= 25);
	}

	// 잘못된 번호는 시작 못 함
	TestFalse(TEXT("Invalid index"), NewObject<USSDialSession>()->Initialize(Run, 5));
	TestFalse(TEXT("No run"), NewObject<USSDialSession>()->Initialize(nullptr, 0));

	USSDialSession* Session = NewObject<USSDialSession>();
	if (!TestTrue(TEXT("Initialize"), Session->Initialize(Run, 0))) return false;
	TestEqual(TEXT("Dial count"), Session->GetDialCount(), 3);

	// 열쇠가 0이 아니라서 처음엔 모든 글자가 바뀌어 있음
	TestNotEqual(TEXT("Starts scrambled"), Session->GetShownText(), FString(Plain));

	// 왼쪽으로 돌면 0 → 25, 다시 오른쪽으로 0
	Session->TurnDial(0, -1);
	TestEqual(TEXT("Wraps 0 -> 25"), Session->GetDial(0), 25);
	Session->TurnDial(0, +1);
	TestEqual(TEXT("Back to 0"), Session->GetDial(0), 0);

	// 정답 위치에서 신호 세기 최대(1) — 정답에서 카이제곱이 가장 작음
	for (int32 Step = 0; Step < Key[0]; ++Step) Session->TurnDial(0, +1);
	TestEqual(TEXT("Dial 0 at key"), Session->GetDial(0), Key[0]);
	TestTrue(TEXT("Strength is 1 at the key"), FMath::IsNearlyEqual(Session->GetDialStrength(0), 1.f));
	TestFalse(TEXT("Not unlocked with one dial"), Session->IsUnlocked());

	// 다이얼 위치는 대기함에 저장 → 새 세션이 이어받음
	USSDialSession* Reopened = NewObject<USSDialSession>();
	Reopened->Initialize(Run, 0);
	TestEqual(TEXT("Dial position saved"), Reopened->GetDial(0), Key[0]);

	// 힌트: 행동력 1, 틀린 첫 다이얼(1번)을 정답 바로 옆으로
	const int32 ApBefore = Run->GetActionPoints();
	TestTrue(TEXT("Hint works"), Session->UseHint());
	TestEqual(TEXT("Hint costs 1 AP"), Run->GetActionPoints(), ApBefore - 1);
	const int32 Gap = FMath::Abs(Session->GetDial(1) - Key[1]);
	TestTrue(TEXT("Hint lands next to the key"), Gap == 1 || Gap == 25);
	TestFalse(TEXT("Hint never solves the dial"), Session->GetDial(1) == Key[1]);

	// 나머지 다이얼을 정답으로 → 잠금 해제
	while (Session->GetDial(1) != Key[1]) Session->TurnDial(1, +1);
	TestFalse(TEXT("Still locked before the last dial"), Session->IsUnlocked());
	while (Session->GetDial(2) != Key[2]) Session->TurnDial(2, +1);

	TestTrue(TEXT("Unlocked"), Session->IsUnlocked());
	TestEqual(TEXT("Shows the original"), Session->GetShownText(), FString(Plain));
	TestTrue(TEXT("Effect applied on unlock"), FMath::IsNearlyEqual(Run->GetHealth(), 95.f));
	TestEqual(TEXT("Message left the queue"), Comms->GetPendingMessages().Num(), 0);

	// 풀린 뒤엔 돌려도·힌트도 아무 일 없음
	Session->TurnDial(0, +1);
	TestEqual(TEXT("Locked dials after unlock"), Session->GetDial(0), Key[0]);
	TestFalse(TEXT("No hint after unlock"), Session->UseHint());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSDialSessionTimeTest, "SS.Decode.DialTimeLimit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSDialSessionTimeTest::RunTest(const FString& Parameters)
{
	using namespace SSDialSessionTest;

	// 실용 정보: 시간제한 있음, 다 쓰면 메시지가 사라짐
	{
		USSRunSubsystem* Run = MakeRunWithMessage(ESSTraceMessageKind::Info);
		USSDialSession* Session = NewObject<USSDialSession>();
		if (!TestTrue(TEXT("Info initialize"), Session->Initialize(Run, 0))) return false;

		TestTrue(TEXT("Info has a time limit"), Session->HasTimeLimit());
		TestTrue(TEXT("Starts with the full time"), FMath::IsNearlyEqual(Session->GetTimeLeft(), USSDialSession::InfoTimeLimit));

		Session->Tick(USSDialSession::InfoTimeLimit - 1.f);
		TestFalse(TEXT("Not failed before time runs out"), Session->IsFailed());
		TestEqual(TEXT("Message still waiting"), Run->GetComms()->GetPendingMessages().Num(), 1);

		Session->Tick(2.f);
		TestTrue(TEXT("Fails when time runs out"), Session->IsFailed());
		TestTrue(TEXT("Time left is 0"), FMath::IsNearlyZero(Session->GetTimeLeft()));
		TestEqual(TEXT("Message lost"), Run->GetComms()->GetPendingMessages().Num(), 0);
		TestTrue(TEXT("No effect from a lost message"), FMath::IsNearlyEqual(Run->GetHealth(), 100.f));
		TestTrue(TEXT("Loss is recorded"), !Run->GetJournalEntries().IsEmpty() && Run->GetJournalEntries().Last().Event == ESSJournalEvent::Signal);

		// 실패한 뒤엔 돌려도·힌트도 아무 일 없음
		const int32 DialBefore = Session->GetDial(0);
		Session->TurnDial(0, +1);
		TestEqual(TEXT("Dials frozen after failure"), Session->GetDial(0), DialBefore);
		TestFalse(TEXT("No hint after failure"), Session->UseHint());
	}

	// 실용 정보 포기: 창을 닫으면 사라짐
	{
		USSRunSubsystem* Run = MakeRunWithMessage(ESSTraceMessageKind::Info);
		USSDialSession* Session = NewObject<USSDialSession>();
		Session->Initialize(Run, 0);
		Session->GiveUp();
		TestTrue(TEXT("Giving up fails the info"), Session->IsFailed());
		TestEqual(TEXT("Given-up message lost"), Run->GetComms()->GetPendingMessages().Num(), 0);
	}

	// 진실 단서: 시간제한 없음, 시간이 흘러도·포기해도 남음
	{
		USSRunSubsystem* Run = MakeRunWithMessage(ESSTraceMessageKind::Truth);
		if (!TestEqual(TEXT("Truth waiting"), Run->GetComms()->GetPendingMessages().Num(), 1)) return false;

		USSDialSession* Session = NewObject<USSDialSession>();
		Session->Initialize(Run, 0);
		TestFalse(TEXT("Truth has no time limit"), Session->HasTimeLimit());

		Session->Tick(1000.f);
		Session->GiveUp();
		TestFalse(TEXT("Truth never fails"), Session->IsFailed());
		TestEqual(TEXT("Truth stays in the queue"), Run->GetComms()->GetPendingMessages().Num(), 1);
	}
	return true;
}
#endif
