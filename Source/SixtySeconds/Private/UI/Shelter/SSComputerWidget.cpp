#include "UI/Shelter/SSComputerWidget.h"
#include "UI/Exploration/SSExpeditionWidget.h"
#include "UI/Trace/SSTraceWidget.h"
#include "UI/Decode/SSDialDecodeWidget.h"
#include "Trace/SSTraceConfig.h"
#include "Item/SSRunSubsystem.h"
#include "Comms/SSCommsState.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"

namespace SSComputerStyle
{
    FLinearColor Color(const TCHAR* Hex) { return FLinearColor::FromSRGBColor(FColor::FromHex(Hex)); }
    UTextBlock* Label(UWidgetTree* Tree, const FText& Text, int32 Size, const TCHAR* Hex = TEXT("F3E8D6"))
    {
        UTextBlock* Widget = Tree->ConstructWidget<UTextBlock>();
        Widget->SetText(Text);
        FSlateFontInfo Font = Widget->GetFont(); Font.Size = Size;
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
        Widget->SetContent(Label(Tree, Text, 20));
        CastChecked<UButtonSlot>(Widget->GetContent()->Slot)->SetPadding(FMargin(25, 12));
        return Widget;
    }
    FText Category(ESSJournalEvent Event)
    {
        switch (Event)
        {
        case ESSJournalEvent::Deposit: return NSLOCTEXT("SSJournal", "CatDeposit", "물자");
        case ESSJournalEvent::Rations: return NSLOCTEXT("SSJournal", "CatRations", "배급");
        case ESSJournalEvent::Expedition: return NSLOCTEXT("SSJournal", "CatExpedition", "탐사");
        case ESSJournalEvent::Robot: return NSLOCTEXT("SSJournal", "CatRobot", "로봇");
        case ESSJournalEvent::ItemUse: return NSLOCTEXT("SSJournal", "CatItemUse", "사용");
        case ESSJournalEvent::Death: return NSLOCTEXT("SSJournal", "CatDeath", "생존");
        case ESSJournalEvent::Event: return NSLOCTEXT("SSJournal", "CatEvent", "사건");
        case ESSJournalEvent::Signal: return NSLOCTEXT("SSJournal", "CatSignal", "통신");
        default: return NSLOCTEXT("SSJournal", "CatDay", "하루 종료");
        }
    }
}

TSharedRef<SWidget> USSComputerWidget::RebuildWidget()
{
    using namespace SSComputerStyle;
    if (!WidgetTree->RootWidget)
    {
        UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
        WidgetTree->RootWidget = Canvas;
        UBorder* Blocker = WidgetTree->ConstructWidget<UBorder>();
        Blocker->SetBrushColor(FLinearColor(0, 0, 0, .58f));
        UCanvasPanelSlot* BlockerSlot = Canvas->AddChildToCanvas(Blocker);
        BlockerSlot->SetAnchors(FAnchors(0, 0, 1, 1));
        BlockerSlot->SetOffsets(FMargin(0));

        UBorder* Frame = WidgetTree->ConstructWidget<UBorder>();
        Frame->SetBrushColor(Color(TEXT("A77A48")));
        Frame->SetPadding(FMargin(2));
        UCanvasPanelSlot* FrameSlot = Canvas->AddChildToCanvas(Frame);
        FrameSlot->SetAnchors(FAnchors(.5f, .5f));
        FrameSlot->SetAlignment(FVector2D(.5f, .5f));
        FrameSlot->SetPosition(FVector2D::ZeroVector);
        FrameSlot->SetSize(FVector2D(1060, 690));
        UBorder* Body = WidgetTree->ConstructWidget<UBorder>();
        Body->SetBrushColor(Color(TEXT("241C19"))); Body->SetPadding(FMargin(26));
        Frame->SetContent(Body);
        UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(); Body->SetContent(Column);

        UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>();
        UVerticalBox* HeaderTitles = WidgetTree->ConstructWidget<UVerticalBox>();
        HeaderTitles->AddChildToVerticalBox(Label(WidgetTree, NSLOCTEXT("SSJournal", "ArchiveTerminal", "제7연구소  /  B1 관리 단말"), 17, TEXT("C7A176")));
        HeaderTitles->AddChildToVerticalBox(Label(WidgetTree, NSLOCTEXT("SSJournal", "ComputerTitle", "하루 기록"), 36))->SetPadding(FMargin(0, 4, 0, 0));
        Header->AddChildToHorizontalBox(HeaderTitles)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        ExpeditionButton = Button(WidgetTree, NSLOCTEXT("SSJournal", "ExpeditionButton", "탐사 열기"));
        Header->AddChildToHorizontalBox(ExpeditionButton)->SetVerticalAlignment(VAlign_Center);
        TraceButton = Button(WidgetTree, NSLOCTEXT("SSJournal", "TraceButton", "외부 통신"));
        UHorizontalBoxSlot* TraceSlot = Header->AddChildToHorizontalBox(TraceButton);
        TraceSlot->SetVerticalAlignment(VAlign_Center);
        TraceSlot->SetPadding(FMargin(10, 0, 0, 0));
        DecodeButton = Button(WidgetTree, NSLOCTEXT("SSJournal", "DecodeButton", "해독"));
        UHorizontalBoxSlot* DecodeSlot = Header->AddChildToHorizontalBox(DecodeButton);
        DecodeSlot->SetVerticalAlignment(VAlign_Center);
        DecodeSlot->SetPadding(FMargin(10, 0, 0, 0));
        Column->AddChildToVerticalBox(Header)->SetPadding(FMargin(0, 0, 0, 19));

        UHorizontalBox* DayNavigationRow = WidgetTree->ConstructWidget<UHorizontalBox>();
        PreviousButton = Button(WidgetTree, NSLOCTEXT("SSJournal", "PreviousDay", "◀  이전 날"));
        NextButton = Button(WidgetTree, NSLOCTEXT("SSJournal", "NextDay", "다음 날  ▶"));
        DayNavigationRow->AddChildToHorizontalBox(PreviousButton);
        DayText = Label(WidgetTree, FText::GetEmpty(), 25, TEXT("F0D2A4")); DayText->SetJustification(ETextJustify::Center);
        UHorizontalBoxSlot* DaySlot = DayNavigationRow->AddChildToHorizontalBox(DayText);
        DaySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); DaySlot->SetVerticalAlignment(VAlign_Center);
        DayNavigationRow->AddChildToHorizontalBox(NextButton);
        Column->AddChildToVerticalBox(DayNavigationRow)->SetPadding(FMargin(0, 0, 0, 17));

        UBorder* RecordPaper = WidgetTree->ConstructWidget<UBorder>();
        RecordPaper->SetBrushColor(Color(TEXT("D8C9B3")));
        RecordPaper->SetPadding(FMargin(19, 17));
        EntryScroll = WidgetTree->ConstructWidget<UScrollBox>();
        Entries = WidgetTree->ConstructWidget<UVerticalBox>(); EntryScroll->AddChild(Entries);
        RecordPaper->SetContent(EntryScroll);
        UVerticalBoxSlot* ScrollSlot = Column->AddChildToVerticalBox(RecordPaper);
        ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); ScrollSlot->SetPadding(FMargin(0, 0, 0, 17));
        CloseButton = Button(WidgetTree, NSLOCTEXT("SSJournal", "Close", "닫기"));
        Column->AddChildToVerticalBox(CloseButton)->SetHorizontalAlignment(HAlign_Right);
    }
    return Super::RebuildWidget();
}

void USSComputerWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (UGameInstance* RunGameInstance = GetGameInstance()) RunSubsystem = RunGameInstance->GetSubsystem<USSRunSubsystem>();
    if (IsValid(RunSubsystem))
    {
        ViewedDay = RunSubsystem->GetCurrentDay();
        if (!RunSubsystem->GetJournalEntries().IsEmpty()) ViewedDay = RunSubsystem->GetJournalEntries().Last().Day;
        RunSubsystem->OnJournalChanged.AddUniqueDynamic(this, &ThisClass::RefreshJournal);
    }
    PreviousButton->OnClicked.AddUniqueDynamic(this, &ThisClass::PreviousDay);
    NextButton->OnClicked.AddUniqueDynamic(this, &ThisClass::NextDay);
    ExpeditionButton->OnClicked.AddUniqueDynamic(this, &ThisClass::OpenExpedition);
    TraceButton->OnClicked.AddUniqueDynamic(this, &ThisClass::OpenTrace);
    DecodeButton->OnClicked.AddUniqueDynamic(this, &ThisClass::OpenDecode);
    CloseButton->OnClicked.AddUniqueDynamic(this, &ThisClass::CloseComputer);
    ExpeditionButton->SetIsEnabled(ExpeditionClass != nullptr);
    TraceButton->SetIsEnabled(TraceClass != nullptr && IsValid(TraceConfig)
        && IsValid(RunSubsystem) && RunSubsystem->GetComms()->CanStartTrace());

    // 해독: 대기함에 메시지가 있을 때만, 버튼에 대기 개수 표시
    const int32 PendingCount = IsValid(RunSubsystem) ? RunSubsystem->GetComms()->GetPendingMessages().Num() : 0;
    DecodeButton->SetIsEnabled(PendingCount > 0);
    if (UTextBlock* DecodeLabel = Cast<UTextBlock>(DecodeButton->GetContent()))
    {
        DecodeLabel->SetText(FText::Format(NSLOCTEXT("SSJournal", "DecodeButtonCount", "해독 ({0})"), PendingCount));
    }
    RefreshJournal();
}

void USSComputerWidget::NativeDestruct()
{
    if (IsValid(RunSubsystem)) RunSubsystem->OnJournalChanged.RemoveDynamic(this, &ThisClass::RefreshJournal);
    PreviousButton->OnClicked.RemoveDynamic(this, &ThisClass::PreviousDay);
    NextButton->OnClicked.RemoveDynamic(this, &ThisClass::NextDay);
    ExpeditionButton->OnClicked.RemoveDynamic(this, &ThisClass::OpenExpedition);
    TraceButton->OnClicked.RemoveDynamic(this, &ThisClass::OpenTrace);
    DecodeButton->OnClicked.RemoveDynamic(this, &ThisClass::OpenDecode);
    CloseButton->OnClicked.RemoveDynamic(this, &ThisClass::CloseComputer);
    Super::NativeDestruct();
}

