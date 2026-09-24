#include "UI/SSRobotInfoContentWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "Item/SSRunSubsystem.h"
#include "Item/SSItemDefinition.h"

namespace SSRobotInfoStyle
{
    FLinearColor Color(const TCHAR* Hex)
    {
        return FLinearColor::FromSRGBColor(FColor::FromHex(Hex));
    }
}

UWidget* USSRobotInfoContentWidget::GetActionWidget() const
{
    return RepairButton;
}

TSharedRef<SWidget> USSRobotInfoContentWidget::RebuildWidget()
{
    if (!WidgetTree->RootWidget)
    {
        UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RobotContentColumn"));
        WidgetTree->RootWidget = Column;
        const auto MakeText = [this](FName Name, const FText& Text, int32 Size, ETextJustify::Type Justification)
        {
            UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
            Label->SetText(Text);
            FSlateFontInfo Font = Label->GetFont();
            Font.Size = Size;
            Label->SetFont(Font);
            Label->SetColorAndOpacity(FSlateColor(SSRobotInfoStyle::Color(TEXT("F3E8D6"))));
            Label->SetJustification(Justification);
            Label->SetAutoWrapText(true);
            return Label;
        };
        StatusText = MakeText(TEXT("StatusText"), FText::GetEmpty(), 24, ETextJustify::Center);
        Column->AddChildToVerticalBox(StatusText)->SetPadding(FMargin(0, 0, 0, 8));
        DescriptionText = MakeText(TEXT("DescriptionText"), FText::GetEmpty(), 18, ETextJustify::Center);
        Column->AddChildToVerticalBox(DescriptionText)->SetPadding(FMargin(0, 0, 0, 16));

        UBorder* ObservationBox = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ObservationBox"));
        ObservationBox->SetBrushColor(SSRobotInfoStyle::Color(TEXT("3B2D21")));
        ObservationBox->SetPadding(FMargin(20, 12));
        UVerticalBox* ObservationColumn = WidgetTree->ConstructWidget<UVerticalBox>();
        ObservationBox->SetContent(ObservationColumn);
        ObservationColumn->AddChildToVerticalBox(MakeText(TEXT("ObservationTitle"), NSLOCTEXT("SS", "ObservationTitle", "관찰"), 22, ETextJustify::Left))->SetPadding(FMargin(0, 0, 0, 8));
        ObservationText = MakeText(TEXT("ObservationText"), NSLOCTEXT("SS", "RobotNormalAppearanceObservation", "외관에 눈에 띄는 변화는 없다."), 20, ETextJustify::Left);
        ObservationColumn->AddChildToVerticalBox(ObservationText);
        Column->AddChildToVerticalBox(ObservationBox)->SetPadding(FMargin(0, 0, 0, 12));

        RepairInfoText = MakeText(TEXT("RepairInfoText"), FText::GetEmpty(), 17, ETextJustify::Center);
        Column->AddChildToVerticalBox(RepairInfoText)->SetPadding(FMargin(0, 0, 0, 10));
        RepairButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("RepairButton"));
        FButtonStyle Style = RepairButton->GetStyle();
        Style.Normal.TintColor = FSlateColor(SSRobotInfoStyle::Color(TEXT("624726")));
        Style.Hovered.TintColor = FSlateColor(SSRobotInfoStyle::Color(TEXT("896335")));
        Style.Pressed.TintColor = FSlateColor(SSRobotInfoStyle::Color(TEXT("47321F")));
        RepairButton->SetStyle(Style);
        RepairButton->SetContent(MakeText(TEXT("RepairLabel"), NSLOCTEXT("SS", "RobotRepair", "수리"), 22, ETextJustify::Center));
        if (UButtonSlot* LabelSlot = Cast<UButtonSlot>(RepairButton->GetContent()->Slot))
            LabelSlot->SetPadding(FMargin(50, 8));
        UVerticalBoxSlot* RepairSlot = Column->AddChildToVerticalBox(RepairButton);
        RepairSlot->SetHorizontalAlignment(HAlign_Center);
    }
    return Super::RebuildWidget();
}

void USSRobotInfoContentWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (UGameInstance* RunGameInstance = GetGameInstance())
        RunSubsystem = RunGameInstance->GetSubsystem<USSRunSubsystem>();
    if (IsValid(RunSubsystem))
    {
        RunSubsystem->OnRobotStateChanged.AddUniqueDynamic(this, &ThisClass::RefreshDisplay);
        RunSubsystem->OnStoredItemsChanged.AddUniqueDynamic(this, &ThisClass::RefreshDisplay);
    }
    if (RepairButton) RepairButton->OnClicked.AddUniqueDynamic(this, &ThisClass::OnRepairClicked);
    RefreshDisplay();
}

void USSRobotInfoContentWidget::NativeDestruct()
{
    if (IsValid(RunSubsystem))
    {
        RunSubsystem->OnRobotStateChanged.RemoveDynamic(this, &ThisClass::RefreshDisplay);
        RunSubsystem->OnStoredItemsChanged.RemoveDynamic(this, &ThisClass::RefreshDisplay);
    }
    if (RepairButton) RepairButton->OnClicked.RemoveDynamic(this, &ThisClass::OnRepairClicked);
    Super::NativeDestruct();
}

void USSRobotInfoContentWidget::RefreshDisplay()
{
    if (!StatusText || !DescriptionText || !RepairInfoText || !RepairButton) return;
    RepairButton->SetIsEnabled(false);
    if (!IsValid(RunSubsystem)) return;
    const int32 KitCount = RunSubsystem->GetStoredQuantityById(SSItemIds::RepairKit);
    FText Status;
    FText Description;
    FText RepairInfo;
    FLinearColor StateColor = SSRobotInfoStyle::Color(TEXT("A9BF87"));
    switch (RunSubsystem->GetRobotState())
    {
    case ESSRobotState::Idle:
        Status = NSLOCTEXT("SS", "RobotIdle", "현재 상태  ·  대기 중");
        Description = NSLOCTEXT("SS", "RobotIdleDescription", "탐사에 파견할 수 있습니다.");
        RepairInfo = NSLOCTEXT("SS", "RobotNoRepair", "정상 작동 중입니다. 수리가 필요하지 않습니다.");
        break;
    case ESSRobotState::Broken:
        StateColor = SSRobotInfoStyle::Color(TEXT("DF9A78"));
        Status = NSLOCTEXT("SS", "RobotBroken", "현재 상태  ·  고장");
        Description = NSLOCTEXT("SS", "RobotBrokenDescription", "수리가 끝나야 탐사에 파견할 수 있습니다.");
        RepairInfo = FText::Format(NSLOCTEXT("SS", "RobotRepairCost", "수리키트 1개 소모 · 보유 {0}개 · 소요 1일"), KitCount);
        RepairButton->SetIsEnabled(KitCount > 0 && RunSubsystem->GetHealth() > 0.f);
        break;
    case ESSRobotState::Repairing:
        Status = NSLOCTEXT("SS", "RobotRepairing", "현재 상태  ·  수리 중");
        Description = NSLOCTEXT("SS", "RobotRepairDescription", "손상된 부품을 복구하고 있습니다.");
        RepairInfo = FText::Format(NSLOCTEXT("SS", "RobotRepairDays", "수리 완료까지 {0}일"), RunSubsystem->GetRemainingRepairDays());
        break;
    case ESSRobotState::Exploring:
        Status = NSLOCTEXT("SS", "RobotExploring", "현재 상태  ·  탐사 중");
        Description = NSLOCTEXT("SS", "RobotExploringDescription", "은신처 밖에서 탐사하고 있습니다.");
        RepairInfo = FText::Format(NSLOCTEXT("SS", "RobotReturnDays", "귀환까지 {0}일"), RunSubsystem->GetRemainingExpeditionDays());
        break;
    }
    StatusText->SetText(Status);
    StatusText->SetColorAndOpacity(FSlateColor(StateColor));
    DescriptionText->SetText(Description);
    RepairInfoText->SetText(RepairInfo);
}

void USSRobotInfoContentWidget::OnRepairClicked()
{
    if (IsValid(RunSubsystem) && RunSubsystem->GetHealth() > 0.f)
        RunSubsystem->RepairRobot();
    RefreshDisplay();
}
