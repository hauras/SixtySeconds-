#include "UI/Shelter/SSShelterHUD.h"
#include "UI/Exploration/SSExpeditionWidget.h"
#include "UI/Shelter/SSComputerWidget.h"
#include "UI/Shelter/SSRadioWidget.h"
#include "UI/Companion/SSCompanionTalkWidget.h"
#include "Companion/SSCompanionState.h"
#include "UI/Trace/SSTraceWidget.h"
#include "UI/Shelter/Info/SSInfoPanelWidget.h"
#include "UI/Shelter/Info/SSRobotInfoContentWidget.h"
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
#include "UI/Shelter/SSSurvivorImageWidget.h"
#include "UI/Shelter/Info/SSSurvivorInfoContentWidget.h"
#include "Character/SSSurvivorDefinition.h"
#include "Blueprint/WidgetTree.h"
#include "UI/Event/SSEventWidget.h"
#include "Event/SSEventDirector.h"
#include "Event/SSEventCatalog.h"
#include "UI/Ara/SSAraWidget.h"
#include "Components/CanvasPanelSlot.h"

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

    if (RadioButton)
    {
        RadioButton->OnClicked.AddUniqueDynamic(
            this, &USSShelterHUD::OnRadioClicked);
    }

    if (NextDayButton)
    {
        NextDayButton->OnClicked.AddUniqueDynamic(
            this, &USSShelterHUD::OnNextDayClicked);
    }

    if (RobotButton)
        RobotButton->OnClicked.AddUniqueDynamic(this, &USSShelterHUD::OnRobotClicked);
    if (AraButton)
        AraButton->OnClicked.AddUniqueDynamic(this, &ThisClass::OnAraClicked);
    // 밤 버튼은 WBP에 배치 (BindWidgetOptional). 평소엔 숨김
    if (NightEventCueButton)
    {
        NightEventCueButton->OnClicked.AddUniqueDynamic(this, &ThisClass::OnNightEventCueClicked);
        NightEventCueButton->SetVisibility(ESlateVisibility::Collapsed);
    }
    if (NightContinueButton)
    {
        NightContinueButton->OnClicked.AddUniqueDynamic(this, &ThisClass::OnNightContinueClicked);
        NightContinueButton->SetVisibility(ESlateVisibility::Collapsed);
    }
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
        RunSubsystem->OnDayAdvanced.AddUniqueDynamic(this, &ThisClass::HandleDayAdvanced);
        RunSubsystem->OnPlayerStatsChanged.AddUniqueDynamic(this, &ThisClass::HandlePlayerStatsChanged);

        // 사건 카탈로그 연결 (런 동안 한 번. 같은 카탈로그면 다시 색인하지 않음)
        USSEventDirector* Director = RunSubsystem->GetEventDirector();
        if (EventCatalog && Director && Director->GetCatalog() != EventCatalog)
        {
            Director->SetCatalog(EventCatalog);
        }

        // 동료 단서·대사 표 연결
        RunSubsystem->GetCompanions()->SetTables(ClueTable, CompanionLineTable);
    }
    RefreshRobotDisplay();
    RefreshDisplay();
    RefreshAraIndicator();
    if (NightLayer)
    {
        NightLayer->SetRenderOpacity(1.f);
        NightLayer->SetVisibility(ESlateVisibility::Collapsed);   // 낮으로 시작
    }
    if (AraIconImage)
    {
        ScheduleAraBlink();
    }
}

