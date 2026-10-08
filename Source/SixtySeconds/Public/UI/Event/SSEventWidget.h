#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Event/SSEventTypes.h"
#include "SSEventWidget.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;
class UWrapBox;
class USSEventDirector;
class USSRunSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSSOnEventFinished);

// 하루 사건 창. 제목·본문과 선택지 버튼(최대 3개)을 보여주고, 고른 선택지를 디렉터에 넘긴다.
UCLASS()
class SIXTYSECONDS_API USSEventWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ShowEvent(USSEventDirector* InDirector, USSRunSubsystem* InRun, FName InEventId);

	const FSSEventResult& GetResult() const { return Result; }
	UPROPERTY(BlueprintAssignable, Category="SS|Event")
	FSSOnEventFinished OnEventFinished;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> BodyText;

	// 선택지 칸. 사건의 선택지 수보다 많은 칸은 숨김
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> ChoiceButton0;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> ChoiceButton1;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> ChoiceButton2;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> ChoiceText0;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> ChoiceText1;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> ChoiceText2;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UVerticalBox> ResultBox;
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> PickedText;
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> OutcomeText;
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UWrapBox> ChangeList;
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UButton> ConfirmButton;

private:
	friend class FSSEventWidgetResultTest;
	UFUNCTION() void HandleChoice0();
	UFUNCTION() void HandleChoice1();
	UFUNCTION() void HandleChoice2();
	void Choose(int32 Index);
	void ShowResult();
	UFUNCTION()
	void HandleConfirm();
	bool bShowingResult = false;
	bool bFinished = false;

	UPROPERTY(Transient) TObjectPtr<USSEventDirector> Director;
	UPROPERTY(Transient) TObjectPtr<USSRunSubsystem> Run;

	FName EventId = NAME_None;
	TArray<FName> ChoiceIds; // 버튼 칸 번호 → 선택지 ID

	FSSEventResult Result;
};
