#include "UI/Decode/SSDialDecodeWidget.h"
#include "UI/Decode/SSDialControlWidget.h"
#include "Decode/SSDialSession.h"
#include "Comms/SSCommsState.h"
#include "Event/SSEventDirector.h"
#include "Item/SSRunSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ProgressBar.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Styling/CoreStyle.h"

namespace SSDecodeStyle
{
	FLinearColor Color(const TCHAR* Hex)
	{
		return FLinearColor::FromSRGBColor(FColor::FromHex(Hex));
	}

	// 다이얼마다 색 (6개까지)
	const TCHAR* const DialHex[] = { TEXT("F0C27A"), TEXT("74E3EE"), TEXT("D9A0E8"), TEXT("8FE0A0"), TEXT("EB8076"), TEXT("C7C7C7") };

	UTextBlock* Label(UWidgetTree* Tree, const FText& Text, int32 Size, const TCHAR* Hex)
	{
		UTextBlock* Widget = Tree->ConstructWidget<UTextBlock>();
		Widget->SetText(Text);
		FSlateFontInfo Font = Widget->GetFont();
		Font.Size = Size;
		Widget->SetFont(Font);
		Widget->SetColorAndOpacity(FSlateColor(Color(Hex)));
		Widget->SetAutoWrapText(true);
		return Widget;
	}

	UButton* Button(UWidgetTree* Tree, const FText& Text)
	{
		UButton* Widget = Tree->ConstructWidget<UButton>();
		FButtonStyle Style = Widget->GetStyle();
		Style.Normal.TintColor = FSlateColor(Color(TEXT("6D5032")));
		Style.Hovered.TintColor = FSlateColor(Color(TEXT("92704A")));
		Style.Pressed.TintColor = FSlateColor(Color(TEXT("47321F")));
		Widget->SetStyle(Style);
		Widget->SetContent(Label(Tree, Text, 19, TEXT("F3E8D6")));
		CastChecked<UButtonSlot>(Widget->GetContent()->Slot)->SetPadding(FMargin(22, 10));
		return Widget;
	}
}

bool USSDialDecodeWidget::StartDecode(int32 PendingIndex)
{
	using namespace SSDecodeStyle;

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		Run = GameInstance->GetSubsystem<USSRunSubsystem>();
	}
	if (!IsValid(Run)) return false;

	// 대기함 정보는 세션을 만들기 전에 읽어둠 (풀리면 대기함에서 빠짐)
	const TArray<FSSPendingMessage>& PendingList = Run->GetComms()->GetPendingMessages();
	if (!PendingList.IsValidIndex(PendingIndex)) return false;
	const FSSPendingMessage& Pending = PendingList[PendingIndex];

	Session = NewObject<USSDialSession>(this);
	if (!Session->Initialize(Run, PendingIndex)) return false;
	Session->OnDialChanged.AddUniqueDynamic(this, &ThisClass::Refresh);
	bResultShown = false;

	// 위쪽 안내: 종류와 기한 (내용은 풀기 전까지 모름)
	if (InfoText)
	{
		const bool bTruth = Pending.Row.Kind == ESSTraceMessageKind::Truth;
		const FText Kind = bTruth
			? NSLOCTEXT("SSDecode", "KindTruth", "우선순위 통신")
			: NSLOCTEXT("SSDecode", "KindInfo", "일반 통신");
		const FText Deadline = Pending.ExpireDay == 0
			? NSLOCTEXT("SSDecode", "NoDeadline", "기한 없음")
			: FText::Format(NSLOCTEXT("SSDecode", "Deadline", "{0}일차까지"), Pending.ExpireDay);
		InfoText->SetText(FText::Format(NSLOCTEXT("SSDecode", "Info", "{0} · {1} · 다이얼 {2}개"), Kind, Deadline, Session->GetDialCount()));
	}

	// 다이얼 칸 만들기 (개수가 메시지마다 다를 수 있어서 여기서)
	if (DialRow)
	{
		DialRow->ClearChildren();
		DialControls.Reset();
		for (int32 DialIndex = 0; DialIndex < Session->GetDialCount(); ++DialIndex)
		{
			USSDialControlWidget* Control = CreateWidget<USSDialControlWidget>(this, USSDialControlWidget::StaticClass());
			Control->Setup(DialIndex, Color(DialHex[DialIndex % UE_ARRAY_COUNT(DialHex)]));
			Control->OnDialTurn.AddUniqueDynamic(this, &ThisClass::HandleDialTurn);

			UHorizontalBoxSlot* ControlSlot = DialRow->AddChildToHorizontalBox(Control);
			ControlSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			ControlSlot->SetPadding(FMargin(DialIndex == 0 ? 0 : 12, 0, 0, 0));
			DialControls.Add(Control);
		}
	}

	if (StatusText)
	{
		StatusText->SetText(NSLOCTEXT("SSDecode", "Intro", "다이얼을 돌려 신호가 가장 강한 곳을 찾아. 다이얼 셋이 모두 맞으면 잠금이 풀려."));
	}
	if (ResultText) ResultText->SetText(FText::GetEmpty());

	// 시간제한 해독은 닫으면 포기, 진실 통신은 진행 저장
	if (CloseButton)
	{
		if (UTextBlock* CloseLabel = Cast<UTextBlock>(CloseButton->GetContent()))
		{
			CloseLabel->SetText(Session->HasTimeLimit()
				? NSLOCTEXT("SSDecode", "GiveUp", "포기 (메시지 소실)")
				: NSLOCTEXT("SSDecode", "Close", "닫기 (진행 저장)"));
		}
	}
	if (TimerPanel) TimerPanel->SetVisibility(Session->HasTimeLimit()
		? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);

	Refresh();
	UpdateTimerDisplay();
	return true;
}

