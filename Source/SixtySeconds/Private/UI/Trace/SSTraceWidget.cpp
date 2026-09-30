#include "UI/Trace/SSTraceWidget.h"
#include "UI/Trace/SSTraceMapWidget.h"
#include "Trace/SSTraceConfig.h"
#include "Item/SSRunSubsystem.h"
#include "Event/SSEventDirector.h"
#include "Comms/SSCommsState.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"

namespace SSTraceStyle
{
	FLinearColor Color(const TCHAR* Hex)
	{
		return FLinearColor::FromSRGBColor(FColor::FromHex(Hex));
	}

	UCanvasPanelSlot* Place(UCanvasPanel* Canvas, UWidget* Widget, const FVector2D Position,
		const FVector2D Size, int32 ZOrder)
	{
		UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(Widget);
		Slot->SetPosition(Position);
		Slot->SetSize(Size);
		Slot->SetZOrder(ZOrder);
		return Slot;
	}

	UTextBlock* Label(UWidgetTree* Tree, const FText& Value, int32 FontSize, const TCHAR* Hex,
		ETextJustify::Type Justification = ETextJustify::Left)
	{
		UTextBlock* Widget = Tree->ConstructWidget<UTextBlock>();
		Widget->SetText(Value);
		FSlateFontInfo Font = Widget->GetFont();
		Font.Size = FontSize;
		Widget->SetFont(Font);
		Widget->SetColorAndOpacity(FSlateColor(Color(Hex)));
		Widget->SetJustification(Justification);
		Widget->SetAutoWrapText(true);
		Widget->SetVisibility(ESlateVisibility::HitTestInvisible);
		return Widget;
	}

	UButton* Button(UWidgetTree* Tree, const FText& Caption, const TCHAR* Hex, int32 FontSize)
	{
		UButton* Widget = Tree->ConstructWidget<UButton>();
		FButtonStyle Style = Widget->GetStyle();
		Style.Normal.DrawAs = ESlateBrushDrawType::NoDrawType;
		Style.Hovered.DrawAs = ESlateBrushDrawType::NoDrawType;
		Style.Pressed.DrawAs = ESlateBrushDrawType::NoDrawType;
		Style.Disabled.DrawAs = ESlateBrushDrawType::NoDrawType;
		Widget->SetStyle(Style);
		Widget->SetContent(Label(Tree, Caption, FontSize, Hex, ETextJustify::Center));
		return Widget;
	}
}

