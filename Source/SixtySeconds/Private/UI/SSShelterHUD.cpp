#include "UI/SSShelterHUD.h"
#include "UI/SSExpeditionWidget.h"
#include "UI/SSComputerWidget.h"
#include "UI/SSInfoPanelWidget.h"
#include "UI/SSRobotInfoContentWidget.h"
#include "Engine/Texture2D.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Item/SSRunSubsystem.h"
#include "Item/SSItemDefinition.h"
#include "Engine/GameInstance.h"
#include "GameMode/SSGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Components/CheckBox.h"
#include "UI/SSSurvivorImageWidget.h"
#include "UI/SSSurvivorInfoContentWidget.h"
#include "Character/SSSurvivorDefinition.h"
#include "Blueprint/WidgetTree.h"

void USSShelterHUD::NativeConstruct()
{
    Super::NativeConstruct();

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        RunSubsystem = GameInstance->GetSubsystem<USSRunSubsystem>();
    }

    if (ComputerButton)
    {
        ComputerButton->OnClicked.AddUniqueDynamic(
            this, &USSShelterHUD::OnComputerClicked);
    }

    if (NextDayButton)
    {
        NextDayButton->OnClicked.AddUniqueDynamic(
            this, &USSShelterHUD::OnNextDayClicked);
    }

    if (RobotButton)
        RobotButton->OnClicked.AddUniqueDynamic(this, &USSShelterHUD::OnRobotClicked);
    TArray<UWidget*> Widgets;
    WidgetTree->GetAllWidgets(Widgets);
    for (UWidget* Widget : Widgets)
        if (USSSurvivorImageWidget* Person = Cast<USSSurvivorImageWidget>(Widget))
            Person->OnSurvivorSelected.AddUniqueDynamic(this, &ThisClass::OnSurvivorSelected);
    if (IsValid(RunSubsystem))
    {
        RunSubsystem->OnStoredItemsChanged.AddUniqueDynamic(this, &USSShelterHUD::RefreshDisplay);
        RunSubsystem->OnRobotStateChanged.AddUniqueDynamic(this, &USSShelterHUD::RefreshRobotDisplay);
        RunSubsystem->OnActionPointsChanged.AddUniqueDynamic(this, &USSShelterHUD::RefreshDisplay);
        RunSubsystem->OnSurvivorsChanged.AddUniqueDynamic(this, &ThisClass::OnSurvivorsUpdated);
    }
    RefreshRobotDisplay();
    RefreshDisplay();
}

void USSShelterHUD::NativeDestruct()
{
    if (IsValid(RunSubsystem))
    {
        RunSubsystem->OnStoredItemsChanged.RemoveDynamic(this, &USSShelterHUD::RefreshDisplay);
        RunSubsystem->OnRobotStateChanged.RemoveDynamic(this, &USSShelterHUD::RefreshRobotDisplay);
        RunSubsystem->OnActionPointsChanged.RemoveDynamic(this, &USSShelterHUD::RefreshDisplay);
        RunSubsystem->OnSurvivorsChanged.RemoveDynamic(this, &ThisClass::OnSurvivorsUpdated);
    }
    if (RobotButton)
        RobotButton->OnClicked.RemoveDynamic(this, &USSShelterHUD::OnRobotClicked);
    TArray<UWidget*> Widgets;
    WidgetTree->GetAllWidgets(Widgets);
    for (UWidget* Widget : Widgets)
        if (USSSurvivorImageWidget* Person = Cast<USSSurvivorImageWidget>(Widget))
            Person->OnSurvivorSelected.RemoveDynamic(this, &ThisClass::OnSurvivorSelected);
    if (IsValid(InfoPanelWidget))
        InfoPanelWidget->RemoveFromParent();
    InfoPanelWidget = nullptr;
    if (IsValid(ComputerWidget)) ComputerWidget->CloseWindows();
    ComputerWidget = nullptr;
    Super::NativeDestruct();
}