void USSShelterHUD::NativeDestruct()
{
    if (ComputerButton) ComputerButton->OnClicked.RemoveDynamic(this, &ThisClass::OnComputerClicked);
    if (RadioButton) RadioButton->OnClicked.RemoveDynamic(this, &ThisClass::OnRadioClicked);
    if (NextDayButton) NextDayButton->OnClicked.RemoveDynamic(this, &ThisClass::OnNextDayClicked);
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(AraBlinkTimer);
        GetWorld()->GetTimerManager().ClearTimer(AraBlinkRestoreTimer);
        GetWorld()->GetTimerManager().ClearTimer(NightFadeTimer);
    }
    if (IsValid(EventWidget)) EventWidget->OnEventFinished.RemoveDynamic(this, &ThisClass::EndNight);
    if (NightEventCueButton) NightEventCueButton->OnClicked.RemoveDynamic(this, &ThisClass::OnNightEventCueClicked);
    if (NightContinueButton) NightContinueButton->OnClicked.RemoveDynamic(this, &ThisClass::OnNightContinueClicked);
    if (IsValid(RunSubsystem))
    {
        RunSubsystem->OnStoredItemsChanged.RemoveDynamic(this, &USSShelterHUD::RefreshDisplay);
        RunSubsystem->OnRobotStateChanged.RemoveDynamic(this, &USSShelterHUD::RefreshRobotDisplay);
        RunSubsystem->OnActionPointsChanged.RemoveDynamic(this, &USSShelterHUD::RefreshDisplay);
        RunSubsystem->OnSurvivorsChanged.RemoveDynamic(this, &ThisClass::OnSurvivorsUpdated);
        RunSubsystem->OnDayAdvanced.RemoveDynamic(this, &ThisClass::HandleDayAdvanced);
        RunSubsystem->OnPlayerStatsChanged.RemoveDynamic(this, &ThisClass::HandlePlayerStatsChanged);
    }
    if (RobotButton)
        RobotButton->OnClicked.RemoveDynamic(this, &USSShelterHUD::OnRobotClicked);
    if (AraButton)
        AraButton->OnClicked.RemoveDynamic(this, &ThisClass::OnAraClicked);
    TArray<UWidget*> Widgets;
    WidgetTree->GetAllWidgets(Widgets);
    for (UWidget* Widget : Widgets)
        if (USSSurvivorImageWidget* Person = Cast<USSSurvivorImageWidget>(Widget))
            Person->OnSurvivorSelected.RemoveDynamic(this, &ThisClass::OnSurvivorSelected);
    if (IsValid(InfoPanelWidget))
        InfoPanelWidget->RemoveFromParent();
    InfoPanelWidget = nullptr;
    if (IsValid(EventWidget)) EventWidget->RemoveFromParent();
    EventWidget = nullptr;
    if (IsValid(AraPanelWidget)) AraPanelWidget->RemoveFromParent();
    AraPanelWidget = nullptr;
    if (IsValid(ComputerWidget)) ComputerWidget->CloseWindows();
    ComputerWidget = nullptr;
    if (IsValid(RadioWidget)) RadioWidget->CloseWindows();
    RadioWidget = nullptr;
    if (IsValid(TalkWidget)) TalkWidget->RemoveFromParent();
    TalkWidget = nullptr;
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

    // [대화하기] 버튼 (살아 있는 동료만 누를 수 있음)
    UButton* TalkButton = WidgetTree->ConstructWidget<UButton>();
    UTextBlock* TalkLabel = WidgetTree->ConstructWidget<UTextBlock>();
    TalkLabel->SetText(NSLOCTEXT("SS", "SurvivorTalk", "대화하기"));
    TalkButton->SetContent(TalkLabel);
    TalkButton->SetIsEnabled(State->bAlive);
    TalkButton->OnClicked.AddUniqueDynamic(this, &ThisClass::OnTalkClicked);
    InfoPanelWidget->SetPanelAction(TalkButton);
}

void USSShelterHUD::OnTalkClicked()
{
    if (InspectedSurvivorId.IsNone()) return;
    if (IsValid(TalkWidget) && TalkWidget->IsInViewport()) return;
    if (!CompanionTalkWidgetClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("[UI] Assign WBP_CompanionTalk in shelter HUD defaults."));
        return;
    }

    // 정보창을 닫고 대화창을 엶 (대화창을 닫으면 은신처 화면으로 돌아옴)
    TalkWidget = CreateWidget<USSCompanionTalkWidget>(GetOwningPlayer(), CompanionTalkWidgetClass);
    if (!IsValid(TalkWidget)) return;
    if (IsValid(InfoPanelWidget)) InfoPanelWidget->RemoveFromParent();
    TalkWidget->SetSurvivor(InspectedSurvivorId);
    TalkWidget->AddToViewport(20);
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

