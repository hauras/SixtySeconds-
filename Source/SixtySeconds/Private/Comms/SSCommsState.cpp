#include "Comms/SSCommsState.h"
#include "Item/SSRunSubsystem.h"
#include "Event/SSEventDirector.h"
#include "Trace/SSTraceSession.h"
#include "Trace/SSTraceConfig.h"
#include "Trace/SSTraceMessage.h"

USSRunSubsystem& USSCommsState::GetRun() const
{
	return *CastChecked<USSRunSubsystem>(GetOuter());
}

void USSCommsState::EnqueueMessage(const USSTraceConfig* Config)
{
	USSRunSubsystem& Run = GetRun();

	// 다음 메시지 고르기 (표가 없거나 다 받았으면 내용 없이 완료만 기록)
	const UDataTable* Table = IsValid(Config) ? Config->MessageTable.Get() : nullptr;
	const FName MessageId = FSSTraceMessagePicker::Pick(
		Table,
		Run.GetCurrentDay(),
		ReceivedMessages,
		CompletedMessages,
		IsValid(Config) ? Config->TruthEvery : 0,
		MessageRandom);

	const FSSTraceMessageRow* Row = MessageId.IsNone() ? nullptr : Table->FindRow<FSSTraceMessageRow>(MessageId, TEXT("EnqueueMessage"));
	if (!Row)
	{
		Run.AddJournal(ESSJournalEvent::Signal,
			NSLOCTEXT("SSTrace", "Completed", "외부 통신: 메시지 하나를 끝까지 받았다."));
		return;
	}

	ReceivedMessages.Add(MessageId);
	FSSPendingMessage& Pending = PendingMessages.AddDefaulted_GetRef();
	Pending.MessageId = MessageId;
	Pending.Row = *Row;

	// 실용 정보는 받은 날 포함 InfoValidDays일째까지, 진실 단서는 기한 없음(0)
	if (Row->Kind == ESSTraceMessageKind::Info)
	{
		Pending.ExpireDay = Run.GetCurrentDay() + Config->InfoValidDays - 1;
	}
	else
	{
		Pending.ExpireDay = 0;
	}

	// 해독 열쇠: 다이얼마다 정답 값을 무작위로, 플레이어 다이얼은 0에서 시작
	for (int32 i = 0; i < Config->DialCount; ++i)
	{
		Pending.Key.Add(MessageRandom.RandRange(1, 25));
		Pending.Dials.Add(0);
	}
	// 기록: 받았지만 아직 내용은 모름
	Run.AddJournal(ESSJournalEvent::Signal,
		NSLOCTEXT("SSTrace", "MessageQueued", "외부 통신: 암호화된 메시지를 받았다. 해독이 필요하다."));
}

bool USSCommsState::CanStartTrace() const
{
	const USSRunSubsystem& Run = GetRun();
	return Run.GetHealth() > 0.f
		&& LastTraceDay < Run.GetCurrentDay()
		&& Run.GetActionPoints() >= TraceActionCost;
}

bool USSCommsState::BeginTrace()
{
	if (!CanStartTrace()) return false;
	if (!GetRun().SpendActionPoints(TraceActionCost)) return false;
	LastTraceDay = GetRun().GetCurrentDay();
	return true;
}

TArray<int32> USSCommsState::GetBlockedTraceRelays() const
{
	const int32 Today = GetRun().GetCurrentDay();

	TArray<int32> Blocked;
	for (const TPair<int32, int32>& Pair : TraceRelayBlockedUntil)
	{
		if (Pair.Value > Today) Blocked.Add(Pair.Key);   // 풀리는 날이 아직 안 옴
	}
	return Blocked;
}

void USSCommsState::FinishTrace(const USSTraceSession& Session)
{
	USSRunSubsystem& Run = GetRun();

	// 이번 판에 적이 확인한 단자는 며칠 동안 차단
	const int32 BlockDays = IsValid(Session.GetConfig()) ? Session.GetConfig()->RelayBlockDays : 1;
	for (const int32 Relay : Session.GetNewlyBlockedRelays())
	{
		TraceRelayBlockedUntil.Add(Relay, Run.GetCurrentDay() + BlockDays);
	}

	switch (Session.GetOutcome())
	{
	case ESSTraceOutcome::Exposed:
		// 적이 위치를 확정하고 확인하러 옴 → 적의 기억은 비워짐, 받던 메시지는 끊김
		TraceSpots.Reset();
		TraceReceived = 0;
		Run.AddJournal(ESSJournalEvent::Signal,
			NSLOCTEXT("SSTrace", "Exposed", "외부 통신 중 은신처 위치가 노출됐다. 통신이 끊겼다."));
		break;

	case ESSTraceOutcome::Completed:
		// 메시지 하나를 다 받음 → 다음 메시지는 처음부터
		TraceSpots = Session.GetSpots();
		TraceReceived = 0;
		++CompletedMessages;
		EnqueueMessage(Session.GetConfig());
		break;

	default:
		// 도중에 멈춤 → 받은 만큼은 내일 이어서
		TraceSpots = Session.GetSpots();
		TraceReceived = Session.GetReceived();
		Run.AddJournal(ESSJournalEvent::Signal, FText::Format(
			NSLOCTEXT("SSTrace", "Stopped", "외부 통신: 신호를 {0}번 주고받고 접속을 끊었다."), Session.GetTurn()));
		break;
	}
}

