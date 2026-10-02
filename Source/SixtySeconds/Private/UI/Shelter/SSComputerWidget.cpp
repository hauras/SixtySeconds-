#include "UI/Shelter/SSComputerWidget.h"
#include "UI/Exploration/SSExpeditionWidget.h"
#include "Item/SSRunSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/WrapBox.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
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
    // Symbols are deliberately simple: no external font or texture dependency.
    FText Icon(ESSJournalEvent Event)
    {
        switch (Event)
        {
        case ESSJournalEvent::Event: return FText::FromString(TEXT("!"));
        case ESSJournalEvent::Investigation: return FText::FromString(TEXT("?"));
        case ESSJournalEvent::Signal: return FText::FromString(TEXT("≈"));
        case ESSJournalEvent::Expedition: return FText::FromString(TEXT("↗"));
        case ESSJournalEvent::Rations: return FText::FromString(TEXT("+"));
        default: return FText::FromString(TEXT("≡"));
        }
    }
    UBorder* Panel(UWidgetTree* Tree, const TCHAR* Hex, FMargin Padding)
    {
        auto* Result = Tree->ConstructWidget<UBorder>();
        Result->SetBrushColor(Color(Hex)); Result->SetPadding(Padding);
        return Result;
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
        case ESSJournalEvent::Investigation: return NSLOCTEXT("SSJournal", "CatInvestigation", "조사");
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
        Frame->SetPadding(FMargin(5));
        UCanvasPanelSlot* FrameSlot = Canvas->AddChildToCanvas(Frame);
        FrameSlot->SetAnchors(FAnchors(.08f, .08f, .92f, .92f));
        FrameSlot->SetAlignment(FVector2D::ZeroVector);
        FrameSlot->SetPosition(FVector2D::ZeroVector);
        FrameSlot->SetOffsets(FMargin(0));
        UBorder* Body = WidgetTree->ConstructWidget<UBorder>();
        Body->SetBrushColor(Color(TEXT("101716"))); Body->SetPadding(FMargin(18));
        Frame->SetContent(Body);
        UHorizontalBox* Workspace = WidgetTree->ConstructWidget<UHorizontalBox>();
        Body->SetContent(Workspace);
        USizeBox* SidebarSize = WidgetTree->ConstructWidget<USizeBox>(); SidebarSize->SetWidthOverride(210);
        UBorder* Sidebar = Panel(WidgetTree, TEXT("171B19"), FMargin(12,18)); SidebarSize->SetContent(Sidebar);
        UVerticalBox* SidebarColumn = WidgetTree->ConstructWidget<UVerticalBox>(); Sidebar->SetContent(SidebarColumn);
        SidebarColumn->AddChildToVerticalBox(Label(WidgetTree, NSLOCTEXT("SSJournal","ArchiveMark","B1 / ARCHIVE"),18,TEXT("D5AD74")))->SetPadding(FMargin(0,0,0,8));
        SidebarColumn->AddChildToVerticalBox(Label(WidgetTree, NSLOCTEXT("SSJournal","ArchiveMenu","기록 보관함"),25))->SetPadding(FMargin(0,0,0,25));
        const FText Categories[] = {
            NSLOCTEXT("SSJournal","FilterAll","전체 기록"),
            NSLOCTEXT("SSJournal","FilterEvent","밤 사건"),
            NSLOCTEXT("SSJournal","FilterInvestigation","동료 조사"),
            NSLOCTEXT("SSJournal","FilterExpedition","탐사 기록"),
            NSLOCTEXT("SSJournal","FilterSignal","통신 기록"),
            NSLOCTEXT("SSJournal","FilterSupply","물자 · 배급")};
        for (int32 Index = 0; Index < UE_ARRAY_COUNT(Categories); ++Index)
        {
            auto* MenuButton = CreateWidget<USSJournalCardWidget>(this);
            MenuButton->SetupNavigation(Categories[Index],Index);
            MenuButton->OnSelected.AddUniqueDynamic(this,&ThisClass::SelectCategory);
            CategoryButtons.Add(MenuButton);
            SidebarColumn->AddChildToVerticalBox(MenuButton)->SetPadding(FMargin(0,0,0,9));
        }
        USpacer* MenuSpacer = WidgetTree->ConstructWidget<USpacer>();
        SidebarColumn->AddChildToVerticalBox(MenuSpacer)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        SidebarColumn->AddChildToVerticalBox(Label(WidgetTree,NSLOCTEXT("SSJournal","ArchiveOnline","● 기록 연결 정상"),16,TEXT("7CCAB8")));
        Workspace->AddChildToHorizontalBox(SidebarSize)->SetPadding(FMargin(0,0,24,0));
        UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
        Workspace->AddChildToHorizontalBox(Column)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

        UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>();
        UVerticalBox* HeaderTitles = WidgetTree->ConstructWidget<UVerticalBox>();
        HeaderTitles->AddChildToVerticalBox(Label(WidgetTree, NSLOCTEXT("SSJournal", "ArchiveTerminal", "제7연구소  /  B1 관리 단말"), 17, TEXT("C7A176")));
        HeaderTitles->AddChildToVerticalBox(Label(WidgetTree, NSLOCTEXT("SSJournal", "ComputerTitle", "하루 기록"), 36))->SetPadding(FMargin(0, 4, 0, 0));
        Header->AddChildToHorizontalBox(HeaderTitles)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        ExpeditionButton = Button(WidgetTree, NSLOCTEXT("SSJournal", "ExpeditionButton", "탐사 열기"));
        Header->AddChildToHorizontalBox(ExpeditionButton)->SetVerticalAlignment(VAlign_Center);
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

        RecordCountText = Label(WidgetTree,FText::GetEmpty(),17,TEXT("99B1AA"));
        Column->AddChildToVerticalBox(RecordCountText)->SetPadding(FMargin(0,0,0,12));
        UBorder* RecordPaper = WidgetTree->ConstructWidget<UBorder>();
        RecordPaper->SetBrushColor(Color(TEXT("0B1211")));
        RecordPaper->SetPadding(FMargin(19, 17));
        EntryScroll = WidgetTree->ConstructWidget<UScrollBox>();
        Entries = WidgetTree->ConstructWidget<UWrapBox>(); Entries->SetInnerSlotPadding(FVector2D(14,14)); EntryScroll->AddChild(Entries);
        RecordPaper->SetContent(EntryScroll);
        UVerticalBoxSlot* ScrollSlot = Column->AddChildToVerticalBox(RecordPaper);
        ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); ScrollSlot->SetPadding(FMargin(0, 0, 0, 17));
        CloseButton = Button(WidgetTree, NSLOCTEXT("SSJournal", "Close", "닫기"));
        Column->AddChildToVerticalBox(CloseButton)->SetHorizontalAlignment(HAlign_Right);
        DetailLayer = WidgetTree->ConstructWidget<UBorder>();
        DetailLayer->SetBrushColor(FLinearColor(0,0,0,.42f));
        DetailLayer->SetPadding(FMargin(0));
        auto* LayerSlot = Canvas->AddChildToCanvas(DetailLayer);
        LayerSlot->SetAnchors(FAnchors(0,0,1,1)); LayerSlot->SetOffsets(FMargin(0)); LayerSlot->SetZOrder(10);
        UCanvasPanel* DetailCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(); DetailLayer->SetContent(DetailCanvas);
        UBorder* Rim = WidgetTree->ConstructWidget<UBorder>(); Rim->SetBrushColor(Color(TEXT("A77A48"))); Rim->SetPadding(FMargin(5));
        auto* RimSlot = DetailCanvas->AddChildToCanvas(Rim); RimSlot->SetAnchors(FAnchors(.31f,.12f,.78f,.88f)); RimSlot->SetOffsets(FMargin(0));
        UBorder* DetailBody = WidgetTree->ConstructWidget<UBorder>(); DetailBody->SetBrushColor(Color(TEXT("151C19"))); DetailBody->SetPadding(FMargin(28)); Rim->SetContent(DetailBody);
        UVerticalBox* DetailColumn = WidgetTree->ConstructWidget<UVerticalBox>(); DetailBody->SetContent(DetailColumn);
        DetailColumn->AddChildToVerticalBox(Label(WidgetTree, NSLOCTEXT("SSJournal","DetailCaption","제7연구소 / 기록 상세"),17,TEXT("7BC9C8")));
        DetailIcon = Label(WidgetTree,FText::GetEmpty(),48,TEXT("D5AD74"));
        DetailColumn->AddChildToVerticalBox(DetailIcon)->SetPadding(FMargin(0,12,0,0));
        DetailTitle = Label(WidgetTree,FText::GetEmpty(),30);
        DetailColumn->AddChildToVerticalBox(DetailTitle)->SetPadding(FMargin(0,12,0,22));
        UScrollBox* DetailScroll = WidgetTree->ConstructWidget<UScrollBox>();
        UBorder* ReadingPanel = Panel(WidgetTree,TEXT("202923"),FMargin(20));
        UVerticalBox* ReadingColumn = WidgetTree->ConstructWidget<UVerticalBox>(); ReadingPanel->SetContent(ReadingColumn);
        ReadingColumn->AddChildToVerticalBox(Label(WidgetTree,NSLOCTEXT("SSJournal","RecordBodyHeading","발생 기록"),18,TEXT("D5AD74")))->SetPadding(FMargin(0,0,0,15));
        DetailMessage = Label(WidgetTree,FText::GetEmpty(),23); ReadingColumn->AddChildToVerticalBox(DetailMessage);
        DetailScroll->AddChild(ReadingPanel);
        const auto AddSection = [&](const FText& Heading, UBorder*& OutPanel, UTextBlock*& OutText)
        {
            OutPanel = Panel(WidgetTree,TEXT("262B22"),FMargin(20));
            UVerticalBox* Section = WidgetTree->ConstructWidget<UVerticalBox>(); OutPanel->SetContent(Section);
            Section->AddChildToVerticalBox(Label(WidgetTree,Heading,18,TEXT("D5AD74")))->SetPadding(FMargin(0,0,0,12));
            OutText = Label(WidgetTree,FText::GetEmpty(),22); Section->AddChildToVerticalBox(OutText);
            DetailScroll->AddChild(OutPanel);
        };
        UBorder* ChoicePanel = nullptr; UTextBlock* ChoiceText = nullptr;
        AddSection(NSLOCTEXT("SSJournal","YourChoice","당신의 선택"),ChoicePanel,ChoiceText);
        DetailChoicePanel = ChoicePanel; DetailChoiceText = ChoiceText;
        UBorder* OutcomePanel = nullptr; UTextBlock* OutcomeText = nullptr;
        AddSection(NSLOCTEXT("SSJournal","YourOutcome","결과 · 실제 변화"),OutcomePanel,OutcomeText);
        DetailOutcomePanel = OutcomePanel; DetailOutcomeText = OutcomeText;
        DetailColumn->AddChildToVerticalBox(DetailScroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        DetailColumn->AddChildToVerticalBox(Label(WidgetTree,NSLOCTEXT("SSJournal","StoredCaption","당시 기록 · 컴퓨터 기록에 보관됨"),16,TEXT("7BC9C8")))->SetPadding(FMargin(0,20,0,12));
        DetailCloseButton = Button(WidgetTree,NSLOCTEXT("SSJournal","BackToRecords","기록 목록으로"));
        DetailColumn->AddChildToVerticalBox(DetailCloseButton)->SetHorizontalAlignment(HAlign_Right);
        DetailLayer->SetVisibility(ESlateVisibility::Collapsed);
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
    CloseButton->OnClicked.AddUniqueDynamic(this, &ThisClass::CloseComputer);
    ExpeditionButton->SetIsEnabled(ExpeditionClass != nullptr);
    DetailCloseButton->OnClicked.AddUniqueDynamic(this, &ThisClass::CloseRecord);
    CloseRecord();
    RefreshJournal();
}

