

#include "UI/SSSurvivorInfoContentWidget.h"
#include "Engine/GameInstance.h"
#include "Item/SSRunSubsystem.h"
#include "Item/SSItemDefinition.h"
#include "Character/SSSurvivorDefinition.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/CheckBox.h"
#include "Components/Border.h"
#include "Components/Button.h"

UWidget* USSSurvivorInfoContentWidget::GetObservationWidget() const
{
    return ObservationBorder;
}

void USSSurvivorInfoContentWidget::InitSurvivor(FName InSurvivorId)
{
	SurvivorId = InSurvivorId;
	RefreshDisplay();
}

void USSSurvivorInfoContentWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (UGameInstance* RunGameInstance = GetGameInstance())
    {
        RunSubsystem = RunGameInstance->GetSubsystem<USSRunSubsystem>();
    }

    if (IsValid(RunSubsystem))
    {
        RunSubsystem->OnSurvivorsChanged.AddUniqueDynamic(
            this, &ThisClass::RefreshDisplay);

        RunSubsystem->OnStoredItemsChanged.AddUniqueDynamic(
            this, &ThisClass::RefreshDisplay);

        RunSubsystem->OnActionPointsChanged.AddUniqueDynamic(
            this, &ThisClass::RefreshDisplay);
    }

    if (FoodRationCheckBox)
    {
        FoodRationCheckBox->OnCheckStateChanged.AddUniqueDynamic(
            this, &ThisClass::OnFoodRationChanged);
    }

    if (WaterRationCheckBox)
    {
        WaterRationCheckBox->OnCheckStateChanged.AddUniqueDynamic(
            this, &ThisClass::OnWaterRationChanged);
    }

    if (HealButton)
    {
        HealButton->OnClicked.AddUniqueDynamic(
            this, &ThisClass::OnHealClicked);
    }

    RefreshDisplay();
}
void USSSurvivorInfoContentWidget::NativeDestruct()
{
	if (IsValid(RunSubsystem))
	{
		RunSubsystem->OnSurvivorsChanged.RemoveDynamic(this, &ThisClass::RefreshDisplay);
		RunSubsystem->OnStoredItemsChanged.RemoveDynamic(this, &ThisClass::RefreshDisplay);
		RunSubsystem->OnActionPointsChanged.RemoveDynamic(this, &ThisClass::RefreshDisplay);
	}
	if (FoodRationCheckBox)
		FoodRationCheckBox->OnCheckStateChanged.RemoveDynamic(this, &ThisClass::OnFoodRationChanged);
	if (WaterRationCheckBox)
		WaterRationCheckBox->OnCheckStateChanged.RemoveDynamic(this, &ThisClass::OnWaterRationChanged);
	if (HealButton)
		HealButton->OnClicked.RemoveDynamic(this, &ThisClass::OnHealClicked);
	RunSubsystem = nullptr;
	
	Super::NativeDestruct();
}

void USSSurvivorInfoContentWidget::RefreshDisplay()
{
    // 데이터에셋의 초기값이 아니라, 구조된 동료의 현재 상태를 찾는다.
    const FSSSurvivorState* Survivor =
        IsValid(RunSubsystem) ? RunSubsystem->FindRescuedSurvivor(SurvivorId) : nullptr;

    // 세 가지 스탯의 표시 방법이 같으므로 공통으로 처리한다.
    const auto UpdateStat = [](
        UTextBlock* Text,
        UProgressBar* Bar,
        float Value,
        bool bHasData)
    {
        const float ClampedValue = FMath::Clamp(Value, 0.f, 100.f);

        if (Text)
        {
            Text->SetText(
                bHasData
                    ? FText::Format(
                        NSLOCTEXT("SS", "SurvivorStatValue", "{0} / 100"),
                        FText::AsNumber(FMath::RoundToInt(ClampedValue)))
                    : FText::FromString(TEXT("—")));
        }

        if (Bar)
        {
            Bar->SetPercent(bHasData ? ClampedValue / 100.f : 0.f);
        }
    };

    const bool bHasData = Survivor != nullptr;

    UpdateStat(
        HealthText, HealthBar,
        bHasData ? Survivor->Stats.Health : 0.f, bHasData);

    UpdateStat(
        SatietyText, SatietyBar,
        bHasData ? Survivor->Stats.Satiety : 0.f, bHasData);

    UpdateStat(
        HydrationText, HydrationBar,
        bHasData ? Survivor->Stats.Hydration : 0.f, bHasData);

    if (StatusText)
    {
        if (!bHasData)
        {
            StatusText->SetText(
                NSLOCTEXT("SS", "SurvivorNotFound", "동료 정보 없음"));
        }
        else if (!Survivor->bAlive)
        {
            StatusText->SetText(
                NSLOCTEXT("SS", "SurvivorDead", "사망"));
        }
        else
        {
            StatusText->SetText(
                NSLOCTEXT("SS", "SurvivorRescued", "은신처에 머무는 중"));
        }
    }

    if (ObservationText)
    {
        ObservationText->SetText(
            bHasData
                ? NSLOCTEXT("SS", "SurvivorNoObservation", "아직 기록된 관찰 내용이 없습니다.")
                : FText::GetEmpty());
    }

    // 체크박스를 현재 배급 설정과 동기화 (콜백 루프 방지: bAlive일 때만 활성화)
    const bool bAlive = bHasData && Survivor->bAlive;
    if (FoodRationCheckBox)
    {
        FoodRationCheckBox->SetIsEnabled(bAlive);
        FoodRationCheckBox->SetIsChecked(bHasData && Survivor->bGiveFood);
    }
    if (WaterRationCheckBox)
    {
        WaterRationCheckBox->SetIsEnabled(bAlive);
        WaterRationCheckBox->SetIsChecked(bHasData && Survivor->bGiveWater);
    }

    if (HealButton)
    {
        const bool bCanHeal = bAlive
            && Survivor->Stats.Health < 100.f
            && RunSubsystem->GetHealth() > 0.f
            && RunSubsystem->GetStoredQuantityById(SSItemIds::Medkit) > 0
            && RunSubsystem->GetActionPoints() >= USSRunSubsystem::HealActionCost;

        HealButton->SetIsEnabled(bCanHeal);
    }
}

void USSSurvivorInfoContentWidget::OnFoodRationChanged(bool bIsChecked)
{
    if (IsValid(RunSubsystem))
        RunSubsystem->SetSurvivorFoodRation(SurvivorId, bIsChecked);
}

void USSSurvivorInfoContentWidget::OnWaterRationChanged(bool bIsChecked)
{
    if (IsValid(RunSubsystem))
        RunSubsystem->SetSurvivorWaterRation(SurvivorId, bIsChecked);
}

void USSSurvivorInfoContentWidget::OnHealClicked()
{
    if (!IsValid(RunSubsystem) || SurvivorId.IsNone())
    {
        return;
    }

    RunSubsystem->HealSurvivor(SurvivorId);
    RefreshDisplay();
}
