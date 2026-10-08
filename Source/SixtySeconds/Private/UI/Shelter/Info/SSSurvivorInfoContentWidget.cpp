

#include "UI/Shelter/Info/SSSurvivorInfoContentWidget.h"
#include "Engine/GameInstance.h"
#include "Item/SSRunSubsystem.h"
#include "Item/SSItemDefinition.h"
#include "Character/SSSurvivorDefinition.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/CheckBox.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Companion/SSCompanionState.h"

UWidget* USSSurvivorInfoContentWidget::GetObservationWidget() const
{
	return ObservationBorder;
}

void USSSurvivorInfoContentWidget::InitSurvivor(FName InSurvivorId)
{
	SurvivorId = InSurvivorId;

	// 다른 동료로 바뀌면 격리 확인 상태를 풀어 둠 (첫 클릭에 바로 격리되지 않게)
	bIsolateArmed = false;
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

	if (InspectButton)
	{
		InspectButton->OnClicked.AddUniqueDynamic(
			this, &ThisClass::OnInspectClicked);
	}

	if (IsolateButton)
	{
		IsolateButton->OnClicked.AddUniqueDynamic(
			this, &ThisClass::OnIsolateClicked);
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
	if (InspectButton)
		InspectButton->OnClicked.RemoveDynamic(this, &ThisClass::OnInspectClicked);
	if (IsolateButton)
		IsolateButton->OnClicked.RemoveDynamic(this, &ThisClass::OnIsolateClicked);
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

	// 관찰 칸: 검사한 적이 있으면 그 결과 (그날의 결과일 뿐, 그 뒤에 바뀌었을 수 있음)
	const USSCompanionState* Companions = IsValid(RunSubsystem) ? RunSubsystem->GetCompanions() : nullptr;
	const FSSCompanionRecord* Record = Companions ? Companions->FindRecord(SurvivorId) : nullptr;
	if (ObservationText)
	{
		FText Observation = bHasData
			? NSLOCTEXT("SS", "SurvivorNoObservation", "아직 기록된 관찰 내용이 없습니다.")
			: FText::GetEmpty();
		if (Record && Record->InspectedDay > 0)
		{
			Observation = FText::Format(
				NSLOCTEXT("SS", "SurvivorInspected", "{0}일째 검사: {1}"),
				Record->InspectedDay,
				USSCompanionState::GetInspectionText(Record->bInspectedAndroid));
		}
		ObservationText->SetText(Observation);
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
		const bool bCanHeal = bAlive && Survivor->Stats.Health < 100.f && RunSubsystem->GetHealth() > 0.f && RunSubsystem->GetStoredQuantityById(SSItemIds::Medkit) > 0 && RunSubsystem->GetActionPoints() >= USSRunSubsystem::HealActionCost;

		HealButton->SetIsEnabled(bCanHeal);
	}

	// 검사: 행동력·배터리가 있을 때만
	if (InspectButton)
	{
		InspectButton->SetIsEnabled(bAlive && Companions && Companions->CanInspect(SurvivorId));
	}

	// 격리: 살아 있을 때만. 확인 중이면 문구가 바뀜
	if (IsolateButton)
	{
		IsolateButton->SetIsEnabled(bAlive);
		if (UTextBlock* Label = Cast<UTextBlock>(IsolateButton->GetContent()))
		{
			Label->SetText(bIsolateArmed
					? NSLOCTEXT("SS", "IsolateConfirm", "정말 격리 (되돌릴 수 없음)")
					: NSLOCTEXT("SS", "Isolate", "격리"));
		}
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

void USSSurvivorInfoContentWidget::OnInspectClicked()
{
	if (!IsValid(RunSubsystem) || SurvivorId.IsNone()) return;

	bIsolateArmed = false;
	RunSubsystem->GetCompanions()->InspectCompanion(SurvivorId);
	RefreshDisplay();
}

void USSSurvivorInfoContentWidget::OnIsolateClicked()
{
	if (!IsValid(RunSubsystem) || SurvivorId.IsNone()) return;

	// 첫 번째 누름: 확인 문구로 바꾸기만 함
	if (!bIsolateArmed)
	{
		bIsolateArmed = true;
		RefreshDisplay();
		return;
	}

	// 두 번째 누름: 격리 (정보창은 동료가 은신처에서 사라지면 HUD가 닫음)
	bIsolateArmed = false;
	bool bWasAndroid = false;
	RunSubsystem->GetCompanions()->IsolateCompanion(SurvivorId, bWasAndroid);
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