TSharedRef<SWidget> USSDialDecodeWidget::RebuildWidget()
{
	using namespace SSDecodeStyle;
	if (!WidgetTree->RootWidget)
	{
		UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
		WidgetTree->RootWidget = Canvas;

		// 뒤 화면 어둡게
		UBorder* Blocker = WidgetTree->ConstructWidget<UBorder>();
		Blocker->SetBrushColor(FLinearColor(0, 0, 0, .6f));
		UCanvasPanelSlot* BlockerSlot = Canvas->AddChildToCanvas(Blocker);
		BlockerSlot->SetAnchors(FAnchors(0, 0, 1, 1));
		BlockerSlot->SetOffsets(FMargin(0));

		// 해독은 독립 작업 화면: 뷰포트를 채우되 가장자리만 조금 남긴다.
		UBorder* Frame = WidgetTree->ConstructWidget<UBorder>();
		Frame->SetBrushColor(Color(TEXT("A77A48")));
		Frame->SetPadding(FMargin(2));
		UCanvasPanelSlot* FrameSlot = Canvas->AddChildToCanvas(Frame);
		FrameSlot->SetAnchors(FAnchors(0, 0, 1, 1));
		FrameSlot->SetOffsets(FMargin(12));

		UBorder* Body = WidgetTree->ConstructWidget<UBorder>();
		Body->SetBrushColor(Color(TEXT("0B1210")));
		Body->SetPadding(FMargin(28, 24));
		Frame->SetContent(Body);

		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
		Body->SetContent(Column);

		// 상단 제목과 잠금 표시
		UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>();
		Header->AddChildToHorizontalBox(Label(WidgetTree, NSLOCTEXT("SSDecode", "Title", "감청 해독"), 30, TEXT("F2E4D0")));
		InfoText = Label(WidgetTree, FText::GetEmpty(), 15, TEXT("C7A985"));
		Header->AddChildToHorizontalBox(InfoText)->SetPadding(FMargin(14, 10, 0, 0));
		LockStateText = Label(WidgetTree, NSLOCTEXT("SSDecode", "Locked", "● 잠금 상태"), 18, TEXT("EB8076"));
		UHorizontalBoxSlot* LockSlot = Header->AddChildToHorizontalBox(LockStateText);
		LockSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		LockSlot->SetHorizontalAlignment(HAlign_Right);
		Column->AddChildToVerticalBox(Header)->SetPadding(FMargin(0, 0, 0, 12));

		UHorizontalBox* Main = WidgetTree->ConstructWidget<UHorizontalBox>();
		Column->AddChildToVerticalBox(Main)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		UVerticalBox* Left = WidgetTree->ConstructWidget<UVerticalBox>();
		Main->AddChildToHorizontalBox(Left)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		USizeBox* RightSize = WidgetTree->ConstructWidget<USizeBox>();
		RightSize->SetWidthOverride(270.f);
		Main->AddChildToHorizontalBox(RightSize)->SetPadding(FMargin(14, 0, 0, 0));
		UVerticalBox* Right = WidgetTree->ConstructWidget<UVerticalBox>();
		RightSize->SetContent(Right);

		// 긴 암호문은 이 영역 안에서만 스크롤한다.
		UBorder* TextPaper = WidgetTree->ConstructWidget<UBorder>();
		TextPaper->SetBrushColor(Color(TEXT("08100E")));
		TextPaper->SetPadding(FMargin(18, 14));
		UVerticalBox* CipherPanel = WidgetTree->ConstructWidget<UVerticalBox>();
		CipherPanel->AddChildToVerticalBox(Label(WidgetTree, NSLOCTEXT("SSDecode", "Transmission", "수신된 암호문 / ENCRYPTED TRANSMISSION"), 16, TEXT("7ACCC9")))
			->SetPadding(FMargin(0, 0, 0, 8));
		UScrollBox* CipherScroll = WidgetTree->ConstructWidget<UScrollBox>();
		CipherText = WidgetTree->ConstructWidget<UTextBlock>();
		CipherText->SetFont(FCoreStyle::GetDefaultFontStyle("Mono", 20));
		CipherText->SetColorAndOpacity(FSlateColor(Color(TEXT("CFE9E4"))));
		CipherScroll->AddChild(CipherText);
		CipherPanel->AddChildToVerticalBox(CipherScroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		TextPaper->SetContent(CipherPanel);
		USizeBox* CipherHeight = WidgetTree->ConstructWidget<USizeBox>();
		CipherHeight->SetHeightOverride(270.f);
		CipherHeight->SetContent(TextPaper);
		UVerticalBoxSlot* TextSlot = Left->AddChildToVerticalBox(CipherHeight);
		TextSlot->SetPadding(FMargin(0, 0, 0, 10));

		// 해독 결과는 암호문 바로 밑에서 읽을 수 있게 한다.
		ResultPanel = WidgetTree->ConstructWidget<UBorder>();
		ResultPanel->SetBrushColor(Color(TEXT("10251E")));
		ResultPanel->SetPadding(FMargin(12, 8));
		ResultPanel->SetVisibility(ESlateVisibility::Collapsed);
		Left->AddChildToVerticalBox(ResultPanel)->SetPadding(FMargin(0, 0, 0, 12));
		ResultText = Label(WidgetTree, FText::GetEmpty(), 17, TEXT("8FE0A0"));
		ResultPanel->SetContent(ResultText);

		USpacer* FlexibleGap = WidgetTree->ConstructWidget<USpacer>();
		Left->AddChildToVerticalBox(FlexibleGap)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

		// 다이얼 칸들 (StartDecode에서 채움)
		Left->AddChildToVerticalBox(Label(WidgetTree, NSLOCTEXT("SSDecode", "Dials", "복호화 다이얼"), 19, TEXT("EBCB99")))
			->SetPadding(FMargin(0, 0, 0, 6));
		DialRow = WidgetTree->ConstructWidget<UHorizontalBox>();
		Left->AddChildToVerticalBox(DialRow);

		// 시간제한은 숫자와 게이지를 함께 보여주고 위험 구간을 강조한다.
		TimerPanel = WidgetTree->ConstructWidget<UBorder>();
		TimerPanel->SetBrushColor(Color(TEXT("18221F")));
		TimerPanel->SetPadding(FMargin(14));
		Right->AddChildToVerticalBox(TimerPanel)->SetPadding(FMargin(0, 0, 0, 12));
		UVerticalBox* TimerColumn = WidgetTree->ConstructWidget<UVerticalBox>();
		TimerPanel->SetContent(TimerColumn);
		TimerColumn->AddChildToVerticalBox(Label(WidgetTree,
			NSLOCTEXT("SSDecode", "TimerTitle", "신호 유지 시간"), 19, TEXT("EBCB99")));
		TimerText = Label(WidgetTree, FText::GetEmpty(), 36, TEXT("74E3EE"));
		TimerColumn->AddChildToVerticalBox(TimerText)->SetPadding(FMargin(0, 8, 0, 7));
		TimerBar = WidgetTree->ConstructWidget<UProgressBar>();
		TimerBar->SetFillColorAndOpacity(Color(TEXT("74E3EE")));
		TimerColumn->AddChildToVerticalBox(TimerBar)->SetPadding(FMargin(0, 0, 0, 8));
		TimerColumn->AddChildToVerticalBox(Label(WidgetTree,
			NSLOCTEXT("SSDecode", "TimerHelp", "신호가 끊기기 전에 해독하세요. 시간이 끝나면 암호문이 사라집니다."),
			14, TEXT("A7BCB4")));

		// 오른쪽 고정 상태 패널
		UBorder* SignalPanel = WidgetTree->ConstructWidget<UBorder>();
		SignalPanel->SetBrushColor(Color(TEXT("15231F")));
		SignalPanel->SetPadding(FMargin(14));
		Right->AddChildToVerticalBox(SignalPanel)->SetPadding(FMargin(0, 0, 0, 12));
		UVerticalBox* SignalColumn = WidgetTree->ConstructWidget<UVerticalBox>();
		SignalPanel->SetContent(SignalColumn);
		SignalColumn->AddChildToVerticalBox(Label(WidgetTree, NSLOCTEXT("SSDecode", "Signal", "해독 신호"), 19, TEXT("EBCB99")));
		SignalText = Label(WidgetTree, FText::GetEmpty(), 40, TEXT("74E3EE"));
		SignalColumn->AddChildToVerticalBox(SignalText)->SetPadding(FMargin(0, 12, 0, 8));
		SignalBar = WidgetTree->ConstructWidget<UProgressBar>();
		SignalBar->SetFillColorAndOpacity(Color(TEXT("46CAD6")));
		SignalColumn->AddChildToVerticalBox(SignalBar)->SetPadding(FMargin(0, 0, 0, 12));
		SignalColumn->AddChildToVerticalBox(Label(WidgetTree,
			NSLOCTEXT("SSDecode", "SignalHelp", "다이얼마다 맞은 글자의 빈도가 영어와 닮을수록 올라갑니다."), 14, TEXT("A7BCB4")));

		UBorder* ReceivePanel = WidgetTree->ConstructWidget<UBorder>();
		ReceivePanel->SetBrushColor(Color(TEXT("15231F")));
		ReceivePanel->SetPadding(FMargin(14));
		Right->AddChildToVerticalBox(ReceivePanel)->SetPadding(FMargin(0, 0, 0, 12));
		UVerticalBox* ReceiveColumn = WidgetTree->ConstructWidget<UVerticalBox>();
		ReceivePanel->SetContent(ReceiveColumn);
		ReceiveColumn->AddChildToVerticalBox(Label(WidgetTree, NSLOCTEXT("SSDecode", "Receive", "수신 상태"), 19, TEXT("EBCB99")));
		ReceiveColumn->AddChildToVerticalBox(Label(WidgetTree, NSLOCTEXT("SSDecode", "Received", "● 암호문 수신 완료"), 16, TEXT("8FE0A0")))
			->SetPadding(FMargin(0, 10, 0, 0));
		ReceiveColumn->AddChildToVerticalBox(Label(WidgetTree, NSLOCTEXT("SSDecode", "Decrypting", "● 복호화 진행 중"), 16, TEXT("C7A985")));

		// 안내 문구 + 버튼
		StatusText = Label(WidgetTree, FText::GetEmpty(), 17, TEXT("D6C5AB"));
		Column->AddChildToVerticalBox(StatusText)->SetPadding(FMargin(0, 12, 0, 0));
		HintButton = Button(WidgetTree, NSLOCTEXT("SSDecode", "Hint", "힌트 (행동력 1)"));
		Right->AddChildToVerticalBox(HintButton)->SetPadding(FMargin(0, 0, 0, 8));
		CloseButton = Button(WidgetTree, NSLOCTEXT("SSDecode", "Close", "닫기 (진행 저장)"));
		Right->AddChildToVerticalBox(CloseButton);
	}
	return Super::RebuildWidget();
}

void USSDialDecodeWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 다이얼 소리 (WBP 없이 코드로 만드는 창이라 경로로 불러옴. 에셋이 없으면 소리 없이 동작)
	DialTickSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Assets/Audio/Decode/SS_DialTurn_Click.SS_DialTurn_Click"));

	if (HintButton) HintButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleHint);
	if (CloseButton) CloseButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleClose);
}

