#include "UI/SSSurvivorImageWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Character/SSSurvivorDefinition.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Engine/GameInstance.h"
#include "Item/SSRunSubsystem.h"

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
    }
    return Super::RebuildWidget();
}

void USSSurvivorImageWidget::NativePreConstruct()
{
    Super::NativePreConstruct();
    if (SurvivorButton)
    {
        UCanvasPanelSlot* ButtonSlot = CastChecked<UCanvasPanelSlot>(SurvivorButton->Slot);
        ButtonSlot->SetAnchors(FAnchors(ClickAreaMin.X, ClickAreaMin.Y, ClickAreaMax.X, ClickAreaMax.Y));
        ButtonSlot->SetOffsets(FMargin(0));
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
    const bool bRescued = IsDesignTime() ? bPreviewRescued
        : IsValid(RunSubsystem) && IsValid(SurvivorDefinition)
            && RunSubsystem->IsSurvivorRescued(SurvivorDefinition->SurvivorId);
    UTexture2D* Texture = bRescued && IsValid(SurvivorDefinition)
        ? SurvivorDefinition->ShelterImage.Get() : nullptr;
    SurvivorImage->SetBrushFromTexture(Texture, false);
    SurvivorImage->SetVisibility(Texture ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
    if (SurvivorButton)
    {
        SurvivorButton->SetVisibility(Texture ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
        SurvivorButton->SetToolTipText(IsValid(SurvivorDefinition) ? SurvivorDefinition->DisplayName : FText::GetEmpty());
    }
}

void USSSurvivorImageWidget::OnSurvivorClicked()
{
    if (IsValid(RunSubsystem) && IsValid(SurvivorDefinition)
        && RunSubsystem->IsSurvivorRescued(SurvivorDefinition->SurvivorId))
        OnSurvivorSelected.Broadcast(SurvivorDefinition->SurvivorId);
}
