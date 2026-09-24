

#include "UI/SSInfoPanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/NamedSlot.h"
#include "Engine/Texture2D.h"

void USSInfoPanelWidget::SetPanelContent(UWidget* InContent)
{
	if (ContentSlot) ContentSlot->SetContent(InContent);
}

void USSInfoPanelWidget::SetPanelAction(UWidget* InAction)
{
    if (ActionSlot) ActionSlot->ClearChildren();
	if (ActionSlot && InAction)
	{
		InAction->RemoveFromParent();
		ActionSlot->SetContent(InAction);
	}
}

void USSInfoPanelWidget::SetPanelObservation(UWidget* InObservation)
{
    if (!ObservationSlot) return;
    ObservationSlot->ClearChildren();
    if (InObservation)
    {
        InObservation->RemoveFromParent();
        ObservationSlot->SetContent(InObservation);
    }
}

void USSInfoPanelWidget::SetPortraitRegion(FVector2D UVMin, FVector2D UVMax)
{
    if (!TargetImage) return;
    FSlateBrush Brush = TargetImage->GetBrush();
    if (UTexture2D* Texture = Cast<UTexture2D>(Brush.GetResourceObject()))
    {
        Brush.SetUVRegion(FBox2D(UVMin, UVMax));
        Brush.ImageSize = FVector2D(Texture->GetSizeX() * (UVMax.X - UVMin.X), Texture->GetSizeY() * (UVMax.Y - UVMin.Y));
        TargetImage->SetBrush(Brush);
    }
}

void USSInfoPanelWidget::SetPanelInfo(const FText& InTitle, UTexture2D* InImage)
{
	if (TitleText)
	{
		TitleText->SetText(InTitle);
	}

	if (TargetImage)
	{
		TargetImage->SetBrushFromTexture(InImage, false);
        SetPortraitRegion(FVector2D::ZeroVector, FVector2D(1, 1));
		TargetImage->SetVisibility(
			InImage
				? ESlateVisibility::HitTestInvisible
				: ESlateVisibility::Hidden);
	}
}

void USSInfoPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (CloseButton)
	{
		CloseButton->OnClicked.AddUniqueDynamic(
			this, &USSInfoPanelWidget::OnCloseClicked);
	}
}

void USSInfoPanelWidget::NativeDestruct()
{
	if (CloseButton)
	{
		CloseButton->OnClicked.RemoveDynamic(
			this, &USSInfoPanelWidget::OnCloseClicked);
	}

	Super::NativeDestruct();
}

void USSInfoPanelWidget::OnCloseClicked()
{
	RemoveFromParent();
}
