#include "UI/SSShelterHUD.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Item/SSRunSubsystem.h"
#include "Item/SSItemDefinition.h"
#include "Engine/GameInstance.h"
#include "GameMode/SSGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Components/CheckBox.h"

void USSShelterHUD::NativeConstruct()
{
    Super::NativeConstruct();

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        RunSubsystem = GameInstance->GetSubsystem<USSRunSubsystem>();
    }

    if (StorageButton)
    {
        StorageButton->OnClicked.AddUniqueDynamic(
            this, &USSShelterHUD::OnStorageClicked);
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

    RefreshDisplay();
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

    static const FName WaterItemId(TEXT("Water"));
    static const FName FoodItemId(TEXT("Food"));

    if (IsValid(RunSubsystem))
    {
        for (const FSSItemStack& Stack : RunSubsystem->GetStoredItems())
        {
            if (!IsValid(Stack.Item) || Stack.Quantity <= 0)
            {
                continue;
            }

            if (Stack.Item->ItemId == WaterItemId)
            {
                WaterCount += Stack.Quantity;
            }
            else if (Stack.Item->ItemId == FoodItemId)
            {
                FoodCount += Stack.Quantity;
            }
        }
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
}

void USSShelterHUD::OnStorageClicked()
{
    // TODO: 보관함 상세 UI 열기
}

void USSShelterHUD::OnComputerClicked()
{
    // TODO: 탐색 지도 UI 열기
}

void USSShelterHUD::OnNextDayClicked()
{
    if (!IsValid(RunSubsystem) || !FoodRationCheckBox || !WaterRationCheckBox || !FoodRationCheckBox)
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

    if (!RunSubsystem->AdvanceDay(bGiveFood, bGiveWater))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[Shelter] Cannot advance day: dead or insufficient supplies."));
        return;
    }

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