bool USSShelterHUD::HasOpenTerminalWindow() const
{
	const bool bComputerOpen = IsValid(ComputerWidget)
		&& (ComputerWidget->IsInViewport() || ComputerWidget->HasOpenExpedition());
	const bool bRadioOpen = IsValid(RadioWidget)
		&& (RadioWidget->IsInViewport() || RadioWidget->HasOpenWindow());
	return bComputerOpen || bRadioOpen;
}

void USSShelterHUD::OnComputerClicked()
{
	if (HasOpenTerminalWindow()) return;
	ComputerWidget = CreateWidget<USSComputerWidget>(GetOwningPlayer());
	if (!IsValid(ComputerWidget)) return;
	ComputerWidget->SetExpeditionClass(ExpeditionWidgetClass);
	ComputerWidget->AddToViewport(20);
}

void USSShelterHUD::OnRadioClicked()
{
	if (HasOpenTerminalWindow()) return;
	RadioWidget = CreateWidget<USSRadioWidget>(GetOwningPlayer());
	if (!IsValid(RadioWidget)) return;

	// 역추적 창: 따로 지정하지 않으면 C++에서 구성하는 기본 통신 화면
	const TSubclassOf<USSTraceWidget> ActiveTraceClass = TraceWidgetClass
		? TraceWidgetClass : TSubclassOf<USSTraceWidget>(USSTraceWidget::StaticClass());
	RadioWidget->SetTraceSetup(ActiveTraceClass, TraceConfig);
	RadioWidget->SetTruthDecodeClass(TruthDecodeWidgetClass);
	RadioWidget->AddToViewport(20);
}

void USSShelterHUD::OnNextDayClicked()
{
    if (bNight || !IsValid(RunSubsystem) || !FoodRationCheckBox || !WaterRationCheckBox)   // 밤 동안은 무시
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

    // 날짜·스탯 갱신과 사망 확인은 OnDayAdvanced → HandleDayAdvanced가 처리
    if (NextDayMessageText) NextDayMessageText->SetText(FText::GetEmpty());
}

void USSShelterHUD::HandleDayAdvanced()
{
    if (!IsValid(RunSubsystem)) return;

    CurrentDay = RunSubsystem->GetCurrentDay();

    // 스탯과 날짜·보관 수량을 함께 갱신
    RefreshStats(
        RunSubsystem->GetHealth(),
        RunSubsystem->GetSatiety(),
        RunSubsystem->GetHydration());

    // 다음 날 배급은 다시 선택
    if (FoodRationCheckBox) FoodRationCheckBox->SetIsChecked(false);
    if (WaterRationCheckBox) WaterRationCheckBox->SetIsChecked(false);

    CheckPlayerDeath();
    if (RunSubsystem->GetHealth() > 0.f) BeginNight();   // 넘어가는 사이가 밤. 사건은 밤에, 아라 브리핑은 아침에
}

