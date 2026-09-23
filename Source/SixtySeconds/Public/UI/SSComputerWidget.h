#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SSComputerWidget.generated.h"

class USSRunSubsystem;
class USSExpeditionWidget;
class UTextBlock;
class UButton;
class UVerticalBox;
class UScrollBox;

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
    UPROPERTY(Transient) TObjectPtr<USSRunSubsystem> RunSubsystem;
    UPROPERTY(Transient) TSubclassOf<USSExpeditionWidget> ExpeditionClass;
    UPROPERTY(Transient) TObjectPtr<USSExpeditionWidget> ExpeditionWidget;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> DayText;
    UPROPERTY(Transient) TObjectPtr<UVerticalBox> Entries;
    UPROPERTY(Transient) TObjectPtr<UScrollBox> EntryScroll;
    UPROPERTY(Transient) TObjectPtr<UButton> PreviousButton;
    UPROPERTY(Transient) TObjectPtr<UButton> NextButton;
    UPROPERTY(Transient) TObjectPtr<UButton> ExpeditionButton;
    UPROPERTY(Transient) TObjectPtr<UButton> CloseButton;
    int32 ViewedDay = 1;
};
