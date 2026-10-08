#include "UI/Shelter/SSSurvivorImageWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Character/SSSurvivorDefinition.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Engine/GameInstance.h"
#include "Item/SSRunSubsystem.h"
#include "Companion/SSCompanionState.h"
#include "Components/TextBlock.h"

TSharedRef<SWidget> USSSurvivorImageWidget::RebuildWidget()
{
    if (!WidgetTree->RootWidget)
    {
        UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
        WidgetTree->RootWidget = Canvas;
        SurvivorImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SurvivorImage"));
        UCanvasPanelSlot* ImageSlot = Canvas->AddChildToCanvas(SurvivorImage);
        ImageSlot->SetAnchors(FAnchors(0, 0, 1, 1));
        ImageSlot->SetOffsets(FMargin(0));
        SurvivorButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SurvivorButton"));
        Canvas->AddChildToCanvas(SurvivorButton)->SetZOrder(1);
        FButtonStyle Style = SurvivorButton->GetStyle();
        Style.Normal.DrawAs = Style.Hovered.DrawAs = Style.Pressed.DrawAs = Style.Disabled.DrawAs = ESlateBrushDrawType::NoDrawType;
        SurvivorButton->SetStyle(Style);
        SurvivorButton->SetCursor(EMouseCursor::Hand);

        // "!" 표시: 클릭을 막지 않게 HitTestInvisible, 위치는 NativePreConstruct에서 머리 위로
        ReportMark = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ReportMark"));
        ReportMark->SetText(FText::FromString(TEXT("!")));
        FSlateFontInfo MarkFont = ReportMark->GetFont();
        MarkFont.Size = 40;
        ReportMark->SetFont(MarkFont);
        ReportMark->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, .78f, .25f)));
        ReportMark->SetShadowOffset(FVector2D(2, 2));
        ReportMark->SetShadowColorAndOpacity(FLinearColor(0, 0, 0, .8f));
        UCanvasPanelSlot* MarkSlot = Canvas->AddChildToCanvas(ReportMark);
        MarkSlot->SetAutoSize(true);
        MarkSlot->SetZOrder(2);
        ReportMark->SetVisibility(ESlateVisibility::Collapsed);
    }
    return Super::RebuildWidget();
}

void USSSurvivorImageWidget::NativePreConstruct()
{
    Super::NativePreConstruct();
    if (SurvivorButton)
    {
        if (UCanvasPanelSlot* ButtonSlot = Cast<UCanvasPanelSlot>(SurvivorButton->Slot))
        {
            ButtonSlot->SetAnchors(FAnchors(ClickAreaMin.X, ClickAreaMin.Y, ClickAreaMax.X, ClickAreaMax.Y));
            ButtonSlot->SetOffsets(FMargin(0));
        }
    }
    if (ReportMark)
    {
        // 클릭 영역 위쪽 가운데에 "!"의 아래 끝을 맞춤
        if (UCanvasPanelSlot* MarkSlot = Cast<UCanvasPanelSlot>(ReportMark->Slot))
        {
            MarkSlot->SetAnchors(FAnchors((ClickAreaMin.X + ClickAreaMax.X) * .5f, ClickAreaMin.Y));
            MarkSlot->SetAlignment(FVector2D(.5f, 1.f));
            MarkSlot->SetPosition(FVector2D::ZeroVector);
        }
    }
    RefreshSurvivor();
}

void USSSurvivorImageWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (SurvivorButton)
        SurvivorButton->OnClicked.AddUniqueDynamic(this, &ThisClass::OnSurvivorClicked);
    if (UGameInstance* Instance = GetGameInstance())
        RunSubsystem = Instance->GetSubsystem<USSRunSubsystem>();
    if (IsValid(RunSubsystem))
        RunSubsystem->OnSurvivorsChanged.AddUniqueDynamic(this, &USSSurvivorImageWidget::RefreshSurvivor);
    RefreshSurvivor();
}

void USSSurvivorImageWidget::NativeDestruct()
{
    if (SurvivorButton)
        SurvivorButton->OnClicked.RemoveDynamic(this, &ThisClass::OnSurvivorClicked);
    if (IsValid(RunSubsystem))
        RunSubsystem->OnSurvivorsChanged.RemoveDynamic(this, &USSSurvivorImageWidget::RefreshSurvivor);
    RunSubsystem = nullptr;
    Super::NativeDestruct();
}

void USSSurvivorImageWidget::RefreshSurvivor()
{
    if (!SurvivorImage) return;
    const FSSSurvivorState* State = !IsDesignTime() && IsValid(RunSubsystem) && IsValid(SurvivorDefinition)
        ? RunSubsystem->FindRescuedSurvivor(SurvivorDefinition->SurvivorId) : nullptr;
    const bool bRescued = IsDesignTime() ? bPreviewRescued : State != nullptr;
    const bool bDead = State && !State->bAlive;
    UTexture2D* Texture = bRescued && IsValid(SurvivorDefinition)
        ? SurvivorDefinition->ShelterImage.Get() : nullptr;
    SurvivorImage->SetBrushFromTexture(Texture, false);
    SurvivorImage->SetVisibility(Texture ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
    // 사망한 동료는 어둡게 남겨두고, 클릭하면 정보창에서 사망 상태를 확인할 수 있다.
    SurvivorImage->SetColorAndOpacity(bDead ? FLinearColor(0.25f, 0.25f, 0.25f, 0.6f) : FLinearColor::White);
    if (SurvivorButton)
    {
        SurvivorButton->SetVisibility(Texture ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
        SurvivorButton->SetToolTipText(IsValid(SurvivorDefinition) ? SurvivorDefinition->DisplayName : FText::GetEmpty());
    }
    if (ReportMark)
    {
        // 낮이고, 살아 있고, 안 들은 보고가 있으면 "!"
        const FSSCompanionRecord* Record = State && State->bAlive
            ? RunSubsystem->GetCompanions()->FindRecord(SurvivorDefinition->SurvivorId) : nullptr;
        const bool bShowMark = bReportMarkAllowed && Record && Record->HasSomethingToSay();
        ReportMark->SetVisibility(bShowMark ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    }
}

void USSSurvivorImageWidget::SetReportMarkAllowed(bool bAllowed)
{
    bReportMarkAllowed = bAllowed;
    RefreshSurvivor();
}

void USSSurvivorImageWidget::OnSurvivorClicked()
{
    if (IsValid(RunSubsystem) && IsValid(SurvivorDefinition)
        && RunSubsystem->IsSurvivorRescued(SurvivorDefinition->SurvivorId))
        OnSurvivorSelected.Broadcast(SurvivorDefinition->SurvivorId);
}
