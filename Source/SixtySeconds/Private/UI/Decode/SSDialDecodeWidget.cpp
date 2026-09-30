#include "UI/Decode/SSDialDecodeWidget.h"
#include "UI/Decode/SSDialControlWidget.h"
#include "Decode/SSDialSession.h"
#include "Decode/SSCipher.h"
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
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
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

	Refresh();
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
		UVerticalBoxSlot* TextSlot = Left->AddChildToVerticalBox(TextPaper);
		TextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		TextSlot->SetPadding(FMargin(0, 0, 0, 10));

		// 다이얼 칸들 (StartDecode에서 채움)
		Left->AddChildToVerticalBox(Label(WidgetTree, NSLOCTEXT("SSDecode", "Dials", "복호화 다이얼"), 19, TEXT("EBCB99")))
			->SetPadding(FMargin(0, 0, 0, 6));
		DialRow = WidgetTree->ConstructWidget<UHorizontalBox>();
		Left->AddChildToVerticalBox(DialRow)->SetPadding(FMargin(0, 0, 0, 10));

		// 잠금 해제 후 나타나는 기록
		ResultPanel = WidgetTree->ConstructWidget<UBorder>();
		ResultPanel->SetBrushColor(Color(TEXT("10251E")));
		ResultPanel->SetPadding(FMargin(12, 8));
		ResultPanel->SetVisibility(ESlateVisibility::Collapsed);
		Left->AddChildToVerticalBox(ResultPanel);
		ResultText = Label(WidgetTree, FText::GetEmpty(), 17, TEXT("8FE0A0"));
		ResultPanel->SetContent(ResultText);

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
	if (HintButton) HintButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleHint);
	if (CloseButton) CloseButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleClose);
}

void USSDialDecodeWidget::NativeDestruct()
{
	if (HintButton) HintButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleHint);
	if (CloseButton) CloseButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleClose);
	if (IsValid(Session)) Session->OnDialChanged.RemoveDynamic(this, &ThisClass::Refresh);
	for (USSDialControlWidget* Control : DialControls)
	{
		if (IsValid(Control)) Control->OnDialTurn.RemoveDynamic(this, &ThisClass::HandleDialTurn);
	}
	Super::NativeDestruct();
}

void USSDialDecodeWidget::HandleDialTurn(int32 DialIndex, int32 Step)
{
	if (IsValid(Session)) Session->TurnDial(DialIndex, Step);
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
	if (LockStateText)
	{
		LockStateText->SetText(bUnlocked
			? NSLOCTEXT("SSDecode", "UnlockedBadge", "● 잠금 해제")
			: NSLOCTEXT("SSDecode", "Locked", "● 잠금 상태"));
		LockStateText->SetColorAndOpacity(FSlateColor(SSDecodeStyle::Color(bUnlocked ? TEXT("8FE0A0") : TEXT("EB8076"))));
	}

	// 문장: 풀기 전엔 줄 밑에 다이얼 번호, 풀리면 원문만
	if (CipherText)
	{
		CipherText->SetText(FText::FromString(FormatWithMarks(Session->GetShownText(), Session->GetDialCount(), !bUnlocked)));
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

FString USSDialDecodeWidget::FormatWithMarks(const FString& Text, int32 DialCount, bool bShowMarks)
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

	// 줄마다: 문장 줄, 그 밑에 글자별 다이얼 번호 줄 (글자가 아니면 빈칸, 순서는 줄을 넘어 이어짐)
	FString Result;
	int32 LetterCount = 0;
	for (const FString& Each : Lines)
	{
		Result += Each;
		Result.AppendChar(TEXT('\n'));
		if (!bShowMarks) continue;

		FString Marks;
		for (const TCHAR Ch : Each)
		{
			if (FSSCipher::IsLetter(Ch) && DialCount > 0)
			{
				Marks.AppendChar(TCHAR(TEXT('1') + LetterCount % DialCount));
				++LetterCount;
			}
			else
			{
				Marks.AppendChar(TEXT(' '));
			}
		}
		Result += Marks;
		Result.AppendChar(TEXT('\n'));
	}
	return Result;
}
