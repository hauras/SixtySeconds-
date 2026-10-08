#include "UI/Decode/SSTruthDecodeWidget.h"
#include "UI/Decode/SSCipherCellWidget.h"
#include "Decode/SSTruthSession.h"
#include "Decode/SSCipher.h"
#include "Comms/SSCommsState.h"
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
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
#include "Engine/GameInstance.h"
#include "Styling/CoreStyle.h"

namespace SSTruthStyle
{
	FLinearColor Color(const TCHAR* Hex)
	{
		return FLinearColor::FromSRGBColor(FColor::FromHex(Hex));
	}

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
		Style.Normal.TintColor = FSlateColor(Color(TEXT("3A2B50")));
		Style.Hovered.TintColor = FSlateColor(Color(TEXT("52406E")));
		Style.Pressed.TintColor = FSlateColor(Color(TEXT("241A33")));
		Widget->SetStyle(Style);
		Widget->SetContent(Label(Tree, Text, 18, TEXT("E7D6FA")));
		CastChecked<UButtonSlot>(Widget->GetContent()->Slot)->SetPadding(FMargin(20, 10));
		return Widget;
	}

	UBorder* Panel(UWidgetTree* Tree, const TCHAR* Hex, const FMargin& Padding)
	{
		UBorder* Widget = Tree->ConstructWidget<UBorder>();
		Widget->SetBrushColor(Color(Hex));
		Widget->SetPadding(Padding);
		return Widget;
	}
}

bool USSTruthDecodeWidget::StartDecode(int32 PendingIndex)
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		Run = GameInstance->GetSubsystem<USSRunSubsystem>();
	}
	if (!IsValid(Run)) return false;

	Session = NewObject<USSTruthSession>(this);
	if (!Session->Initialize(Run, PendingIndex)) return false;
	Session->OnTruthChanged.AddUniqueDynamic(this, &ThisClass::Refresh);

	SelectedLetter = INDEX_NONE;
	SolverElapsed = 0.f;
	bResultShown = false;
	LastChartTries = -1;
	LastLoggedAccepted = 0;
	FitnessHistory.Reset();
	SolverLogLines.Reset();

	// 치환표 칸: 암호문에 나온 글자만, 많이 나온 순
	if (CellBox)
	{
		CellBox->ClearChildren();
		Cells.Reset();
		for (const int32 Letter : Session->GetCipherLetters())
		{
			USSCipherCellWidget* Cell = CreateWidget<USSCipherCellWidget>(this, USSCipherCellWidget::StaticClass());
			Cell->Setup(Letter, Session->GetLetterCount(Letter));
			Cell->OnCellClicked.AddUniqueDynamic(this, &ThisClass::HandleCellClicked);
			CellBox->AddChildToWrapBox(Cell)->SetPadding(FMargin(0, 0, 6, 6));
			Cells.Add(Cell);
		}
	}

	if (StatusText)
	{
		StatusText->SetText(Session->IsSolverDone()
				? NSLOCTEXT("SSTruth", "Resume", "보라색 빈칸을 누르고 키보드로 원래 글자를 넣어.")
				: NSLOCTEXT("SSTruth", "Intro", "평소와 다른 방식으로 잠긴 통신이야. 자동 해독기를 돌려보자."));
	}

	Refresh();
	return true;
}