void USSComputerWidget::RefreshJournal()
{
    if (!Entries || !IsValid(RunSubsystem)) return;
    ViewedDay = FMath::Clamp(ViewedDay, 1, RunSubsystem->GetCurrentDay());
    DayText->SetText(FText::Format(NSLOCTEXT("SSJournal", "DayTitleWithCurrent", "DAY {0}  /  현재 {1}일차"), ViewedDay, RunSubsystem->GetCurrentDay()));
    PreviousButton->SetIsEnabled(ViewedDay > 1);
    NextButton->SetIsEnabled(ViewedDay < RunSubsystem->GetCurrentDay());
    Entries->ClearChildren();
    for (const FSSJournalEntry& Entry : RunSubsystem->GetJournalEntries())
    {
        if (Entry.Day != ViewedDay) continue;
        UBorder* RowPaper = WidgetTree->ConstructWidget<UBorder>();
        RowPaper->SetBrushColor(SSComputerStyle::Color(TEXT("E9DFD0")));
        RowPaper->SetPadding(FMargin(16, 14));
        UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
        RowPaper->SetContent(Row);
        UTextBlock* Category = SSComputerStyle::Label(WidgetTree, SSComputerStyle::Category(Entry.Event), 19, TEXT("346369"));
        UHorizontalBoxSlot* CategorySlot = Row->AddChildToHorizontalBox(Category);
        CategorySlot->SetPadding(FMargin(0, 0, 18, 0));
        CategorySlot->SetVerticalAlignment(VAlign_Center);
        UTextBlock* Message = SSComputerStyle::Label(WidgetTree, Entry.Message, 21, TEXT("302821"));
        Row->AddChildToHorizontalBox(Message)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        Entries->AddChildToVerticalBox(RowPaper)->SetPadding(FMargin(0, 0, 0, 8));
    }
    if (Entries->GetChildrenCount() == 0)
        Entries->AddChild(SSComputerStyle::Label(WidgetTree, NSLOCTEXT("SSJournal", "Empty", "이 날짜에 기록된 일이 없습니다."), 21, TEXT("53483E")));
}

void USSComputerWidget::PreviousDay() { --ViewedDay; RefreshJournal(); EntryScroll->ScrollToStart(); }
void USSComputerWidget::NextDay() { ++ViewedDay; RefreshJournal(); EntryScroll->ScrollToStart(); }
void USSComputerWidget::CloseComputer() { RemoveFromParent(); }

void USSComputerWidget::OpenExpedition()
{
    if (!ExpeditionClass || HasOpenExpedition()) return;
    ExpeditionWidget = CreateWidget<USSExpeditionWidget>(GetOwningPlayer(), ExpeditionClass);
    if (IsValid(ExpeditionWidget))
    {
        ExpeditionWidget->AddToViewport(20);
        RemoveFromParent();
    }
}

void USSComputerWidget::OpenTrace()
{
    if (!TraceClass || !IsValid(TraceConfig) || HasOpenTrace()) return;
    USSTraceWidget* NewTrace = CreateWidget<USSTraceWidget>(GetOwningPlayer(), TraceClass);
    if (!IsValid(NewTrace) || !NewTrace->StartTrace(TraceConfig)) return;
    TraceWidget = NewTrace;
    TraceWidget->AddToViewport(25);
    RemoveFromParent();
}

void USSComputerWidget::OpenDecode()
{
    if (HasOpenDecode()) return;

    // 대기함 맨 앞(가장 먼저 받은 메시지)부터 풂
    USSDialDecodeWidget* NewDecode = CreateWidget<USSDialDecodeWidget>(GetOwningPlayer(), USSDialDecodeWidget::StaticClass());
    if (!IsValid(NewDecode)) return;

    // 화면이 먼저 만들어져야 다이얼 칸을 채울 수 있어서 AddToViewport 뒤에 시작
    NewDecode->AddToViewport(25);
    if (!NewDecode->StartDecode(0))
    {
        NewDecode->RemoveFromParent();
        return;
    }
    DecodeWidget = NewDecode;
    RemoveFromParent();
}

bool USSComputerWidget::HasOpenDecode() const
{
    return IsValid(DecodeWidget) && DecodeWidget->IsInViewport();
}

bool USSComputerWidget::HasOpenExpedition() const
{
    return IsValid(ExpeditionWidget) && ExpeditionWidget->IsInViewport();
}

bool USSComputerWidget::HasOpenTrace() const
{
    return IsValid(TraceWidget) && TraceWidget->IsInViewport();
}

void USSComputerWidget::CloseWindows()
{
    if (IsValid(ExpeditionWidget)) ExpeditionWidget->RemoveFromParent();
    if (IsValid(TraceWidget)) TraceWidget->RemoveFromParent();
    if (IsValid(DecodeWidget)) DecodeWidget->RemoveFromParent();
    RemoveFromParent();
}