TSharedRef<SWidget> USSTraceWidget::RebuildWidget()
{
	using namespace SSTraceStyle;
	if (!WidgetTree->RootWidget)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>();
		WidgetTree->RootWidget = Root;
		UScaleBox* Scale = WidgetTree->ConstructWidget<UScaleBox>();
		Scale->SetStretch(EStretch::ScaleToFit);
		UCanvasPanelSlot* ScaleSlot = Root->AddChildToCanvas(Scale);
		ScaleSlot->SetAnchors(FAnchors(0, 0, 1, 1));
		ScaleSlot->SetOffsets(FMargin(0));
		USizeBox* Design = WidgetTree->ConstructWidget<USizeBox>();
		Design->SetWidthOverride(1672.f);
		Design->SetHeightOverride(941.f);
		Scale->AddChild(Design);
		UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
		Design->AddChild(Canvas);

		UImage* Frame = WidgetTree->ConstructWidget<UImage>();
		if (UTexture2D* Texture = LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/Assets/UI/Trace/SS_TraceTerminal_Frame_v1.SS_TraceTerminal_Frame_v1")))
		{
			Frame->SetBrushFromTexture(Texture);
		}
		Frame->SetVisibility(ESlateVisibility::HitTestInvisible);
		Place(Canvas, Frame, FVector2D::ZeroVector, FVector2D(1672, 941), 0);

		MapView = WidgetTree->ConstructWidget<USSTraceMapWidget>();
		Place(Canvas, MapView, FVector2D(215, 158), FVector2D(949, 536), 2);
		Place(Canvas, Label(WidgetTree, NSLOCTEXT("SSTrace", "Title", "외부 통신"), 36, TEXT("F2E4D0")),
			FVector2D(245, 91), FVector2D(400, 55), 4);
		Place(Canvas, Label(WidgetTree, NSLOCTEXT("SSTrace", "Subtitle", "역추적 방어  /  제7연구소 통신 단말"), 18, TEXT("C7A985")),
			FVector2D(470, 104), FVector2D(675, 34), 4);
		Place(Canvas, Label(WidgetTree, NSLOCTEXT("SSTrace", "PanelTitle", "통신 상태"), 29, TEXT("EBCB99")),
			FVector2D(1200, 100), FVector2D(310, 45), 4);
		Place(Canvas, Label(WidgetTree, NSLOCTEXT("SSTrace", "ReceiveHeading", "메시지 수신"), 22, TEXT("F1E7D6")),
			FVector2D(1204, 165), FVector2D(300, 35), 4);
		ReceiveBar = WidgetTree->ConstructWidget<UProgressBar>();
		ReceiveBar->SetPercent(0.f);
		ReceiveBar->SetFillColorAndOpacity(Color(TEXT("46CAD6")));
		Place(Canvas, ReceiveBar, FVector2D(1205, 206), FVector2D(280, 18), 4);
		ReceiveText = Label(WidgetTree, NSLOCTEXT("SSTrace", "InitialReceive", "수신 0%"), 25, TEXT("74E3EE"));
		Place(Canvas, ReceiveText, FVector2D(1204, 229), FVector2D(310, 38), 4);
		TurnText = Label(WidgetTree, NSLOCTEXT("SSTrace", "InitialTurns", "남은 송신 6 / 6"), 25, TEXT("F3D18B"));
		Place(Canvas, TurnText, FVector2D(1204, 289), FVector2D(310, 42), 4);
		AlertText = Label(WidgetTree, NSLOCTEXT("SSTrace", "InitialAlert", "적 경계 0 / 3"), 25, TEXT("EB8076"));
		Place(Canvas, AlertText, FVector2D(1204, 358), FVector2D(310, 42), 4);
		RelayText = Label(WidgetTree, NSLOCTEXT("SSTrace", "InitialRelay", "경유: 선택 안 함 · 차단 0곳"), 22, TEXT("F3D18B"));
		Place(Canvas, RelayText, FVector2D(1204, 428), FVector2D(315, 76), 4);
		Place(Canvas, Label(WidgetTree, NSLOCTEXT("SSTrace", "LogHeading", "최근 수신 기록"), 22, TEXT("F1E7D6")),
			FVector2D(1204, 531), FVector2D(300, 34), 4);
		LogText = Label(WidgetTree, NSLOCTEXT("SSTrace", "LogPlaceholder", "외부 기록 수신 대기 중"), 18, TEXT("AEB9B4"));
		Place(Canvas, LogText, FVector2D(1204, 573), FVector2D(300, 140), 4);

		SendDirectButton = Button(WidgetTree, NSLOCTEXT("SSTrace", "DirectButton", "직접 송신"), TEXT("7DE5F1"), 30);
		Place(Canvas, SendDirectButton, FVector2D(255, 727), FVector2D(348, 81), 8);
		SendRelayButton = Button(WidgetTree, NSLOCTEXT("SSTrace", "RelayButton", "중계기 경유"), TEXT("F5D08D"), 30);
		Place(Canvas, SendRelayButton, FVector2D(660, 727), FVector2D(348, 81), 8);
		EndButton = Button(WidgetTree, NSLOCTEXT("SSTrace", "EndButton", "접속 종료"), TEXT("FF9B91"), 30);
		Place(Canvas, EndButton, FVector2D(1061, 727), FVector2D(348, 81), 8);
		StatusText = Label(WidgetTree, NSLOCTEXT("SSTrace", "InitialStatus", "지도에서 통신 단자를 선택해 경유할 수 있습니다."),
			21, TEXT("D6C5AB"), ETextJustify::Center);
		Place(Canvas, StatusText, FVector2D(320, 838), FVector2D(1030, 46), 4);
		CloseButton = Button(WidgetTree, NSLOCTEXT("SSTrace", "CloseButton", "닫기"), TEXT("D0BC9C"), 20);
		Place(Canvas, CloseButton, FVector2D(1475, 38), FVector2D(110, 45), 8);
		CloseButton->SetVisibility(ESlateVisibility::Collapsed);
	}
	return Super::RebuildWidget();
}

bool USSTraceWidget::StartTrace(USSTraceConfig* InConfig)
{
	if (!IsValid(InConfig) || InConfig->SensorPositions.Num() < 3
		|| InConfig->MapSize.X <= 0.0 || InConfig->MapSize.Y <= 0.0
		|| InConfig->ReceiveGoal < 1 || InConfig->MaxTurns < 1) return false;

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		Run = GameInstance->GetSubsystem<USSRunSubsystem>();
	}
	if (!IsValid(Run) || !Run->GetComms()->BeginTrace()) return false;   // 오늘 이미 했거나 행동력 부족

	Config = InConfig;
	bFinished = false;

	// 날을 넘어온 적의 기억, 받은 횟수, 차단된 단자를 이어받아 판 시작
	Session = NewObject<USSTraceSession>(this);
	USSCommsState* Comms = Run->GetComms();
	Session->Initialize(Config, Comms->GetTraceSpots(), Comms->GetBlockedTraceRelays(), Comms->GetTraceReceived());
	Session->OnTraceChanged.AddUniqueDynamic(this, &ThisClass::Refresh);

	if (MapView) MapView->SetSource(Session, Config);

	SetStatus(NSLOCTEXT("SSTrace", "Intro", "다음 메시지를 요청할 때마다 신호가 나가. 적 센서가 거리를 재서 위치를 좁혀 와."));
	Refresh();
	return true;
}

void USSTraceWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (SendDirectButton) SendDirectButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleSendDirect);
	if (SendRelayButton) SendRelayButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleSendRelay);
	if (EndButton) EndButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleEnd);
	if (CloseButton) CloseButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleClose);
	if (MapView) MapView->OnMapClicked.AddUniqueDynamic(this, &ThisClass::HandleMapClicked);

	// StartTrace가 화면이 만들어지기 전에 불렸으면, 지금 지도에 판을 연결하고 수치를 채움
	if (IsValid(Session))
	{
		if (MapView) MapView->SetSource(Session, Config);
		SetStatus(NSLOCTEXT("SSTrace", "Intro", "다음 메시지를 요청할 때마다 신호가 나가. 적 센서가 거리를 재서 위치를 좁혀 와."));
		Refresh();
	}
}

void USSTraceWidget::NativeDestruct()
{
	if (SendDirectButton) SendDirectButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleSendDirect);
	if (SendRelayButton) SendRelayButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleSendRelay);
	if (EndButton) EndButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleEnd);
	if (CloseButton) CloseButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleClose);
	if (MapView) MapView->OnMapClicked.RemoveDynamic(this, &ThisClass::HandleMapClicked);
	if (IsValid(Session)) Session->OnTraceChanged.RemoveDynamic(this, &ThisClass::Refresh);

	// 판이 끝나기 전에 창이 닫히면(HUD가 사라지는 등) 받은 만큼 저장하고 끝냄
	if (IsValid(Session) && !bFinished)
	{
		Session->EndSession();
		if (IsValid(Run)) Run->GetComms()->FinishTrace(*Session);
		bFinished = true;
	}
	Super::NativeDestruct();
}

void USSTraceWidget::HandleSendDirect()
{
	if (!IsValid(Session) || !IsValid(Config)) return;
	if (Session->SendDirect() && MapView)
	{
		MapView->PlayBurst(Config->ShelterPosition);
	}
}

void USSTraceWidget::HandleSendRelay()
{
	if (!IsValid(Session) || !IsValid(Config)) return;
	const FVector2D RelayPosition = Session->GetRelayPosition();   // 송신 뒤 차단돼 선택이 풀릴 수 있어서 미리 기억
	if (Session->SendViaRelay() && MapView)
	{
		MapView->PlayBurst(RelayPosition);
		MapView->PlayBurst(Config->ShelterPosition, true);   // 은신처에서 새어 나간 약한 신호
	}
}

void USSTraceWidget::HandleEnd()
{
	if (IsValid(Session)) Session->EndSession();
}

void USSTraceWidget::HandleClose()
{
	RemoveFromParent();
}

void USSTraceWidget::HandleMapClicked(FVector2D MapPosition)
{
	if (!IsValid(Session) || !IsValid(Config)) return;

	// 클릭한 곳에서 가장 가까운 통신 단자 찾기 (클릭 판정 거리 안에서만)
	int32 Nearest = INDEX_NONE;
	double NearestDist = Config->RelayPickRadius;
	for (int32 i = 0; i < Config->RelayPositions.Num(); ++i)
	{
		const double Dist = FVector2D::Distance(MapPosition, Config->RelayPositions[i]);
		if (Dist <= NearestDist)
		{
			NearestDist = Dist;
			Nearest = i;
		}
	}
	if (Nearest == INDEX_NONE) return;   // 단자가 아닌 곳을 누름

	if (Session->IsRelayBlocked(Nearest))
	{
		SetStatus(NSLOCTEXT("SSTrace", "RelayBlocked", "그 단자는 적이 확인해서 차단됐어. 며칠 뒤에 다시 쓸 수 있어."));
		return;
	}
	Session->SelectRelay(Nearest);
}