TSharedRef<SWidget> USSTruthDecodeWidget::RebuildWidget()
{
	using namespace SSTruthStyle;
	if (!WidgetTree->RootWidget)
	{
		UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
		WidgetTree->RootWidget = Canvas;

		// 뒤 화면 어둡게
		UBorder* Blocker = WidgetTree->ConstructWidget<UBorder>();
		Blocker->SetBrushColor(FLinearColor(0, 0, 0, .65f));
		UCanvasPanelSlot* BlockerSlot = Canvas->AddChildToCanvas(Blocker);
		BlockerSlot->SetAnchors(FAnchors(0, 0, 1, 1));
		BlockerSlot->SetOffsets(FMargin(0));

		// 보라 테두리 (우선순위 통신)
		UBorder* Frame = Panel(WidgetTree, TEXT("6D4F86"), FMargin(2));
		UCanvasPanelSlot* FrameSlot = Canvas->AddChildToCanvas(Frame);
		FrameSlot->SetAnchors(FAnchors(0, 0, 1, 1));
		FrameSlot->SetOffsets(FMargin(12));

		UBorder* Body = Panel(WidgetTree, TEXT("0B1210"), FMargin(28, 24));
		Frame->SetContent(Body);
		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
		Body->SetContent(Column);

		// 제목 줄: 감청 해독 · PRIORITY · 안내 · 잠금 상태
		UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>();
		Header->AddChildToHorizontalBox(Label(WidgetTree, NSLOCTEXT("SSTruth", "Title", "감청 해독"), 30, TEXT("F2E4D0")));
		UBorder* Badge = Panel(WidgetTree, TEXT("2A2036"), FMargin(10, 3));
		Badge->SetContent(Label(WidgetTree, NSLOCTEXT("SSTruth", "Priority", "PRIORITY"), 13, TEXT("C9A3F0")));
		Header->AddChildToHorizontalBox(Badge)->SetPadding(FMargin(14, 10, 0, 0));
		Header->AddChildToHorizontalBox(Label(WidgetTree, NSLOCTEXT("SSTruth", "Info", "우선순위 통신 · 기한 없음"), 15, TEXT("C7A985")))
			->SetPadding(FMargin(14, 12, 0, 0));
		LockStateText = Label(WidgetTree, NSLOCTEXT("SSTruth", "Locked", "● 잠금 상태"), 18, TEXT("EB8076"));
		UHorizontalBoxSlot* LockSlot = Header->AddChildToHorizontalBox(LockStateText);
		LockSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		LockSlot->SetHorizontalAlignment(HAlign_Right);
		Column->AddChildToVerticalBox(Header)->SetPadding(FMargin(0, 0, 0, 12));

		UHorizontalBox* Main = WidgetTree->ConstructWidget<UHorizontalBox>();
		Column->AddChildToVerticalBox(Main)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		UVerticalBox* Left = WidgetTree->ConstructWidget<UVerticalBox>();
		Main->AddChildToHorizontalBox(Left)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		USizeBox* RightSize = WidgetTree->ConstructWidget<USizeBox>();
		RightSize->SetWidthOverride(280.f);
		Main->AddChildToHorizontalBox(RightSize)->SetPadding(FMargin(14, 0, 0, 0));
		UVerticalBox* Right = WidgetTree->ConstructWidget<UVerticalBox>();
		RightSize->SetContent(Right);

		// 왼쪽 위: 문장 (고정폭)
		UBorder* TextPaper = Panel(WidgetTree, TEXT("08100E"), FMargin(18, 14));
		UVerticalBox* TextColumn = WidgetTree->ConstructWidget<UVerticalBox>();
		TextPaper->SetContent(TextColumn);
		TextColumn->AddChildToVerticalBox(Label(WidgetTree, NSLOCTEXT("SSTruth", "Transmission", "수신된 암호문 / PRIORITY TRANSMISSION"), 16, TEXT("C9A3F0")))
			->SetPadding(FMargin(0, 0, 0, 8));
		CipherText = WidgetTree->ConstructWidget<UTextBlock>();
		CipherText->SetFont(FCoreStyle::GetDefaultFontStyle("Mono", 20));
		CipherText->SetColorAndOpacity(FSlateColor(Color(TEXT("CFE9E4"))));
		CipherText->SetAutoWrapText(true);
		TextColumn->AddChildToVerticalBox(CipherText);
		Left->AddChildToVerticalBox(TextPaper)->SetPadding(FMargin(0, 0, 0, 12));

		// 결과 (잠금 해제 후)
		ResultPanel = Panel(WidgetTree, TEXT("10251E"), FMargin(12, 8));
		ResultPanel->SetVisibility(ESlateVisibility::Collapsed);
		ResultText = Label(WidgetTree, FText::GetEmpty(), 17, TEXT("8FE0A0"));
		ResultPanel->SetContent(ResultText);
		Left->AddChildToVerticalBox(ResultPanel)->SetPadding(FMargin(0, 0, 0, 12));

		// 왼쪽 아래: 치환표 칸들 (StartDecode에서 채움)
		Left->AddChildToVerticalBox(Label(WidgetTree, NSLOCTEXT("SSTruth", "Table", "치환표"), 19, TEXT("EBCB99")))
			->SetPadding(FMargin(0, 0, 0, 8));
		Left->AddChildToVerticalBox(Label(WidgetTree,
										NSLOCTEXT("SSTruth", "TableHelp", "암호 글자 아래 숫자는 등장 횟수 · 칸 선택 후 A–Z 입력 · Backspace 지우기"),
										13, TEXT("9A91A8")))
			->SetPadding(FMargin(0, 0, 0, 8));
		CellBox = WidgetTree->ConstructWidget<UWrapBox>();
		Left->AddChildToVerticalBox(CellBox);

		// 오른쪽: 자동 해독기
		UBorder* SolverPanel = Panel(WidgetTree, TEXT("1A1524"), FMargin(14));
		Right->AddChildToVerticalBox(SolverPanel)->SetPadding(FMargin(0, 0, 0, 12));
		UVerticalBox* SolverColumn = WidgetTree->ConstructWidget<UVerticalBox>();
		SolverPanel->SetContent(SolverColumn);
		SolverColumn->AddChildToVerticalBox(Label(WidgetTree, NSLOCTEXT("SSTruth", "Solver", "자동 해독기"), 19, TEXT("E7D6FA")));
		FitnessText = Label(WidgetTree, FText::GetEmpty(), 36, TEXT("C9A3F0"));
		SolverColumn->AddChildToVerticalBox(FitnessText)->SetPadding(FMargin(0, 10, 0, 6));
		FitnessBar = WidgetTree->ConstructWidget<UProgressBar>();
		FitnessBar->SetFillColorAndOpacity(Color(TEXT("C9A3F0")));
		SolverColumn->AddChildToVerticalBox(FitnessBar)->SetPadding(FMargin(0, 0, 0, 8));
		USizeBox* ChartSize = WidgetTree->ConstructWidget<USizeBox>();
		ChartSize->SetWidthOverride(244.f);
		ChartSize->SetHeightOverride(76.f);
		FitnessChart = WidgetTree->ConstructWidget<UCanvasPanel>();
		ChartSize->SetContent(FitnessChart);
		SolverColumn->AddChildToVerticalBox(ChartSize)->SetPadding(FMargin(0, 2, 0, 9));
		for (int32 Index = 0; Index < 24; ++Index)
		{
			UBorder* Bar = Panel(WidgetTree, TEXT("7D62A0"), FMargin(0));
			UCanvasPanelSlot* BarSlot = FitnessChart->AddChildToCanvas(Bar);
			BarSlot->SetPosition(FVector2D(4.f + Index * 10.f, 70.f));
			BarSlot->SetSize(FVector2D(7.f, 2.f));
			FitnessChartBars.Add(Bar);
		}
		SolverText = Label(WidgetTree, FText::GetEmpty(), 14, TEXT("A7A0B4"));
		SolverColumn->AddChildToVerticalBox(SolverText)->SetPadding(FMargin(0, 0, 0, 8));
		UBorder* LogPanel = Panel(WidgetTree, TEXT("100D16"), FMargin(8, 6));
		USizeBox* LogSize = WidgetTree->ConstructWidget<USizeBox>();
		LogSize->SetHeightOverride(86.f);
		LogSize->SetContent(LogPanel);
		SolverLogText = Label(WidgetTree, NSLOCTEXT("SSTruth", "LogIdle", "해독 기록 대기 중"), 12, TEXT("A7A0B4"));
		LogPanel->SetContent(SolverLogText);
		SolverColumn->AddChildToVerticalBox(LogSize)->SetPadding(FMargin(0, 0, 0, 8));
		SolverColumn->AddChildToVerticalBox(Label(WidgetTree,
			NSLOCTEXT("SSTruth", "SolverHelp", "글자 두 개를 맞바꿔보고, 문장이 더 영어다워지면(글자쌍 점수 상승) 채택합니다."), 13, TEXT("8F889C")));

		SolveButton = Button(WidgetTree, NSLOCTEXT("SSTruth", "Solve", "자동 해독 시작"));
		Right->AddChildToVerticalBox(SolveButton)->SetPadding(FMargin(0, 0, 0, 8));
		CloseButton = Button(WidgetTree, NSLOCTEXT("SSTruth", "Close", "닫기 (진행 저장)"));
		Right->AddChildToVerticalBox(CloseButton);

		// 아래 안내
		StatusText = Label(WidgetTree, FText::GetEmpty(), 17, TEXT("D6C5AB"));
		Column->AddChildToVerticalBox(StatusText)->SetPadding(FMargin(0, 12, 0, 0));
	}
	return Super::RebuildWidget();
}

void USSTruthDecodeWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (FitnessChart && FitnessChartBars.IsEmpty())
	{
		for (int32 Index = 0; Index < 24; ++Index)
		{
			const FName Name(*FString::Printf(TEXT("FitnessHistoryBar_%02d"), Index));
			if (UBorder* Bar = Cast<UBorder>(WidgetTree->FindWidget(Name))) FitnessChartBars.Add(Bar);
		}
	}

	// 키보드 입력을 받으려면 초점을 받을 수 있어야 함
	SetIsFocusable(true);

	if (SolveButton) SolveButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleSolve);
	if (CloseButton) CloseButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleClose);
}

void USSTruthDecodeWidget::NativeDestruct()
{
	// 자동 해독 도중에 닫혀도 결과는 정리해서 저장 (다시 열면 빈칸 채우기부터)
	if (IsValid(Session) && Session->IsSolverRunning()) Session->FinishSolver();

	if (SolveButton) SolveButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleSolve);
	if (CloseButton) CloseButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleClose);
	if (IsValid(Session)) Session->OnTruthChanged.RemoveDynamic(this, &ThisClass::Refresh);
	for (USSCipherCellWidget* Cell : Cells)
	{
		if (IsValid(Cell)) Cell->OnCellClicked.RemoveDynamic(this, &ThisClass::HandleCellClicked);
	}
	Super::NativeDestruct();
}

void USSTruthDecodeWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!IsValid(Session) || !Session->IsSolverRunning()) return;

	// 자동 해독 연출: 매 프레임 몇 걸음씩, 정해진 시간이 지나면 멈춤
	SolverElapsed += InDeltaTime;
	Session->StepSolver(StepsPerFrame);
	if (SolverElapsed >= SolverDuration)
	{
		Session->FinishSolver();
		if (StatusText)
		{
			StatusText->SetText(NSLOCTEXT("SSTruth", "SolverStopped", "자동 해독 한계. 보라색 빈칸을 누르고 키보드로 원래 글자를 넣어."));
		}
	}
}

FReply USSTruthDecodeWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (!IsValid(Session) || !Session->IsSolverDone() || Session->IsUnlocked() || SelectedLetter == INDEX_NONE)
	{
		return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
	}

	const FKey Key = InKeyEvent.GetKey();

	// 지우기
	if (Key == EKeys::BackSpace || Key == EKeys::Delete)
	{
		Session->SetGuess(SelectedLetter, INDEX_NONE);
		return FReply::Handled();
	}

	// A~Z (키 이름이 한 글자인 알파벳 키)
	const FString KeyName = Key.GetFName().ToString();
	if (KeyName.Len() == 1 && FSSCipher::IsLetter(KeyName[0]))
	{
		Session->SetGuess(SelectedLetter, FSSCipher::ToIndex(KeyName[0]));
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void USSTruthDecodeWidget::HandleSolve()
{
	if (!IsValid(Session)) return;
	SolverElapsed = 0.f;
	LastChartTries = -1;
	LastLoggedAccepted = 0;
	FitnessHistory.Reset();
	SolverLogLines.Reset();
	Session->StartSolver();
	if (StatusText) StatusText->SetText(NSLOCTEXT("SSTruth", "Solving", "자동 해독 중… 글자를 맞바꾸며 점수가 오르는 쪽으로 이동"));
}

void USSTruthDecodeWidget::HandleClose()
{
	RemoveFromParent();
}

void USSTruthDecodeWidget::HandleCellClicked(int32 CipherLetter)
{
	if (!IsValid(Session) || !Session->IsSolverDone() || Session->IsUnlocked()) return;

	SelectedLetter = CipherLetter;

	// 키보드 입력이 이 창으로 오게
	SetKeyboardFocus();
	Refresh();
}

void USSTruthDecodeWidget::Refresh()
{
	using namespace SSTruthStyle;
	if (!IsValid(Session)) return;

	const bool bUnlocked = Session->IsUnlocked();
	const bool bRunning = Session->IsSolverRunning();
	const bool bSolverDone = Session->IsSolverDone();

	if (LockStateText)
	{
		LockStateText->SetText(bUnlocked
				? NSLOCTEXT("SSTruth", "UnlockedBadge", "● 잠금 해제")
				: NSLOCTEXT("SSTruth", "Locked", "● 잠금 상태"));
		LockStateText->SetColorAndOpacity(FSlateColor(Color(bUnlocked ? TEXT("8FE0A0") : TEXT("EB8076"))));
	}

	if (CipherText)
	{
		CipherText->SetText(FText::FromString(FormatText(Session->GetShownText())));
		CipherText->SetColorAndOpacity(FSlateColor(Color(bUnlocked ? TEXT("8FE0A0") : (bRunning ? TEXT("C9A3F0") : TEXT("CFE9E4")))));
	}

	// 칸마다 추측·선택 (자동 해독 중엔 입력 막음)
	for (USSCipherCellWidget* Cell : Cells)
	{
		if (!IsValid(Cell)) continue;
		const int32 Index = Cells.IndexOfByKey(Cell);
		const int32 Letter = Session->GetCipherLetters().IsValidIndex(Index) ? Session->GetCipherLetters()[Index] : INDEX_NONE;
		Cell->ShowState(Session->GetGuessFor(Letter), Letter == SelectedLetter, bUnlocked || !bSolverDone);
	}

	const float Fitness = Session->GetFitness();
	if (FitnessBar) FitnessBar->SetPercent(Fitness);
	if (FitnessText) FitnessText->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Fitness * 100.f))));
	if (bRunning && Session->GetSolverTries() != LastChartTries)
	{
		LastChartTries = Session->GetSolverTries();
		FitnessHistory.Add(Fitness);
		if (FitnessHistory.Num() > 24) FitnessHistory.RemoveAt(0);
		if (Session->GetSolverAccepted() > LastLoggedAccepted)
		{
			const int32 Delta = Session->GetSolverAccepted() - LastLoggedAccepted;
			SolverLogLines.Insert(FString::Printf(TEXT("채택 +%d  /  적합도 %d%%"), Delta, FMath::RoundToInt(Fitness * 100.f)), 0);
			if (SolverLogLines.Num() > 4) SolverLogLines.SetNum(4);
			LastLoggedAccepted = Session->GetSolverAccepted();
		}
	}
	for (int32 Index = 0; Index < FitnessChartBars.Num(); ++Index)
	{
		UBorder* Bar = FitnessChartBars[Index];
		if (!IsValid(Bar)) continue;
		const int32 HistoryIndex = Index - (FitnessChartBars.Num() - FitnessHistory.Num());
		const float Height = FitnessHistory.IsValidIndex(HistoryIndex)
			? FMath::Max(3.f, FitnessHistory[HistoryIndex] * 66.f)
			: 2.f;
		if (UCanvasPanelSlot* BarSlot = Cast<UCanvasPanelSlot>(Bar->Slot))
		{
			BarSlot->SetPosition(FVector2D(4.f + Index * 10.f, 72.f - Height));
			BarSlot->SetSize(FVector2D(7.f, Height));
		}
		Bar->SetBrushColor(Color(HistoryIndex >= 0 ? TEXT("C9A3F0") : TEXT("2E2440")));
	}
	if (SolverLogText)
	{
		SolverLogText->SetText(SolverLogLines.IsEmpty()
				? NSLOCTEXT("SSTruth", "LogIdle", "해독 기록 대기 중")
				: FText::FromString(FString::Join(SolverLogLines, TEXT("\n"))));
	}
	if (SolverText)
	{
		SolverText->SetText(FText::Format(NSLOCTEXT("SSTruth", "SolverCount", "시도 {0} · 채택 {1}"),
			Session->GetSolverTries(), Session->GetSolverAccepted()));
	}

	if (SolveButton) SolveButton->SetIsEnabled(!bSolverDone && !bRunning && !bUnlocked);

	if (bUnlocked && !bResultShown)
	{
		bResultShown = true;
		ShowResult();
	}
}

void USSTruthDecodeWidget::ShowResult()
{
	// 해독 성공 처리는 세션이 이미 함 (CommsState::DecodeMessage) → 결과만 표시
	const FSSEventResult& Message = Run->GetComms()->GetLastMessage();
	const FText Korean = Message.Lines.IsEmpty() ? FText::GetEmpty() : Message.Lines[0];

	if (ResultPanel) ResultPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (ResultText)
	{
		ResultText->SetText(FText::Format(NSLOCTEXT("SSTruth", "Result", "[{0}] {1}\n→ 진실 단서 {2}개 확보"),
			Message.Title, Korean, Run->GetComms()->GetTruthCluesFound()));
	}
	if (StatusText) StatusText->SetText(NSLOCTEXT("SSTruth", "Unlocked", "잠금 해제. 기록창에 남겼어."));
	if (UTextBlock* CloseLabel = CloseButton ? Cast<UTextBlock>(CloseButton->GetContent()) : nullptr)
	{
		CloseLabel->SetText(NSLOCTEXT("SSTruth", "Done", "닫기"));
	}
}

FString USSTruthDecodeWidget::FormatText(const FString& Text)
{
	constexpr int32 MaxLineLength = 44;

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
