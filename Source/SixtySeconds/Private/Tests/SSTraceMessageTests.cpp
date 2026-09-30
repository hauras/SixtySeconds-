#include "Trace/SSTraceMessage.h"
#include "Trace/SSTraceSession.h"
#include "Trace/SSTraceConfig.h"
#include "Item/SSRunSubsystem.h"
#include "Comms/SSCommsState.h"
#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace SSTraceMessageTest
{
	FSSTraceMessageRow MakeRow(ESSTraceMessageKind Kind, int32 Order, int32 MinDay)
	{
		FSSTraceMessageRow Row;
		Row.Kind = Kind;
		Row.Order = Order;
		Row.MinDay = MinDay;
		Row.Weight = 10;
		Row.Title = FText::FromString(TEXT("Title"));
		Row.Text = FText::FromString(TEXT("Text"));
		return Row;
	}

	// 실용 정보 2개(하나는 3일차부터) + 진실 2개(순서 2, 1로 일부러 뒤섞음)
	UDataTable* MakeTable()
	{
		UDataTable* Table = NewObject<UDataTable>();
		Table->RowStruct = FSSTraceMessageRow::StaticStruct();

		FSSTraceMessageRow Hurt = MakeRow(ESSTraceMessageKind::Info, 0, 1);
		Hurt.EffectType = ESSEventEffect::PlayerHealth;
		Hurt.EffectAmount = -5;
		Table->AddRow(TEXT("Info_Hurt"), Hurt);

		Table->AddRow(TEXT("Info_Late"), MakeRow(ESSTraceMessageKind::Info, 0, 3));
		Table->AddRow(TEXT("Truth_B"), MakeRow(ESSTraceMessageKind::Truth, 2, 1));
		Table->AddRow(TEXT("Truth_A"), MakeRow(ESSTraceMessageKind::Truth, 1, 1));
		return Table;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSTraceMessagePickTest, "SS.Trace.MessagePick",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSTraceMessagePickTest::RunTest(const FString& Parameters)
{
	using namespace SSTraceMessageTest;
	UDataTable* Table = MakeTable();
	FRandomStream Random(7);
	TSet<FName> Received;

	// 표가 없거나 행 구조가 다르면 없음
	TestTrue(TEXT("No table, no message"), FSSTraceMessagePicker::Pick(nullptr, 1, Received, 1, 2, Random).IsNone());
	UDataTable* Wrong = NewObject<UDataTable>();
	Wrong->RowStruct = FSSEventRow::StaticStruct();
	TestTrue(TEXT("Wrong row struct, no message"), FSSTraceMessagePicker::Pick(Wrong, 1, Received, 1, 2, Random).IsNone());

	// 1번째(진실 차례 아님): 1일차엔 Info_Late가 아직 안 나오니 실용 정보는 Info_Hurt뿐
	TestEqual(TEXT("First message is info"), FSSTraceMessagePicker::Pick(Table, 1, Received, 1, 2, Random), FName(TEXT("Info_Hurt")));

	// 2번째(진실 차례): Order가 가장 작은 진실
	TestEqual(TEXT("Second message is the first truth"), FSSTraceMessagePicker::Pick(Table, 1, Received, 2, 2, Random), FName(TEXT("Truth_A")));

	// 받은 건 다시 안 나옴 → 다음 진실
	Received.Add(TEXT("Truth_A"));
	TestEqual(TEXT("Truths come in order"), FSSTraceMessagePicker::Pick(Table, 1, Received, 4, 2, Random), FName(TEXT("Truth_B")));

	// 실용 정보가 다 떨어지면 진실 차례가 아니어도 남은 진실
	Received.Add(TEXT("Info_Hurt"));
	TestEqual(TEXT("Out of info falls back to truth"), FSSTraceMessagePicker::Pick(Table, 1, Received, 3, 2, Random), FName(TEXT("Truth_B")));

	// 날짜가 되면 늦게 풀리는 정보도 나옴
	TestEqual(TEXT("MinDay unlocks info"), FSSTraceMessagePicker::Pick(Table, 3, Received, 3, 2, Random), FName(TEXT("Info_Late")));

	// 전부 받으면 없음
	Received.Add(TEXT("Truth_B"));
	Received.Add(TEXT("Info_Late"));
	TestTrue(TEXT("Everything received, no message"), FSSTraceMessagePicker::Pick(Table, 5, Received, 6, 2, Random).IsNone());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSTraceMessageDecodeTest, "SS.Trace.MessageDecode",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSTraceMessageDecodeTest::RunTest(const FString& Parameters)
{
	using namespace SSTraceMessageTest;
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	USSRunSubsystem* Run = NewObject<USSRunSubsystem>(GameInstance);
	Run->InitializeShelterStats(100.f, 100.f, 100.f);
	USSCommsState* Comms = Run->GetComms();
	Comms->SetMessageSeed(1);

	USSTraceConfig* Config = NewObject<USSTraceConfig>();
	Config->SensorPositions = { FVector2D(60.0, 50.0), FVector2D(550.0, 55.0), FVector2D(300.0, 360.0) };
	Config->ShelterPosition = FVector2D(360.0, 190.0);
	Config->MessageTable = MakeTable();
	Config->TruthEvery = 2;
	Config->InfoValidDays = 3;

	// 한 번만 더 받으면 되는 판 → 직접 송신 한 번으로 완료
	const auto CompleteOnce = [&]()
	{
		USSTraceSession* Session = NewObject<USSTraceSession>();
		Session->Initialize(Config, {}, {}, Config->ReceiveGoal - 1);
		Session->SetSeed(1234);
		Session->SendDirect();
		Comms->FinishTrace(*Session);
		return Session->GetOutcome();
	};

	// 1번째 완료: 실용 정보가 대기함에 들어감. 아직 효과 없음
	TestEqual(TEXT("First session completes"), CompleteOnce(), ESSTraceOutcome::Completed);
	TestEqual(TEXT("One message waiting"), Comms->GetPendingMessages().Num(), 1);
	if (Comms->GetPendingMessages().Num() == 1)
	{
		const FSSPendingMessage& Info = Comms->GetPendingMessages()[0];
		TestEqual(TEXT("Queued message is the info"), Info.MessageId, FName(TEXT("Info_Hurt")));
		TestEqual(TEXT("Info expires after InfoValidDays (day 1 + 3 - 1)"), Info.ExpireDay, 3);
	}
	TestTrue(TEXT("No effect before decoding"), FMath::IsNearlyEqual(Run->GetHealth(), 100.f));
	TestTrue(TEXT("Nothing revealed yet"), Comms->GetLastMessage().EventId.IsNone());
	const TArray<FSSJournalEntry>& Journal = Run->GetJournalEntries();
	TestTrue(TEXT("Journal records the encrypted message"), !Journal.IsEmpty() && Journal.Last().Event == ESSJournalEvent::Signal);

	// 2번째 완료: 진실 차례 → 첫 진실이 대기함에, 기한 없음
	TestEqual(TEXT("Second session completes"), CompleteOnce(), ESSTraceOutcome::Completed);
	TestEqual(TEXT("Two messages waiting"), Comms->GetPendingMessages().Num(), 2);
	if (Comms->GetPendingMessages().Num() == 2)
	{
		TestEqual(TEXT("Second is the first truth"), Comms->GetPendingMessages()[1].MessageId, FName(TEXT("Truth_A")));
		TestEqual(TEXT("Truth never expires"), Comms->GetPendingMessages()[1].ExpireDay, 0);
	}
	TestEqual(TEXT("Truth not counted before decoding"), Comms->GetTruthCluesFound(), 0);

	// 잘못된 번호는 아무 일 없음
	TestFalse(TEXT("Invalid index fails"), Comms->DecodeMessage(5));
	TestEqual(TEXT("Queue untouched"), Comms->GetPendingMessages().Num(), 2);

	// 실용 정보 해독: 공개 + 효과(체력 -5) + 대기함에서 빠짐
	TestTrue(TEXT("Decode info"), Comms->DecodeMessage(0));
	const FSSEventResult& First = Comms->GetLastMessage();
	TestEqual(TEXT("Decoded message is the info"), First.EventId, FName(TEXT("Info_Hurt")));
	TestEqual(TEXT("Message text revealed"), First.Lines.Num(), 1);
	TestTrue(TEXT("Effect applied to health"), FMath::IsNearlyEqual(Run->GetHealth(), 95.f));
	TestTrue(TEXT("Actual change recorded"), First.Changes.Num() == 1 && First.Changes[0].Amount == -5);
	TestEqual(TEXT("Decoded message leaves the queue"), Comms->GetPendingMessages().Num(), 1);

	// 진실 해독: 단서 수 +1
	TestTrue(TEXT("Decode truth"), Comms->DecodeMessage(0));
	TestEqual(TEXT("Truth decoded"), Comms->GetLastMessage().EventId, FName(TEXT("Truth_A")));
	TestEqual(TEXT("Truth clue counted on decode"), Comms->GetTruthCluesFound(), 1);
	TestEqual(TEXT("Queue empty"), Comms->GetPendingMessages().Num(), 0);

	// 기한: 1일에 받은 실용 정보(유효 3일)는 3일까지 남고 4일 아침에 사라짐
	// 실용 정보 하나만 있는 표로 바꿔서 확인 (대기함은 위에서 다 풀어 비어 있음)
	UDataTable* InfoOnly = NewObject<UDataTable>();
	InfoOnly->RowStruct = FSSTraceMessageRow::StaticStruct();
	InfoOnly->AddRow(TEXT("Info_Expire"), MakeRow(ESSTraceMessageKind::Info, 0, 1));
	Config->MessageTable = InfoOnly;
	Config->TruthEvery = 0;
	CompleteOnce();
	TestEqual(TEXT("Expiring info queued"), Comms->GetPendingMessages().Num(), 1);

	TestTrue(TEXT("Day 2"), Run->AdvanceDay(false, false));
	TestTrue(TEXT("Day 3"), Run->AdvanceDay(false, false));
	TestEqual(TEXT("Info still there on its last day"), Comms->GetPendingMessages().Num(), 1);
	TestTrue(TEXT("Day 4"), Run->AdvanceDay(false, false));
	TestEqual(TEXT("Info expired on day 4"), Comms->GetPendingMessages().Num(), 0);
	// 하루 넘김 때 다른 기록도 남으므로, 4일차의 통신 기록이 있는지로 확인
	TestTrue(TEXT("Expiry is recorded"), Run->GetJournalEntries().ContainsByPredicate([](const FSSJournalEntry& Entry)
	{
		return Entry.Day == 4 && Entry.Event == ESSJournalEvent::Signal;
	}));

	// 중간에 끊으면 대기함에 안 들어감
	USSTraceSession* Quit = NewObject<USSTraceSession>();
	Quit->Initialize(Config, {}, {}, 0);
	Quit->EndSession();
	Comms->FinishTrace(*Quit);
	TestEqual(TEXT("Stopping queues nothing"), Comms->GetPendingMessages().Num(), 0);

	// 새 게임이면 초기화
	CompleteOnce();
	Run->ResetRun();
	TestEqual(TEXT("Reset clears truth clues"), Comms->GetTruthCluesFound(), 0);
	TestEqual(TEXT("Reset clears the queue"), Comms->GetPendingMessages().Num(), 0);
	TestTrue(TEXT("Reset clears last message"), Comms->GetLastMessage().EventId.IsNone());

	return true;
}
#endif