void USSTraceWidget::Refresh()
{
	if (!IsValid(Session) || !IsValid(Config)) return;

	const bool bInProgress = Session->GetOutcome() == ESSTraceOutcome::InProgress;
	const int32 Goal = FMath::Max(1, Config->ReceiveGoal);
	const float Ratio = FMath::Clamp(float(Session->GetReceived()) / Goal, 0.f, 1.f);

	if (ReceiveBar) ReceiveBar->SetPercent(Ratio);
	if (ReceiveText) ReceiveText->SetText(FText::Format(NSLOCTEXT("SSTrace", "Receive", "수신 {0}%"), FMath::RoundToInt(Ratio * 100.f)));
	if (LogText)
	{
		LogText->SetText(Session->GetReceived() > 0
			? FText::Format(NSLOCTEXT("SSTrace", "ReceivedFragments", "외부 신호 조각 {0} / {1} 수신\n전문은 수신 완료 후 해독됩니다."), Session->GetReceived(), Goal)
			: NSLOCTEXT("SSTrace", "WaitingForFragments", "외부 기록 수신 대기 중"));
	}
	if (TurnText)
	{
		TurnText->SetText(FText::Format(NSLOCTEXT("SSTrace", "Turn", "남은 송신 {0} / {1}"),
			FMath::Max(0, Config->MaxTurns - Session->GetTurn()), Config->MaxTurns));
	}
	if (RelayText)
	{
		int32 BlockedCount = 0;
		for (int32 i = 0; i < Config->RelayPositions.Num(); ++i)
		{
			if (Session->IsRelayBlocked(i)) ++BlockedCount;
		}
		const FText Selected = Session->HasRelay()
			? FText::Format(NSLOCTEXT("SSTrace", "RelaySelected", "단자 {0}"), Session->GetSelectedRelay() + 1)
			: NSLOCTEXT("SSTrace", "RelayNone", "선택 안 함");
		RelayText->SetText(FText::Format(NSLOCTEXT("SSTrace", "Relay", "경유: {0} · 차단 {1}곳"), Selected, BlockedCount));
	}
	if (AlertText)
	{
		AlertText->SetText(FText::Format(NSLOCTEXT("SSTrace", "Alert", "적 경계 {0} / {1}"),
			Session->GetAlertLevel(), Config->MaxAlertLevel));
	}

	if (SendDirectButton) SendDirectButton->SetIsEnabled(bInProgress);
	if (SendRelayButton) SendRelayButton->SetIsEnabled(bInProgress && Session->HasRelay());
	if (EndButton) EndButton->SetIsEnabled(bInProgress);
	if (CloseButton) CloseButton->SetVisibility(bInProgress ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);

	if (bInProgress || bFinished) return;

	// ── 판이 끝남: 결과를 한 번만 넘기고 문구 표시 ──
	bFinished = true;
	if (IsValid(Run)) Run->GetComms()->FinishTrace(*Session);

	switch (Session->GetOutcome())
	{
	case ESSTraceOutcome::Exposed:
		if (MapView) MapView->PlayExposed();
		SetStatus(NSLOCTEXT("SSTrace", "ExposedStatus", "위치 노출. 적이 은신처를 확정했어. 오늘 밤 조심해."));
		break;
	case ESSTraceOutcome::Completed:
		ShowReceivedMessage();
		break;
	default:
		SetStatus(NSLOCTEXT("SSTrace", "StoppedStatus", "접속을 끊었어. 받은 만큼은 내일 이어서 받을 수 있어."));
		break;
	}

	OnTraceFinished.Broadcast(Session->GetOutcome());
}

void USSTraceWidget::ShowReceivedMessage()
{
	if (!IsValid(Run)) return;

	// 다 받은 메시지는 해독 대기함에 들어감 (비었으면 고를 메시지가 없었던 것)
	const TArray<FSSPendingMessage>& Pending = Run->GetComms()->GetPendingMessages();
	if (Pending.IsEmpty())
	{
		SetStatus(NSLOCTEXT("SSTrace", "CompletedStatus", "메시지를 끝까지 받았어. 새로 알아낸 건 없어."));
		return;
	}

	// 방금 들어간 메시지 = 대기함의 마지막 칸 (EnqueueMessage가 맨 뒤에 추가)
	const FSSPendingMessage& Latest = Pending.Last();

	// 기한: 실용 정보는 마지막으로 풀 수 있는 날, 진실 단서는 기한 없음(0)
	FText Deadline;
	if (Latest.ExpireDay == 0)
	{
		Deadline = NSLOCTEXT("SSTrace", "NoDeadline", "기한 없음");
	}
	else
	{
		Deadline = FText::Format(NSLOCTEXT("SSTrace", "Deadline", "{0}일차까지 해독"), Latest.ExpireDay);
	}

	// 오른쪽 기록 칸: 내용은 모르고, 대기 개수와 기한만
	if (LogText)
	{
		LogText->SetText(FText::Format(
			NSLOCTEXT("SSTrace", "QueuedLog", "[암호화된 메시지]\n해독 대기 {0}개 · {1}"),
			Pending.Num(),
			Deadline));
	}

	SetStatus(NSLOCTEXT("SSTrace", "QueuedStatus", "암호문 수신 완료. 컴퓨터에서 해독할 수 있어."));
}

void USSTraceWidget::SetStatus(const FText& Text)
{
	if (StatusText) StatusText->SetText(Text);
}