void USSShelterHUD::RefreshRobotDisplay()
{
    const bool bShowRobot = IsValid(RunSubsystem)
        && RunSubsystem->GetRobotState() != ESSRobotState::Exploring;
    if (RobotButton)
        RobotButton->SetVisibility(bShowRobot ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
    if (!bShowRobot && bShowingRobotInfo && IsValid(InfoPanelWidget))
        InfoPanelWidget->RemoveFromParent();
}

void USSShelterHUD::OnRobotClicked()
{
    if (!IsValid(RunSubsystem) || RunSubsystem->GetRobotState() == ESSRobotState::Exploring)
        return;
    if (IsValid(InfoPanelWidget) && InfoPanelWidget->IsInViewport())
        return;
    if (!InfoPanelWidgetClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("[UI] Set InfoPanelWidgetClass in the shelter HUD defaults."));
        return;
    }
    if (!IsValid(InfoPanelWidget))
        InfoPanelWidget = CreateWidget<USSInfoPanelWidget>(GetOwningPlayer(), InfoPanelWidgetClass);
    if (!IsValid(InfoPanelWidget)) return;
    bShowingRobotInfo = true;
    InspectedSurvivorId = NAME_None;

    // Reuse the button's image unless a separate portrait was assigned.
    UTexture2D* Portrait = RobotInfoTexture;
    if (!Portrait && RobotButton)
    {
        if (UImage* ButtonImage = Cast<UImage>(RobotButton->GetContent()))
            Portrait = Cast<UTexture2D>(ButtonImage->GetBrush().GetResourceObject());
    }
    InfoPanelWidget->AddToViewport(20);
    InfoPanelWidget->SetPanelInfo(NSLOCTEXT("SS", "RobotInfoTitle", "탐사로봇"), Portrait);
    if (USSRobotInfoContentWidget* RobotContent = CreateWidget<USSRobotInfoContentWidget>(GetOwningPlayer()))
    {
        InfoPanelWidget->SetPanelContent(RobotContent);
        InfoPanelWidget->SetPanelAction(RobotContent->GetActionWidget());
    }
}

void USSShelterHUD::OnSurvivorSelected(FName SurvivorId)
{
    if (!IsValid(RunSubsystem) || !RunSubsystem->IsSurvivorRescued(SurvivorId)) return;
    if (!InfoPanelWidgetClass || !SurvivorInfoContentClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("[UI] Assign info panel and survivor content classes in shelter HUD defaults."));
        return;
    }
    if (IsValid(InfoPanelWidget) && InfoPanelWidget->IsInViewport()) return;
    const FSSSurvivorState* State = RunSubsystem->FindRescuedSurvivor(SurvivorId);
    if (!State) return;
    if (!IsValid(InfoPanelWidget))
        InfoPanelWidget = CreateWidget<USSInfoPanelWidget>(GetGameInstance(), InfoPanelWidgetClass);
    USSSurvivorInfoContentWidget* Content = CreateWidget<USSSurvivorInfoContentWidget>(GetGameInstance(), SurvivorInfoContentClass);
    if (!IsValid(InfoPanelWidget) || !IsValid(Content)) return;
    bShowingRobotInfo = false;
    InspectedSurvivorId = SurvivorId;
    Content->InitSurvivor(SurvivorId);
    InfoPanelWidget->AddToViewport(20);
    InfoPanelWidget->SetPanelInfo(FText::Format(NSLOCTEXT("SS", "SurvivorPanelTitle", "동료 정보 · {0}"), State->Definition->DisplayName),
        State->Definition->Portrait ? State->Definition->Portrait.Get() : State->Definition->ShelterImage.Get());
    if (!State->Definition->Portrait)
    {
        TArray<UWidget*> Widgets;
        WidgetTree->GetAllWidgets(Widgets);
        for (UWidget* Widget : Widgets)
        {
            const USSSurvivorImageWidget* Person = Cast<USSSurvivorImageWidget>(Widget);
            if (Person && IsValid(Person->SurvivorDefinition) && Person->SurvivorDefinition->SurvivorId == SurvivorId)
            {
                const FVector2D TopHalf(Person->ClickAreaMax.X, Person->ClickAreaMin.Y + (Person->ClickAreaMax.Y - Person->ClickAreaMin.Y) * .5);
                InfoPanelWidget->SetPortraitRegion(Person->ClickAreaMin, TopHalf);
                break;
            }
        }
    }
    InfoPanelWidget->SetPanelContent(Content);
    // Build bindings before retrieving the observation section for the full-width slot.
    Content->TakeWidget();
    InfoPanelWidget->SetPanelObservation(Content->GetObservationWidget());
}

void USSShelterHUD::OnSurvivorsUpdated()
{
    if (!InspectedSurvivorId.IsNone() && IsValid(RunSubsystem)
        && !RunSubsystem->IsSurvivorRescued(InspectedSurvivorId))
    {
        if (IsValid(InfoPanelWidget)) InfoPanelWidget->RemoveFromParent();
        InspectedSurvivorId = NAME_None;
    }
}

void USSShelterHUD::InitHUD(int32 InDay)
{
    CurrentDay = FMath::Max(1, InDay);
    RefreshDisplay();
}

void USSShelterHUD::RefreshStats(
    float Health, float Satiety, float Hydration)
{
    CachedHealth = Health;
    CachedSatiety = Satiety;
    CachedHydration = Hydration;

    RefreshDisplay();
}

