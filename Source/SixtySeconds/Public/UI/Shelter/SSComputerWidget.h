#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Item/SSJournalTypes.h"
#include "SSComputerWidget.generated.h"

class USSRunSubsystem;
class USSExpeditionWidget;
class UTextBlock;
class UButton;
class UVerticalBox;
class UScrollBox;
class UBorder;
class UWrapBox;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSSJournalCardClicked, int32, EntryIndex);

UCLASS()
class SIXTYSECONDS_API USSJournalCardWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void Setup(const FSSJournalEntry& Entry, int32 InIndex);
	void SetupNavigation(const FText& Caption, int32 InIndex);
	void SetSelected(bool bSelected);
	bool IsNavigation() const { return bNavigation; }
	FSSJournalCardClicked OnSelected;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION() void SelectCard();
	UPROPERTY(Transient) TObjectPtr<UButton> CardButton;
	FSSJournalEntry Record;
	int32 EntryIndex = INDEX_NONE;
	bool bNavigation = false;
};

UCLASS()
class SIXTYSECONDS_API USSComputerWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void SetExpeditionClass(TSubclassOf<USSExpeditionWidget> InClass) { ExpeditionClass = InClass; }
	bool HasOpenExpedition() const;
	void CloseWindows();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION() void RefreshJournal();
	UFUNCTION() void PreviousDay();
	UFUNCTION() void NextDay();
	UFUNCTION() void OpenExpedition();
	UFUNCTION() void CloseComputer();
	UFUNCTION() void OpenRecord(int32 EntryIndex);
	UFUNCTION() void CloseRecord();
	UFUNCTION() void SelectCategory(int32 CategoryIndex);
	bool MatchesCategory(ESSJournalEvent Event) const;
	UPROPERTY(Transient) TObjectPtr<USSRunSubsystem> RunSubsystem;
	UPROPERTY(Transient) TSubclassOf<USSExpeditionWidget> ExpeditionClass;
	UPROPERTY(Transient) TObjectPtr<USSExpeditionWidget> ExpeditionWidget;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> DayText;
	UPROPERTY(Transient) TObjectPtr<UWrapBox> Entries;
	UPROPERTY(Transient) TObjectPtr<UBorder> DetailLayer;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> DetailTitle;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> DetailMessage;
	UPROPERTY(Transient) TObjectPtr<UButton> DetailCloseButton;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> RecordCountText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> DetailIcon;
	UPROPERTY(Transient) TObjectPtr<UBorder> DetailChoicePanel;
	UPROPERTY(Transient) TObjectPtr<UBorder> DetailOutcomePanel;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> DetailChoiceText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> DetailOutcomeText;
	UPROPERTY(Transient) TArray<TObjectPtr<USSJournalCardWidget>> CategoryButtons;
	UPROPERTY(Transient) TObjectPtr<UScrollBox> EntryScroll;
	UPROPERTY(Transient) TObjectPtr<UButton> PreviousButton;
	UPROPERTY(Transient) TObjectPtr<UButton> NextButton;
	UPROPERTY(Transient) TObjectPtr<UButton> ExpeditionButton;
	UPROPERTY(Transient) TObjectPtr<UButton> CloseButton;
	int32 ViewedDay = 1;
	int32 SelectedCategory = 0;
};