void USSDialDecodeWidget::NativeDestruct()
{
	// 시간제한 해독 도중에 창이 닫히면 포기 (닫았다 다시 열어 시간을 벌지 못하게)
	if (IsValid(Session)) Session->GiveUp();

	if (HintButton) HintButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleHint);
	if (CloseButton) CloseButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleClose);
	if (IsValid(Session)) Session->OnDialChanged.RemoveDynamic(this, &ThisClass::Refresh);
	for (USSDialControlWidget* Control : DialControls)
	{
		if (IsValid(Control)) Control->OnDialTurn.RemoveDynamic(this, &ThisClass::HandleDialTurn);
	}
	Super::NativeDestruct();
}

void USSDialDecodeWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!IsValid(Session) || !Session->HasTimeLimit()) return;

	// 시간 흘려보내기 (0이 되면 세션이 실패 처리 → Refresh로 화면 갱신)
	Session->Tick(InDeltaTime);
	UpdateTimerDisplay();
}

void USSDialDecodeWidget::UpdateTimerDisplay()
{
	if (!IsValid(Session) || !Session->HasTimeLimit()) return;
	const bool bUnlocked = Session->IsUnlocked();
	const bool bFailed = Session->IsFailed();
	const float Remaining = Session->GetTimeLeft();
	const bool bDanger = Remaining <= 10.f && !bUnlocked && !bFailed;
	const bool bBlinkOn = FMath::Fmod(Remaining, 1.f) > .5f;
	const TCHAR* TimerColor = bUnlocked ? TEXT("8FE0A0")
		: bFailed ? TEXT("9AA39E")
		: bDanger ? (bBlinkOn ? TEXT("EB8076") : TEXT("7A3A34"))
		: TEXT("74E3EE");
	if (TimerText)
	{
		const int32 Seconds = FMath::CeilToInt(Remaining);
		TimerText->SetText(bUnlocked ? NSLOCTEXT("SSDecode", "TimerSuccess", "해독 완료")
			: bFailed ? NSLOCTEXT("SSDecode", "TimerFailed", "신호 소실")
			: FText::FromString(FString::Printf(TEXT("%02d:%02d"), Seconds / 60, Seconds % 60)));
		TimerText->SetColorAndOpacity(FSlateColor(SSDecodeStyle::Color(TimerColor)));
	}
	if (TimerBar)
	{
		TimerBar->SetPercent(bUnlocked ? 1.f : Remaining / USSDialSession::InfoTimeLimit);
		TimerBar->SetFillColorAndOpacity(SSDecodeStyle::Color(TimerColor));
	}
	if (TimerPanel)
	{
		TimerPanel->SetBrushColor(SSDecodeStyle::Color(bDanger ? TEXT("34201E") : TEXT("18221F")));
	}
}