void USSShelterHUD::RefreshDisplay()
{
    if (DayText)
    {
        DayText->SetText(FText::Format(
            NSLOCTEXT("SS", "ShelterDay", "Day {0}"),
            CurrentDay));
    }

    if (HealthText)
    {
        HealthText->SetText(FText::Format(
            NSLOCTEXT("SS", "ShelterHealth", "체력 {0}"),
            FMath::RoundToInt(CachedHealth)));
    }

    if (SatietyText)
    {
        SatietyText->SetText(FText::Format(
            NSLOCTEXT("SS", "ShelterSatiety", "포만감 {0}"),
            FMath::RoundToInt(CachedSatiety)));
    }

    if (HydrationText)
    {
        HydrationText->SetText(FText::Format(
            NSLOCTEXT("SS", "ShelterHydration", "수분 {0}"),
            FMath::RoundToInt(CachedHydration)));
    }

    int32 WaterCount = 0;
    int32 FoodCount = 0;
    int32 BatteryCount = 0;

    if (IsValid(RunSubsystem))
    {
        WaterCount   = RunSubsystem->GetStoredQuantityById(SSItemIds::Water);
        FoodCount    = RunSubsystem->GetStoredQuantityById(SSItemIds::Food);
        BatteryCount = RunSubsystem->GetStoredQuantityById(SSItemIds::Battery);
    }

    if (WaterCountText)
    {
        WaterCountText->SetText(FText::Format(
            NSLOCTEXT("SS", "ShelterWaterCount", "물 x{0}"),
            WaterCount));
    }

    if (FoodCountText)
    {
        FoodCountText->SetText(FText::Format(
            NSLOCTEXT("SS", "ShelterFoodCount", "식량 x{0}"),
            FoodCount));
    }

    if (BatteryCountText)
    {
        BatteryCountText->SetText(FText::Format(
            NSLOCTEXT("SS", "ShelterBatteryCount", "배터리 x{0}"),
            BatteryCount));
    }

    if (ActionPointsText && IsValid(RunSubsystem))
    {
        ActionPointsText->SetText(FText::Format(
            NSLOCTEXT("SS", "ShelterAP", "행동력 {0}/{1}"),
            RunSubsystem->GetActionPoints(),
            USSRunSubsystem::MaxActionPoints));
    }

}

void USSShelterHUD::OnComputerClicked()
{
	if (IsValid(ComputerWidget) && (ComputerWidget->IsInViewport() || ComputerWidget->HasOpenExpedition())) return;
	ComputerWidget = CreateWidget<USSComputerWidget>(GetOwningPlayer());
	if (!IsValid(ComputerWidget)) return;
	ComputerWidget->SetExpeditionClass(ExpeditionWidgetClass);
	ComputerWidget->AddToViewport(20);
}

void USSShelterHUD::OnNextDayClicked()
{
    if (!IsValid(RunSubsystem) || !FoodRationCheckBox || !WaterRationCheckBox)
    {
        return;
    }

    ASSGameMode* ShelterGameMode = Cast<ASSGameMode>(UGameplayStatics::GetGameMode(this));

    if (!IsValid(ShelterGameMode)
        || ShelterGameMode->GetCurrentPhase() != ESSGamePhase::Shelter)
    {
        return;
    }

    const bool bGiveFood = FoodRationCheckBox->IsChecked();
    const bool bGiveWater = WaterRationCheckBox->IsChecked();

    int32 RequiredFood = 0;
    int32 RequiredWater = 0;
    RunSubsystem->GetRequiredRations(bGiveFood, bGiveWater, RequiredFood, RequiredWater);
    const int32 StoredFood  = RunSubsystem->GetStoredQuantityById(SSItemIds::Food);
    const int32 StoredWater = RunSubsystem->GetStoredQuantityById(SSItemIds::Water);

    if (StoredFood < RequiredFood || StoredWater < RequiredWater)
    {
        if (NextDayMessageText)
        {
            NextDayMessageText->SetText(FText::Format(
                NSLOCTEXT("SS", "RationShortage", "배급 수량 부족 — 식량 {0}/{1} · 물 {2}/{3}"),
                StoredFood, RequiredFood, StoredWater, RequiredWater));
        }
        return;
    }

    if (!RunSubsystem->AdvanceDay(bGiveFood, bGiveWater))
    {
        if (NextDayMessageText)
            NextDayMessageText->SetText(NSLOCTEXT("SS", "CannotAdvanceDay", "다음 날로 넘어갈 수 없습니다."));
        return;
    }

    if (NextDayMessageText) NextDayMessageText->SetText(FText::GetEmpty());

    CurrentDay = RunSubsystem->GetCurrentDay();

    // 스탯과 날짜·보관 수량을 함께 갱신
    RefreshStats(
        RunSubsystem->GetHealth(),
        RunSubsystem->GetSatiety(),
        RunSubsystem->GetHydration());

    // 다음 날 배급은 다시 선택
    FoodRationCheckBox->SetIsChecked(false);
    WaterRationCheckBox->SetIsChecked(false);

    if (RunSubsystem->GetHealth() <= 0.f)
    {
        if (NextDayButton)
        {
            NextDayButton->SetIsEnabled(false);
        }

        ShelterGameMode->StartDeath();
    }
}
