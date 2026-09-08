#include "UI/SSScrambleTimerWidget.h"
#include "GameMode/SSGameMode.h"
#include "Components/TextBlock.h"

void USSScrambleTimerWidget::InitializeTimer(ASSGameMode* InGameMode)
{
	GameMode = InGameMode;
	DisplayedSeconds = INDEX_NONE;
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
	if (Seconds == DisplayedSeconds) return;
	DisplayedSeconds = Seconds;
	TimerText->SetText(FText::FromString(FString::Printf(TEXT("%02d:%02d"), Seconds / 60, Seconds % 60)));
}