void USSDialDecodeWidget::HandleDialTurn(int32 DialIndex, int32 Step)
{
	// 풀린 뒤엔 돌아가지 않으니 소리도 안 냄
	if (!IsValid(Session) || Session->IsUnlocked() || Session->IsFailed()) return;

	Session->TurnDial(DialIndex, Step);

	// 버튼·휠 모두 여기를 거치므로 한 곳에서 재생
	if (DialTickSound) UGameplayStatics::PlaySound2D(this, DialTickSound);
}

void USSDialDecodeWidget::HandleHint()
{
	if (!IsValid(Session)) return;
	if (!Session->UseHint() && StatusText)
	{
		StatusText->SetText(NSLOCTEXT("SSDecode", "NoHint", "행동력이 부족해서 힌트를 쓸 수 없어."));
	}
}

void USSDialDecodeWidget::HandleClose()
{
	RemoveFromParent();
}

void USSDialDecodeWidget::Refresh()
{
	if (!IsValid(Session)) return;

	const bool bUnlocked = Session->IsUnlocked();
	const bool bFailed = Session->IsFailed();

	// 신호 소실: 다이얼 멈추고 안내 (아래 문장·다이얼 갱신은 건너뜀)
	if (bFailed)
	{
		if (LockStateText)
		{
			LockStateText->SetText(NSLOCTEXT("SSDecode", "LostBadge", "● 신호 소실"));
			LockStateText->SetColorAndOpacity(FSlateColor(SSDecodeStyle::Color(TEXT("9AA39E"))));
		}
		if (CipherText) CipherText->SetColorAndOpacity(FSlateColor(SSDecodeStyle::Color(TEXT("5A6560"))));
		for (USSDialControlWidget* Control : DialControls)
		{
			if (IsValid(Control)) Control->ShowState(0, 0.f, true);
		}
		if (HintButton) HintButton->SetIsEnabled(false);
		if (StatusText) StatusText->SetText(NSLOCTEXT("SSDecode", "Lost", "신호가 끊겼어. 메시지가 사라졌어."));
		if (CloseButton)
		{
			if (UTextBlock* CloseLabel = Cast<UTextBlock>(CloseButton->GetContent()))
			{
				CloseLabel->SetText(NSLOCTEXT("SSDecode", "Done", "닫기"));
			}
		}
		return;
	}

	if (LockStateText)
	{
		LockStateText->SetText(bUnlocked
			? NSLOCTEXT("SSDecode", "UnlockedBadge", "● 잠금 해제")
			: NSLOCTEXT("SSDecode", "Locked", "● 잠금 상태"));
		LockStateText->SetColorAndOpacity(FSlateColor(SSDecodeStyle::Color(bUnlocked ? TEXT("8FE0A0") : TEXT("EB8076"))));
	}

	// 다이얼 번호 줄은 암호문을 가려서 표시하지 않는다.
	if (CipherText)
	{
		CipherText->SetText(FText::FromString(FormatCipherText(Session->GetShownText())));
		CipherText->SetColorAndOpacity(FSlateColor(SSDecodeStyle::Color(bUnlocked ? TEXT("8FE0A0") : TEXT("CFE9E4"))));
	}

	// 다이얼 칸마다 값·신호 세기, 전체 신호 세기는 평균
	float Total = 0.f;
	for (int32 DialIndex = 0; DialIndex < DialControls.Num(); ++DialIndex)
	{
		const float Strength = Session->GetDialStrength(DialIndex);
		Total += Strength;
		if (IsValid(DialControls[DialIndex]))
		{
			DialControls[DialIndex]->ShowState(Session->GetDial(DialIndex), Strength, bUnlocked);
		}
	}
	const float Average = DialControls.IsEmpty() ? 0.f : Total / DialControls.Num();
	if (SignalBar) SignalBar->SetPercent(Average);
	if (SignalText) SignalText->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Average * 100.f))));

	if (HintButton) HintButton->SetIsEnabled(!bUnlocked && Run->GetActionPoints() >= 1);

	if (bUnlocked && !bResultShown)
	{
		bResultShown = true;
		ShowResult();
	}
}

