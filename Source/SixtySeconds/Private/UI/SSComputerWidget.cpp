#include "UI/SSComputerWidget.h"
#include "UI/SSExpeditionWidget.h"
#include "Item/SSRunSubsystem.h"
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
    UTextBlock* Label(UWidgetTree* Tree, const FText& Text, int32 Size)
    {
        UTextBlock* Widget = Tree->ConstructWidget<UTextBlock>();
        Widget->SetText(Text);
        FSlateFontInfo Font = Widget->GetFont(); Font.Size = Size;
        Widget->SetFont(Font);
        Widget->SetColorAndOpacity(FSlateColor(Color(TEXT("F3E8D6"))));
        Widget->SetAutoWrapText(true);
        return Widget;
    }
    UButton* Button(UWidgetTree* Tree, const FText& Text)
    {
        UButton* Widget = Tree->ConstructWidget<UButton>();
        FButtonStyle Style = Widget->GetStyle();
        Style.Normal.TintColor = FSlateColor(Color(TEXT("624726")));
        Style.Hovered.TintColor = FSlateColor(Color(TEXT("896335")));
        Style.Pressed.TintColor = FSlateColor(Color(TEXT("47321F")));
        Widget->SetStyle(Style);
        Widget->SetContent(Label(Tree, Text, 20));
        CastChecked<UButtonSlot>(Widget->GetContent()->Slot)->SetPadding(FMargin(24, 10));
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
        Blocker->SetBrushColor(FLinearColor(0, 0, 0, .35f));
        UCanvasPanelSlot* BlockerSlot = Canvas->AddChildToCanvas(Blocker);
        BlockerSlot->SetAnchors(FAnchors(0, 0, 1, 1));
        BlockerSlot->SetOffsets(FMargin(0));

        UBorder* Frame = WidgetTree->ConstructWidget<UBorder>();
        Frame->SetBrushColor(Color(TEXT("B58A4A")));
        Frame->SetPadding(FMargin(2));
        UCanvasPanelSlot* FrameSlot = Canvas->AddChildToCanvas(Frame);
        FrameSlot->SetAnchors(FAnchors(.5f, .5f));
        FrameSlot->SetAlignment(FVector2D(.5f, .5f));
        FrameSlot->SetPosition(FVector2D::ZeroVector);
        FrameSlot->SetSize(FVector2D(960, 740));
        UBorder* Body = WidgetTree->ConstructWidget<UBorder>();
        Body->SetBrushColor(Color(TEXT("2B211A"))); Body->SetPadding(FMargin(28));
        Frame->SetContent(Body);
        UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(); Body->SetContent(Column);

        UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>();
        Header->AddChildToHorizontalBox(Label(WidgetTree, NSLOCTEXT("SSJournal", "ComputerTitle", "컴퓨터 · 하루 기록"), 30))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        ExpeditionButton = Button(WidgetTree, NSLOCTEXT("SSJournal", "ExpeditionButton", "탐사 열기"));
        Header->AddChildToHorizontalBox(ExpeditionButton);
        Column->AddChildToVerticalBox(Header)->SetPadding(FMargin(0, 0, 0, 24));

        UHorizontalBox* DayNavigationRow = WidgetTree->ConstructWidget<UHorizontalBox>();
        PreviousButton = Button(WidgetTree, NSLOCTEXT("SSJournal", "PreviousDay", "이전 날"));
        NextButton = Button(WidgetTree, NSLOCTEXT("SSJournal", "NextDay", "다음 날"));
        DayNavigationRow->AddChildToHorizontalBox(PreviousButton);
        DayText = Label(WidgetTree, FText::GetEmpty(), 25); DayText->SetJustification(ETextJustify::Center);
        UHorizontalBoxSlot* DaySlot = DayNavigationRow->AddChildToHorizontalBox(DayText);
        DaySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); DaySlot->SetVerticalAlignment(VAlign_Center);
        DayNavigationRow->AddChildToHorizontalBox(NextButton);
        Column->AddChildToVerticalBox(DayNavigationRow)->SetPadding(FMargin(0, 0, 0, 20));

        EntryScroll = WidgetTree->ConstructWidget<UScrollBox>();
        Entries = WidgetTree->ConstructWidget<UVerticalBox>(); EntryScroll->AddChild(Entries);
        UVerticalBoxSlot* ScrollSlot = Column->AddChildToVerticalBox(EntryScroll);
        ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); ScrollSlot->SetPadding(FMargin(0, 0, 0, 20));
        CloseButton = Button(WidgetTree, NSLOCTEXT("SSJournal", "Close", "닫기"));
        Column->AddChildToVerticalBox(CloseButton)->SetHorizontalAlignment(HAlign_Center);
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
    RefreshJournal();
}

void USSComputerWidget::NativeDestruct()
{
    if (IsValid(RunSubsystem)) RunSubsystem->OnJournalChanged.RemoveDynamic(this, &ThisClass::RefreshJournal);
    PreviousButton->OnClicked.RemoveDynamic(this, &ThisClass::PreviousDay);
    NextButton->OnClicked.RemoveDynamic(this, &ThisClass::NextDay);
    ExpeditionButton->OnClicked.RemoveDynamic(this, &ThisClass::OpenExpedition);
    CloseButton->OnClicked.RemoveDynamic(this, &ThisClass::CloseComputer);
    Super::NativeDestruct();
}

void USSComputerWidget::RefreshJournal()
{
    if (!Entries || !IsValid(RunSubsystem)) return;
    ViewedDay = FMath::Clamp(ViewedDay, 1, RunSubsystem->GetCurrentDay());
    DayText->SetText(FText::Format(NSLOCTEXT("SSJournal", "DayTitleWithCurrent", "{0}일차 기록  ·  현재 {1}일차"), ViewedDay, RunSubsystem->GetCurrentDay()));
    PreviousButton->SetIsEnabled(ViewedDay > 1);
    NextButton->SetIsEnabled(ViewedDay < RunSubsystem->GetCurrentDay());
    Entries->ClearChildren();
    for (const FSSJournalEntry& Entry : RunSubsystem->GetJournalEntries())
    {
        if (Entry.Day != ViewedDay) continue;
        UTextBlock* Row = SSComputerStyle::Label(WidgetTree, FText::Format(NSLOCTEXT("SSJournal", "Entry", "[{0}]  {1}"), SSComputerStyle::Category(Entry.Event), Entry.Message), 21);
        Entries->AddChildToVerticalBox(Row)->SetPadding(FMargin(0, 0, 14, 16));
    }
    if (Entries->GetChildrenCount() == 0)
        Entries->AddChild(SSComputerStyle::Label(WidgetTree, NSLOCTEXT("SSJournal", "Empty", "아직 기록된 일이 없습니다."), 21));
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

bool USSComputerWidget::HasOpenExpedition() const
{
    return IsValid(ExpeditionWidget) && ExpeditionWidget->IsInViewport();
}

void USSComputerWidget::CloseWindows()
{
    if (IsValid(ExpeditionWidget)) ExpeditionWidget->RemoveFromParent();
    RemoveFromParent();
}

