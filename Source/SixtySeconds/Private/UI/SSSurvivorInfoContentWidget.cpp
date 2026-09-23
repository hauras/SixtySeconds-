

#include "UI/SSSurvivorInfoContentWidget.h"
#include "Engine/GameInstance.h"
#include "Item/SSRunSubsystem.h"
#include "Character/SSSurvivorDefinition.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/Border.h"

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
		RunSubsystem->OnSurvivorsChanged.AddUniqueDynamic(this, &USSSurvivorInfoContentWidget::RefreshDisplay);
	}

	RefreshDisplay();
}

void USSSurvivorInfoContentWidget::NativeDestruct()
{
	if (IsValid(RunSubsystem))
	{
		RunSubsystem->OnSurvivorsChanged.RemoveDynamic(this, &USSSurvivorInfoContentWidget::RefreshDisplay);
	}
	RunSubsystem = nullptr;
	
	Super::NativeDestruct();
}

void USSSurvivorInfoContentWidget::RefreshDisplay()
{
    // 데이터에셋의 초기값이 아니라, 구조된 동료의 현재 상태를 찾는다.
    const FSSSurvivorState* Survivor = nullptr;

    if (IsValid(RunSubsystem) && !SurvivorId.IsNone())
    {
        Survivor = RunSubsystem->GetRescuedSurvivors().FindByPredicate(
            [this](const FSSSurvivorState& State)
            {
                return IsValid(State.Definition)
                    && State.Definition->SurvivorId == SurvivorId;
            });
    }

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
        StatusText->SetText(
            bHasData
                ? NSLOCTEXT("SS", "SurvivorRescued", "은신처에 머무는 중")
                : NSLOCTEXT("SS", "SurvivorNotFound", "동료 정보 없음"));
    }

    if (ObservationText)
    {
        ObservationText->SetText(
            bHasData
                ? NSLOCTEXT(
                    "SS", "SurvivorNoObservation",
                    "아직 기록된 관찰 내용이 없습니다.")
                : FText::GetEmpty());
    }
}
