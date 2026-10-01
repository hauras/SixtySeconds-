#include "UI/Shelter/SSRadioWidget.h"
#include "UI/Trace/SSTraceWidget.h"
#include "UI/Decode/SSDialDecodeWidget.h"
#include "UI/Decode/SSTruthDecodeWidget.h"
#include "Trace/SSTraceConfig.h"
#include "Comms/SSCommsState.h"
#include "Item/SSRunSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"

namespace SSRadioStyle
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
		Style.Normal.TintColor = FSlateColor(Color(TEXT("16201D")));
		Style.Hovered.TintColor = FSlateColor(Color(TEXT("24403A")));
		Style.Pressed.TintColor = FSlateColor(Color(TEXT("0E1614")));
		Widget->SetStyle(Style);
		Widget->SetContent(Label(Tree, Text, 20, TEXT("CFE9E4")));
		CastChecked<UButtonSlot>(Widget->GetContent()->Slot)->SetPadding(FMargin(22, 12));
		return Widget;
	}
}

TSharedRef<SWidget> USSRadioWidget::RebuildWidget()
{
	using namespace SSRadioStyle;
	if (!WidgetTree->RootWidget)
	{
		UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
		WidgetTree->RootWidget = Canvas;

		// 뒤 화면 살짝 어둡게
		UBorder* Blocker = WidgetTree->ConstructWidget<UBorder>();
		Blocker->SetBrushColor(FLinearColor(0, 0, 0, .45f));
		UCanvasPanelSlot* BlockerSlot = Canvas->AddChildToCanvas(Blocker);
		BlockerSlot->SetAnchors(FAnchors(0, 0, 1, 1));
		BlockerSlot->SetOffsets(FMargin(0));

		// 하늘색 테두리의 작은 메뉴 (컴퓨터 화면과 구분)
		UBorder* Frame = WidgetTree->ConstructWidget<UBorder>();
		Frame->SetBrushColor(Color(TEXT("3F8F97")));
		Frame->SetPadding(FMargin(2));
		UCanvasPanelSlot* FrameSlot = Canvas->AddChildToCanvas(Frame);
		FrameSlot->SetAnchors(FAnchors(.5f, .5f));
		FrameSlot->SetAlignment(FVector2D(.5f, .5f));
		FrameSlot->SetSize(FVector2D(460, 400));

		UBorder* Body = WidgetTree->ConstructWidget<UBorder>();
		Body->SetBrushColor(Color(TEXT("0B1210")));
		Body->SetPadding(FMargin(26, 22));
		Frame->SetContent(Body);

		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
		Body->SetContent(Column);

		Column->AddChildToVerticalBox(Label(WidgetTree, NSLOCTEXT("SSRadio", "Place", "B1  /  비상 수신기"), 16, TEXT("7ACCC9")));
		Column->AddChildToVerticalBox(Label(WidgetTree, NSLOCTEXT("SSRadio", "Title", "무전기"), 32, TEXT("F2E4D0")))
			->SetPadding(FMargin(0, 4, 0, 2));
		Column->AddChildToVerticalBox(Label(WidgetTree, NSLOCTEXT("SSRadio", "NoLink", "● 아라 네트워크 미연결"), 15, TEXT("8FE0A0")))
			->SetPadding(FMargin(0, 0, 0, 18));

		TraceButton = Button(WidgetTree, NSLOCTEXT("SSRadio", "Trace", "외부 통신"));
		Column->AddChildToVerticalBox(TraceButton)->SetPadding(FMargin(0, 0, 0, 10));
		DecodeButton = Button(WidgetTree, NSLOCTEXT("SSRadio", "Decode", "해독"));
		Column->AddChildToVerticalBox(DecodeButton)->SetPadding(FMargin(0, 0, 0, 10));

		StatusText = Label(WidgetTree, FText::GetEmpty(), 14, TEXT("A7BCB4"));
		UVerticalBoxSlot* StatusSlot = Column->AddChildToVerticalBox(StatusText);
		StatusSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

		CloseButton = Button(WidgetTree, NSLOCTEXT("SSRadio", "Close", "닫기"));
		Column->AddChildToVerticalBox(CloseButton)->SetHorizontalAlignment(HAlign_Right);
	}
	return Super::RebuildWidget();
}

void USSRadioWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (UGameInstance* GameInstance = GetGameInstance()) RunSubsystem = GameInstance->GetSubsystem<USSRunSubsystem>();

	TraceButton->OnClicked.AddUniqueDynamic(this, &ThisClass::OpenTrace);
	DecodeButton->OnClicked.AddUniqueDynamic(this, &ThisClass::OpenDecode);
	CloseButton->OnClicked.AddUniqueDynamic(this, &ThisClass::CloseRadio);

	USSCommsState* Comms = IsValid(RunSubsystem) ? RunSubsystem->GetComms() : nullptr;

	// 외부 통신: 하루 1회, 행동력 1
	const bool bCanTrace = TraceClass != nullptr && IsValid(TraceConfig) && Comms && Comms->CanStartTrace();
	TraceButton->SetIsEnabled(bCanTrace);

	// 해독: 대기함에 메시지가 있을 때만, 버튼에 대기 개수 표시
	const int32 PendingCount = Comms ? Comms->GetPendingMessages().Num() : 0;
	DecodeButton->SetIsEnabled(PendingCount > 0);
	if (UTextBlock* DecodeLabel = Cast<UTextBlock>(DecodeButton->GetContent()))
	{
		const bool bTruthPending = PendingCount > 0
			&& Comms->GetPendingMessages()[0].Row.Kind == ESSTraceMessageKind::Truth;
		DecodeLabel->SetText(bTruthPending
			? FText::Format(NSLOCTEXT("SSRadio", "TruthDecodeCount", "감청 해독 ({0})"), PendingCount)
			: FText::Format(NSLOCTEXT("SSRadio", "DialDecodeCount", "다이얼 해독 ({0})"), PendingCount));
	}

	// 왜 못 쓰는지 안내
	if (StatusText)
	{
		FText Status = NSLOCTEXT("SSRadio", "Ready", "주파수를 맞춰 바깥 통신을 잡을 수 있어. 신호를 보낼 때마다 로봇 센서에 잡힐 위험이 있어.");
		if (!bCanTrace && Comms && !Comms->CanStartTrace())
		{
			Status = NSLOCTEXT("SSRadio", "TraceUsed", "오늘은 더 접속할 수 없어 (하루 1회, 행동력 1).");
		}
		StatusText->SetText(Status);
	}
}

void USSRadioWidget::NativeDestruct()
{
	TraceButton->OnClicked.RemoveDynamic(this, &ThisClass::OpenTrace);
	DecodeButton->OnClicked.RemoveDynamic(this, &ThisClass::OpenDecode);
	CloseButton->OnClicked.RemoveDynamic(this, &ThisClass::CloseRadio);
	Super::NativeDestruct();
}

void USSRadioWidget::CloseRadio()
{
	RemoveFromParent();
}

void USSRadioWidget::OpenTrace()
{
	if (!TraceClass || !IsValid(TraceConfig) || HasOpenWindow()) return;
	USSTraceWidget* NewTrace = CreateWidget<USSTraceWidget>(GetOwningPlayer(), TraceClass);
	if (!IsValid(NewTrace) || !NewTrace->StartTrace(TraceConfig)) return;
	TraceWidget = NewTrace;
	TraceWidget->AddToViewport(25);
	RemoveFromParent();
}

void USSRadioWidget::OpenDecode()
{
	if (HasOpenWindow() || !IsValid(RunSubsystem)) return;

	// 대기함 맨 앞(가장 먼저 받은 메시지)부터 풂
	const TArray<FSSPendingMessage>& PendingList = RunSubsystem->GetComms()->GetPendingMessages();
	if (PendingList.IsEmpty()) return;
	const bool bTruth = PendingList[0].Row.Kind == ESSTraceMessageKind::Truth;

	// 화면이 먼저 만들어져야 칸을 채울 수 있어서 AddToViewport 뒤에 시작
	// 진실 단서 = 치환 암호 창, 실용 정보 = 다이얼 창
	UUserWidget* NewDecode = nullptr;
	bool bStarted = false;
	if (bTruth)
	{
		const TSubclassOf<USSTruthDecodeWidget> ActiveClass = TruthDecodeClass
			? TruthDecodeClass : TSubclassOf<USSTruthDecodeWidget>(USSTruthDecodeWidget::StaticClass());
		USSTruthDecodeWidget* Truth = CreateWidget<USSTruthDecodeWidget>(GetOwningPlayer(), ActiveClass);
		if (!IsValid(Truth)) return;
		Truth->AddToViewport(25);
		bStarted = Truth->StartDecode(0);
		NewDecode = Truth;
	}
	else
	{
		USSDialDecodeWidget* Dial = CreateWidget<USSDialDecodeWidget>(GetOwningPlayer(), USSDialDecodeWidget::StaticClass());
		if (!IsValid(Dial)) return;
		Dial->AddToViewport(25);
		bStarted = Dial->StartDecode(0);
		NewDecode = Dial;
	}

	if (!bStarted)
	{
		NewDecode->RemoveFromParent();
		return;
	}
	DecodeWidget = NewDecode;
	RemoveFromParent();
}

bool USSRadioWidget::HasOpenWindow() const
{
	return (IsValid(TraceWidget) && TraceWidget->IsInViewport())
		|| (IsValid(DecodeWidget) && DecodeWidget->IsInViewport());
}

void USSRadioWidget::CloseWindows()
{
	if (IsValid(TraceWidget)) TraceWidget->RemoveFromParent();
	if (IsValid(DecodeWidget)) DecodeWidget->RemoveFromParent();
	RemoveFromParent();
}
