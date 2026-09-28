#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SSAraWidget.generated.h"

class UButton;
class UTextBlock;
class USSRunSubsystem;

/** Compact ARA briefing opened from the shelter HUD icon. */
UCLASS()
class SIXTYSECONDS_API USSAraWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetBriefing(const FText& InBriefing);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void CloseBriefing();
	UFUNCTION()
	void AskOutside();
	UFUNCTION()
	void AskPatrol();
	UFUNCTION()
	void AskSurvivors();
	void AskQuestion(int32 QuestionIndex);
	void RefreshQuestions();

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> BriefingText;

	UPROPERTY(Transient)
	TObjectPtr<UButton> CloseButton;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> QuestionButtons;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> AnswerText;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ActionPointText;
	UPROPERTY(Transient)
	TObjectPtr<USSRunSubsystem> RunSubsystem;

	FText Briefing;
};