bool USSCommsState::DecodeMessage(int32 PendingIndex)
{
	// 잘못된 번호면 이전 결과를 건드리지 않고 끝
	if (!PendingMessages.IsValidIndex(PendingIndex)) return false;

	USSRunSubsystem& Run = GetRun();
	LastMessage = FSSEventResult();

	// 대기함에서 꺼냄 
	const FSSPendingMessage Pending = PendingMessages[PendingIndex];
	const FSSTraceMessageRow& Row = Pending.Row;

	if (Row.Kind == ESSTraceMessageKind::Truth) ++TruthCluesFound;

	// 결과: 제목, 원문
	LastMessage.EventId = Pending.MessageId;
	LastMessage.Title = Row.Title;
	LastMessage.Lines.Add(Row.Text);

	// 효과는 사건 효과를 그대로 적용 
	USSEventDirector* Director = Run.GetEventDirector();
	if (Row.EffectType != ESSEventEffect::None)
	{
		FSSEventEffectRow Effect;
		Effect.Type = Row.EffectType;
		Effect.Target = Row.EffectTarget;
		Effect.Amount = Row.EffectAmount;
		Director->ApplyStandaloneEffect(Effect, Run, LastMessage);
	}

	// 저널: "외부 통신 해독 — 제목: 원문 (체력 +5)"
	const FText Changes = Director->DescribeChanges(LastMessage);
	const FText Body = Changes.IsEmpty()
		? FText::Format(NSLOCTEXT("SSTrace", "MessageJournal", "외부 통신 해독 — {0}: {1}"), Row.Title, Row.Text)
		: FText::Format(NSLOCTEXT("SSTrace", "MessageJournalChanges", "외부 통신 해독 — {0}: {1} ({2})"), Row.Title, Row.Text, Changes);
	Run.AddJournal(ESSJournalEvent::Signal, Body);

	if (!LastMessage.Changes.IsEmpty()) Run.OnPlayerStatsChanged.Broadcast();

	// 풀었으니 대기함에서 뺌
	PendingMessages.RemoveAt(PendingIndex);
	return true;
}

void USSCommsState::SaveDials(int32 PendingIndex, const TArray<int32>& Dials)
{
	// 잘못된 번호면 아무것도 안 함
	if (!PendingMessages.IsValidIndex(PendingIndex)) return;

	// 그 메시지의 다이얼 위치를 새 값으로 덮어씀
	PendingMessages[PendingIndex].Dials = Dials;
}

void USSCommsState::OnNewDay()
{
	// 하룻밤 지나면 적의 기억이 흐려짐
	FSSSuspectMemory::DecayDaily(TraceSpots);

	// 기한 지난 메시지는 사라짐 (RunSubsystem이 날짜를 올린 뒤 부르므로 새 날짜)
	const int32 Today = GetRun().GetCurrentDay();

	// 뒤에서부터 돌아야 지워도 번호가 안 꼬임
	for (int32 i = PendingMessages.Num() - 1; i >= 0; --i)
	{
		const FSSPendingMessage& Pending = PendingMessages[i];

		// 기한 없음(0)이거나 아직 풀 수 있는 날이면 남김
		if (Pending.ExpireDay == 0 || Today <= Pending.ExpireDay) continue;

		// 기록 먼저 (지우고 나면 Pending이 가리키던 칸이 사라짐)
		GetRun().AddJournal(ESSJournalEvent::Signal, FText::Format(
			NSLOCTEXT("SSTrace", "MessageExpired", "외부 통신: 해독하지 못한 메시지({0})가 쓸모없어졌다."),
			Pending.Row.Title));

		PendingMessages.RemoveAt(i);
	}
}

void USSCommsState::ResetRun()
{
	TraceSpots.Reset();
	TraceReceived = 0;
	TraceRelayBlockedUntil.Reset();
	LastTraceDay = 0;
	CompletedMessages = 0;
	ReceivedMessages.Reset();
	TruthCluesFound = 0;
	LastMessage = FSSEventResult();
	PendingMessages.Reset();
}