void USSShelterHUD::RefreshAraIndicator()
{
    const bool bUnread = IsValid(RunSubsystem) && RunSubsystem->HasUnreadAraBriefing();
    const bool bWarning = IsAraWarning();
    if (GetWorld() && GetWorld()->GetTimerManager().IsTimerActive(AraBlinkRestoreTimer))
        RestoreAraBlink();
    UTexture2D* Face = bWarning ? AraWarningIconTexture.Get()
        : bUnread ? AraUnreadIconTexture.Get() : AraIdleIconTexture.Get();
    if (AraIconImage && IsValid(Face))
        AraIconImage->SetBrushFromTexture(Face, false);
    if (AraUnreadText)
        AraUnreadText->SetVisibility(bUnread && (bWarning || !IsValid(AraUnreadIconTexture))
            ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

bool USSShelterHUD::IsAraWarning() const
{
    return IsValid(RunSubsystem) && RunSubsystem->GetHealth() > 0.f
        && (RunSubsystem->GetHealth() <= 25.f || RunSubsystem->GetSatiety() <= 20.f
            || RunSubsystem->GetHydration() <= 20.f);
}

void USSShelterHUD::ScheduleAraBlink()
{
    if (GetWorld() && AraIconImage)
        GetWorld()->GetTimerManager().SetTimer(AraBlinkTimer, this,
            &ThisClass::BlinkAra, FMath::FRandRange(5.f, 7.f), false);
}

void USSShelterHUD::BlinkAra()
{
    if (AraIconImage && IsValid(AraBlinkIconTexture) && IsInViewport() && IsValid(RunSubsystem)
        && !RunSubsystem->HasUnreadAraBriefing() && !IsAraWarning())
    {
        AraIconImage->SetBrushFromTexture(AraBlinkIconTexture, false);
        GetWorld()->GetTimerManager().SetTimer(AraBlinkRestoreTimer, this,
            &ThisClass::RestoreAraBlink, 0.14f, false);
    }
    ScheduleAraBlink();
}

void USSShelterHUD::RestoreAraBlink()
{
    if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(AraBlinkRestoreTimer);
    RefreshAraIndicator();
}

void USSShelterHUD::OnAraClicked()
{
    if (!IsValid(RunSubsystem)) return;
    if (IsValid(AraPanelWidget) && AraPanelWidget->IsInViewport())
    {
        AraPanelWidget->RemoveFromParent();
        return;
    }

    if (!IsValid(AraPanelWidget))
        AraPanelWidget = CreateWidget<USSAraWidget>(GetOwningPlayer());
    if (!IsValid(AraPanelWidget)) return;

    AraPanelWidget->SetBriefing(RunSubsystem->GetAraBriefing());   // 문장 만들기는 세션 몫, HUD는 전달만
    AraPanelWidget->AddToViewport(30);
    RunSubsystem->MarkAraBriefingRead();
    RefreshAraIndicator();
}

void USSShelterHUD::HandlePlayerStatsChanged()
{
    if (!IsValid(RunSubsystem)) return;
    RefreshStats(RunSubsystem->GetHealth(), RunSubsystem->GetSatiety(), RunSubsystem->GetHydration());
    RefreshAraIndicator();
    CheckPlayerDeath();
}

void USSShelterHUD::CheckPlayerDeath()
{
    if (!IsValid(RunSubsystem) || RunSubsystem->GetHealth() > 0.f) return;

    if (NextDayButton)
    {
        NextDayButton->SetIsEnabled(false);
    }

    if (ASSGameMode* ShelterGameMode = Cast<ASSGameMode>(UGameplayStatics::GetGameMode(this)))
    {
        ShelterGameMode->StartDeath();
    }
}

bool USSShelterHUD::TryShowDailyEvent()
{
    if (!IsValid(RunSubsystem) || !EventWidgetClass || PendingNightEventId.IsNone()) return false;
    if (IsValid(EventWidget) && EventWidget->IsInViewport())   // 이미 떠 있음 → 그 창이 끝나길 기다림
    {
        EventWidget->OnEventFinished.AddUniqueDynamic(this, &ThisClass::EndNight);
        return true;
    }

    USSEventDirector* Director = RunSubsystem->GetEventDirector();
    if (!IsValid(Director) || !Director->GetCatalog()) return false;

    EventWidget = CreateWidget<USSEventWidget>(GetOwningPlayer(), EventWidgetClass);
    if (!IsValid(EventWidget)) return false;
    EventWidget->AddToViewport(50);   // 탐사 결과창(40)보다 위
    EventWidget->OnEventFinished.AddUniqueDynamic(this, &ThisClass::EndNight);
    EventWidget->ShowEvent(Director, RunSubsystem, PendingNightEventId);
    PendingNightEventId = NAME_None;
    return true;
}

void USSShelterHUD::OnNightContinueClicked()
{
    if (!bNight || !PendingNightEventId.IsNone()) return;
    if (IsValid(EventWidget) && EventWidget->IsInViewport()) return;
    EndNight();
}

void USSShelterHUD::PlaceNightEventCue(FName EventId)
{
    // 위치는 사건 데이터(Spot)가 정함. HUD는 Spot → 화면 좌표와 글자만 앎
    const USSEventDirector* Director = IsValid(RunSubsystem) ? RunSubsystem->GetEventDirector() : nullptr;
    const FSSEventRow* Row = Director ? Director->FindEvent(EventId) : nullptr;
    const ESSEventSpot Spot = Row ? Row->Spot : ESSEventSpot::Monitor;

    if (NightEventCueButton)
    {
        if (UCanvasPanelSlot* CueSlot = Cast<UCanvasPanelSlot>(NightEventCueButton->Slot))
        {
            if (const FVector2D* Anchor = NightSpotAnchors.Find(Spot))
                CueSlot->SetAnchors(FAnchors(Anchor->X, Anchor->Y));   // 좌표가 없으면 에디터에 둔 자리 그대로
        }
    }

    if (NightEventCueText)
    {
        FText SpotName;
        switch (Spot)
        {
        case ESSEventSpot::Door:      SpotName = NSLOCTEXT("SSNight", "DoorSpot", "문 확인"); break;
        case ESSEventSpot::Vent:      SpotName = NSLOCTEXT("SSNight", "VentSpot", "환풍구 확인"); break;
        case ESSEventSpot::Shelf:     SpotName = NSLOCTEXT("SSNight", "ShelfSpot", "선반 확인"); break;
        case ESSEventSpot::Equipment: SpotName = NSLOCTEXT("SSNight", "EquipmentSpot", "설비 확인"); break;
        case ESSEventSpot::Terminal:  SpotName = NSLOCTEXT("SSNight", "TerminalSpot", "단말 확인"); break;
        case ESSEventSpot::Bed:       SpotName = NSLOCTEXT("SSNight", "BedSpot", "침대 확인"); break;
        default:                      SpotName = NSLOCTEXT("SSNight", "CameraSpot", "감시 화면 확인"); break;
        }
        NightEventCueText->SetText(FText::FromString(TEXT("!")));
        if (NightEventCueButton) NightEventCueButton->SetToolTipText(SpotName);
    }
}

void USSShelterHUD::OnNightEventCueClicked()
{
    if (!bNight || PendingNightEventId.IsNone()) return;
    if (NightEventCueButton) NightEventCueButton->SetVisibility(ESlateVisibility::Collapsed);
    if (!TryShowDailyEvent()) EndNight();
}

void USSShelterHUD::BeginNight()
{
    if (bNight) return;
    bNight = true;
    SetDayControlsEnabled(false);   // 밤엔 낮 행동 금지 (사건 창이 떠 있는 동안 하루가 또 넘어가지 않게)

    if (NightProgressText)
        NightProgressText->SetText(FText::Format(NSLOCTEXT("SSNight", "Progress", "DAY {0}  →  NIGHT  →  DAY {1}"),
            CurrentDay - 1, CurrentDay));   // 하루가 이미 넘어갔으므로 어제 → 오늘
    StartNightFade(true);
}

void USSShelterHUD::StartNightFade(bool bFadeToNight)
{
    if (UWorld* World = GetWorld()) World->GetTimerManager().ClearTimer(NightFadeTimer);
    if (!NightLayer || !GetWorld())
    {
        if (bFadeToNight) ContinueNightAfterIntro();
        else SetDayControlsEnabled(true);
        return;
    }

    bFadingToNight = bFadeToNight;
    NightFadeStartedAt = GetWorld()->GetTimeSeconds();
    NightLayer->SetVisibility(ESlateVisibility::HitTestInvisible);
    NightLayer->SetRenderOpacity(bFadeToNight ? 0.f : 1.f);
    GetWorld()->GetTimerManager().SetTimer(NightFadeTimer, this, &ThisClass::UpdateNightFade, 0.02f, true);
}

void USSShelterHUD::UpdateNightFade()
{
    UWorld* World = GetWorld();
    if (!World || !NightLayer) return;

    const float Duration = bFadingToNight ? NightIntroSeconds : MorningFadeSeconds;
    const float Progress = FMath::Clamp((World->GetTimeSeconds() - NightFadeStartedAt) / FMath::Max(Duration, 0.1f), 0.f, 1.f);
    const float Smoothed = Progress * Progress * (3.f - 2.f * Progress);
    NightLayer->SetRenderOpacity(bFadingToNight ? Smoothed : 1.f - Smoothed);
    if (Progress < 1.f) return;

    World->GetTimerManager().ClearTimer(NightFadeTimer);
    if (bFadingToNight)
    {
        ContinueNightAfterIntro();
    }
    else
    {
        NightLayer->SetVisibility(ESlateVisibility::Collapsed);
        NightLayer->SetRenderOpacity(1.f);
        SetDayControlsEnabled(true);
    }
}

void USSShelterHUD::ContinueNightAfterIntro()
{
    if (!bNight) return;

    USSEventDirector* Director = IsValid(RunSubsystem) ? RunSubsystem->GetEventDirector() : nullptr;
    PendingNightEventId = IsValid(Director) && Director->GetCatalog() && EventWidgetClass
        ? Director->PickEventForToday(*RunSubsystem) : NAME_None;

    if (!PendingNightEventId.IsNone() && NightEventCueButton)
    {
        PlaceNightEventCue(PendingNightEventId);
        NightEventCueButton->SetVisibility(ESlateVisibility::Visible); // 플레이어가 변화를 눌러야 사건을 발견
    }
    else if (!PendingNightEventId.IsNone())
    {
        // 예상과 달리 HUD 루트가 Canvas가 아니면 사건을 조용히 버리지 않는다.
        if (!TryShowDailyEvent()) EndNight();
    }
    else if (NightContinueButton)
    {
        if (NightProgressText)
            NightProgressText->SetText(NSLOCTEXT("SSNight", "QuietNight", "특이사항 없는 밤"));
        NightContinueButton->SetVisibility(ESlateVisibility::Visible);
    }
    else EndNight(); // 버튼 생성 불가 시 진행이 막히지 않도록 복구
}

void USSShelterHUD::EndNight()
{
    if (!bNight) return;   // 델리게이트와 타이머가 겹쳐도 한 번만
    bNight = false;
    PendingNightEventId = NAME_None;
    if (NightEventCueButton) NightEventCueButton->SetVisibility(ESlateVisibility::Collapsed);
    if (NightContinueButton) NightContinueButton->SetVisibility(ESlateVisibility::Collapsed);

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(NightFadeTimer);
    }
    if (IsValid(EventWidget)) EventWidget->OnEventFinished.RemoveDynamic(this, &ThisClass::EndNight);

    // 사건 효과로 죽었으면 사망 처리가 화면을 가져감. 낮 버튼을 다시 풀지 않음
    if (!IsValid(RunSubsystem) || RunSubsystem->GetHealth() <= 0.f) return;

    RunSubsystem->BuildAraBriefing();   // 밤사이 결과(압수 등)까지 반영한 아침 보고
    RefreshAraIndicator();               // 아침이 되어서야 안 읽음 표시

    StartNightFade(false);
}

void USSShelterHUD::SetDayControlsEnabled(bool bEnabled)
{
    // 아라 버튼은 그대로 둠 (밤에도 깨어 있는 AI)
    if (NextDayButton) NextDayButton->SetIsEnabled(bEnabled);
    if (FoodRationCheckBox) FoodRationCheckBox->SetIsEnabled(bEnabled);
    if (WaterRationCheckBox) WaterRationCheckBox->SetIsEnabled(bEnabled);
    if (ComputerButton) ComputerButton->SetIsEnabled(bEnabled);
    if (RadioButton) RadioButton->SetIsEnabled(bEnabled);
    if (RobotButton) RobotButton->SetIsEnabled(bEnabled);
    TArray<UWidget*> Widgets;
    WidgetTree->GetAllWidgets(Widgets);
    for (UWidget* Widget : Widgets)
        if (USSSurvivorImageWidget* Person = Cast<USSSurvivorImageWidget>(Widget))
        {
            Person->SetIsEnabled(bEnabled);
            // 밤에는 "!" 숨김, 아침 페이드가 끝나면 다시 보임
            Person->SetReportMarkAllowed(bEnabled);
        }
}