void USSComputerWidget::NativeDestruct()
{
    if (IsValid(RunSubsystem)) RunSubsystem->OnJournalChanged.RemoveDynamic(this, &ThisClass::RefreshJournal);
    PreviousButton->OnClicked.RemoveDynamic(this, &ThisClass::PreviousDay);
    NextButton->OnClicked.RemoveDynamic(this, &ThisClass::NextDay);
    ExpeditionButton->OnClicked.RemoveDynamic(this, &ThisClass::OpenExpedition);
    CloseButton->OnClicked.RemoveDynamic(this, &ThisClass::CloseComputer);
    DetailCloseButton->OnClicked.RemoveDynamic(this, &ThisClass::CloseRecord);
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
    const auto& Journal = RunSubsystem->GetJournalEntries();
    for (int32 Index = 0; Index < Journal.Num(); ++Index)
    {
        if (Journal[Index].Day != ViewedDay || !MatchesCategory(Journal[Index].Event)) continue;
        auto* Card = CreateWidget<USSJournalCardWidget>(this);
        if (!Card) continue;
        Card->Setup(Journal[Index], Index);
        Card->OnSelected.AddUniqueDynamic(this, &ThisClass::OpenRecord);
        Entries->AddChild(Card);
    }
    RecordCountText->SetText(FText::Format(NSLOCTEXT("SSJournal","RecordCount","보관된 기록 {0}건 · 카드를 눌러 상세 내용을 확인하세요"),Entries->GetChildrenCount()));
    for (int32 Index = 0; Index < CategoryButtons.Num(); ++Index)
        CategoryButtons[Index]->SetSelected(Index == SelectedCategory);
    if (Entries->GetChildrenCount() == 0)
        Entries->AddChild(SSComputerStyle::Label(WidgetTree, NSLOCTEXT("SSJournal", "Empty", "이 날짜에 기록된 일이 없습니다."), 21, TEXT("B2C9C8")));
}

