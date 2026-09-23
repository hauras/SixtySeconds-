#include "UI/SSScrambleTimerWidget.h"
#include "GameMode/SSGameMode.h"
#include "Components/TextBlock.h"

void USSScrambleTimerWidget::InitializeTimer(ASSGameMode* InGameMode)
{
	GameMode = InGameMode;
	DisplayedSeconds = INDEX_NONE;
	DisplayedShelterState = INDEX_NONE;
	UpdateTimerDisplay();
}

void USSScrambleTimerWidget::NativeConstruct()
{
	Super::NativeConstruct();
	DisplayedSeconds = INDEX_NONE;
	UpdateTimerDisplay();
}

void USSScrambleTimerWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	UpdateTimerDisplay();
}

void USSScrambleTimerWidget::UpdateTimerDisplay()
{
	if (!TimerText) return;
	if (!IsValid(GameMode))
	{
		TimerText->SetText(FText::GetEmpty());
		DisplayedSeconds = INDEX_NONE;
		return;
	}
	const int32 Seconds = FMath::CeilToInt(FMath::Max(0.f, GameMode->GetScrambleTimeRemaining()));
	const int32 ShelterState = GameMode->IsPlayerInShelter() ? 1 : 0;
	if (GuidanceText && ShelterState != DisplayedShelterState)
	{
		DisplayedShelterState = ShelterState;
		GuidanceText->SetText(ShelterState
			? NSLOCTEXT("SS", "ScrambleInside", "은신처 안 · 시간 종료까지 대기")
			: NSLOCTEXT("SS", "ScrambleReturn", "제한 시간 안에 은신처로 돌아가세요"));
		GuidanceText->SetColorAndOpacity(FSlateColor(FLinearColor::FromSRGBColor(
			ShelterState ? FColor(169, 191, 135) : FColor(243, 232, 214))));
	}
	FLinearColor TimerColor = FLinearColor::FromSRGBColor(Seconds <= 5
		? FColor(242, 103, 87) : Seconds <= 10 ? FColor(245, 178, 83) : FColor(243, 232, 214));
	if (Seconds > 0 && Seconds <= 10)
		TimerColor.A = 0.82f + 0.18f * FMath::Sin(GameMode->GetScrambleTimeRemaining() * 2.f * PI);
	TimerText->SetColorAndOpacity(FSlateColor(TimerColor));
	if (Seconds == DisplayedSeconds) return;
	DisplayedSeconds = Seconds;
	TimerText->SetText(FText::FromString(FString::Printf(TEXT("%02d:%02d"), Seconds / 60, Seconds % 60)));
}
