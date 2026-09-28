#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SSScrambleTimerWidget.generated.h"

class ASSGameMode;
class UTextBlock;

UCLASS(Abstract)
class SIXTYSECONDS_API USSScrambleTimerWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void InitializeTimer(ASSGameMode* InGameMode);
protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TimerText;
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> GuidanceText;
private:
	void UpdateTimerDisplay();
	UPROPERTY(Transient)
	TObjectPtr<ASSGameMode> GameMode;
	int32 DisplayedSeconds = INDEX_NONE;
	int32 DisplayedShelterState = INDEX_NONE;
};