void USSComputerWidget::PreviousDay() { CloseRecord(); --ViewedDay; RefreshJournal(); EntryScroll->ScrollToStart(); }
void USSComputerWidget::NextDay() { CloseRecord(); ++ViewedDay; RefreshJournal(); EntryScroll->ScrollToStart(); }
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

bool USSComputerWidget::HasOpenExpedition() const
{
    return IsValid(ExpeditionWidget) && ExpeditionWidget->IsInViewport();
}

void USSComputerWidget::CloseWindows()
{
    if (IsValid(ExpeditionWidget)) ExpeditionWidget->RemoveFromParent();
    RemoveFromParent();
}


void USSComputerWidget::OpenRecord(int32 EntryIndex)
{
    if (!IsValid(RunSubsystem) || !DetailLayer) return;
    const auto& Journal = RunSubsystem->GetJournalEntries();
    if (!Journal.IsValidIndex(EntryIndex)) return;
    const auto& Entry = Journal[EntryIndex];
    DetailTitle->SetText(FText::Format(NSLOCTEXT("SSJournal","DetailHeading","DAY {0} · {1}"),Entry.Day,
        Entry.Title.IsEmpty() ? SSComputerStyle::Category(Entry.Event) : Entry.Title));
    DetailIcon->SetText(SSComputerStyle::Icon(Entry.Event));
    DetailMessage->SetText(Entry.Body.IsEmpty() ? Entry.Message : Entry.Body);
    DetailChoiceText->SetText(Entry.Choice);
    DetailChoicePanel->SetVisibility(Entry.Choice.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    DetailOutcomeText->SetText(FText::Join(FText::FromString(TEXT("\n\n")),TArray<FText>{Entry.Outcome,Entry.Changes}));
    DetailOutcomePanel->SetVisibility(Entry.Outcome.IsEmpty() && Entry.Changes.IsEmpty()
        ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    DetailLayer->SetVisibility(ESlateVisibility::Visible);
    DetailCloseButton->SetKeyboardFocus();
}
void USSComputerWidget::CloseRecord() { if (DetailLayer) DetailLayer->SetVisibility(ESlateVisibility::Collapsed); }
void USSJournalCardWidget::Setup(const FSSJournalEntry& Entry, int32 InIndex) { Record = Entry; EntryIndex = InIndex; }
TSharedRef<SWidget> USSJournalCardWidget::RebuildWidget()
{
    using namespace SSComputerStyle;
    if (!WidgetTree->RootWidget)
    {
        USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(); Size->SetWidthOverride(bNavigation ? 182 : 250); Size->SetHeightOverride(bNavigation ? 54 : 240); WidgetTree->RootWidget = Size;
        CardButton = Button(WidgetTree,FText::GetEmpty()); Size->SetContent(CardButton);
        FButtonStyle CardStyle = CardButton->GetStyle();
        CardStyle.Normal.TintColor = FSlateColor(Color(TEXT("202B26")));
        CardStyle.Hovered.TintColor = FSlateColor(Color(TEXT("394D40")));
        CardStyle.Pressed.TintColor = FSlateColor(Color(TEXT("142729")));
        CardButton->SetStyle(CardStyle);
        UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(); CardButton->SetContent(Content);
        CastChecked<UButtonSlot>(Content->Slot)->SetPadding(bNavigation ? FMargin(14,10) : FMargin(18));
        if (bNavigation)
        {
            Content->AddChildToVerticalBox(Label(WidgetTree,Record.Message,19));
        }
        else
        {
            Content->AddChildToVerticalBox(Label(WidgetTree,FText::Format(NSLOCTEXT("SSJournal","CardDay","DAY {0} · {1}"),Record.Day,Category(Record.Event)),16,TEXT("92DEDD")));
            auto* CardIcon = Label(WidgetTree,Icon(Record.Event),42,TEXT("D5AD74"));
            CardIcon->SetJustification(ETextJustify::Center);
            Content->AddChildToVerticalBox(CardIcon)->SetPadding(FMargin(0,8,0,4));
            FString Preview = Record.Title.IsEmpty() ? Record.Message.ToString() : Record.Title.ToString();
            if (Preview.Len()>26) Preview = Preview.Left(26)+TEXT("…");
            USizeBox* PreviewBox = WidgetTree->ConstructWidget<USizeBox>();
            PreviewBox->SetHeightOverride(78); PreviewBox->SetClipping(EWidgetClipping::ClipToBounds);
            UTextBlock* PreviewText = Label(WidgetTree,FText::FromString(Preview),17);
            PreviewText->SetWrapTextAt(208); PreviewBox->SetContent(PreviewText);
            Content->AddChildToVerticalBox(PreviewBox)->SetPadding(FMargin(0,4,0,8));
            Content->AddChildToVerticalBox(Label(WidgetTree,NSLOCTEXT("SSJournal","OpenCard","기록 열기 →"),15,TEXT("F0D2A4")));
        }
    }
    return Super::RebuildWidget();
}
void USSJournalCardWidget::NativeConstruct() { Super::NativeConstruct(); if(CardButton) CardButton->OnClicked.AddUniqueDynamic(this,&ThisClass::SelectCard); }
void USSJournalCardWidget::NativeDestruct() { if(CardButton) CardButton->OnClicked.RemoveDynamic(this,&ThisClass::SelectCard); Super::NativeDestruct(); }
void USSJournalCardWidget::SelectCard() { OnSelected.Broadcast(EntryIndex); }
void USSJournalCardWidget::SetupNavigation(const FText& Caption, int32 InIndex)
{
    bNavigation = true; Record.Message = Caption; EntryIndex = InIndex;
}
void USSJournalCardWidget::SetSelected(bool bSelected)
{
    if (!CardButton) return;
    FButtonStyle Style = CardButton->GetStyle();
    Style.Normal.TintColor = FSlateColor(SSComputerStyle::Color(bSelected ? TEXT("745536") : TEXT("202B26")));
    CardButton->SetStyle(Style);
}
bool USSComputerWidget::MatchesCategory(ESSJournalEvent Event) const
{
    switch (SelectedCategory)
    {
    case 1: return Event == ESSJournalEvent::Event;
    case 2: return Event == ESSJournalEvent::Investigation;
    case 3: return Event == ESSJournalEvent::Expedition || Event == ESSJournalEvent::Robot;
    case 4: return Event == ESSJournalEvent::Signal;
    case 5: return Event == ESSJournalEvent::Deposit || Event == ESSJournalEvent::Rations || Event == ESSJournalEvent::ItemUse;
    default: return true;
    }
}
void USSComputerWidget::SelectCategory(int32 CategoryIndex)
{
    if (!CategoryButtons.IsValidIndex(CategoryIndex)) return;
    SelectedCategory = CategoryIndex;
    CloseRecord(); RefreshJournal(); EntryScroll->ScrollToStart();
}