void USSDialDecodeWidget::ShowResult()
{
	// 해독 성공 처리는 세션이 이미 함 (CommsState::DecodeMessage) → 결과만 읽어서 표시
	const FSSEventResult& Message = Run->GetComms()->GetLastMessage();
	const FText Changes = Run->GetEventDirector()->DescribeChanges(Message);
	const FText Korean = Message.Lines.IsEmpty() ? FText::GetEmpty() : Message.Lines[0];

	if (ResultText)
	{
		if (ResultPanel) ResultPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		ResultText->SetText(Changes.IsEmpty()
			? FText::Format(NSLOCTEXT("SSDecode", "Result", "[{0}] {1}"), Message.Title, Korean)
			: FText::Format(NSLOCTEXT("SSDecode", "ResultChanges", "[{0}] {1}\n→ {2}"), Message.Title, Korean, Changes));
	}
	if (StatusText) StatusText->SetText(NSLOCTEXT("SSDecode", "Unlocked", "잠금 해제. 기록창에 남겼어."));
	if (CloseButton && CloseButton->GetContent())
	{
		if (UTextBlock* CloseLabel = Cast<UTextBlock>(CloseButton->GetContent()))
		{
			CloseLabel->SetText(NSLOCTEXT("SSDecode", "Done", "닫기"));
		}
	}
}

FString USSDialDecodeWidget::FormatCipherText(const FString& Text)
{
	constexpr int32 MaxLineLength = 44;

	// 단어 단위로 줄 나누기 (한 줄이 MaxLineLength를 넘지 않게)
	TArray<FString> Words;
	Text.ParseIntoArray(Words, TEXT(" "), true);

	TArray<FString> Lines;
	FString Line;
	for (const FString& Word : Words)
	{
		if (!Line.IsEmpty() && Line.Len() + 1 + Word.Len() > MaxLineLength)
		{
			Lines.Add(Line);
			Line.Reset();
		}
		if (!Line.IsEmpty()) Line.AppendChar(TEXT(' '));
		Line += Word;
	}
	if (!Line.IsEmpty()) Lines.Add(Line);

	return FString::Join(Lines, TEXT("\n"));
}
