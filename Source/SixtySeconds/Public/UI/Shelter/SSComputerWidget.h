#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SSComputerWidget.generated.h"

class USSRunSubsystem;
class USSExpeditionWidget;
class USSTraceWidget;
class USSTraceConfig;
class USSDialDecodeWidget;
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
    void SetTraceSetup(TSubclassOf<USSTraceWidget> InClass, USSTraceConfig* InConfig)
    { TraceClass = InClass; TraceConfig = InConfig; }
    bool HasOpenExpedition() const;
    bool HasOpenTrace() const;
    bool HasOpenDecode() const;
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
    UFUNCTION() void OpenTrace();
    UFUNCTION() void OpenDecode();
    UFUNCTION() void CloseComputer();
    UPROPERTY(Transient) TObjectPtr<USSRunSubsystem> RunSubsystem;
    UPROPERTY(Transient) TSubclassOf<USSExpeditionWidget> ExpeditionClass;
    UPROPERTY(Transient) TObjectPtr<USSExpeditionWidget> ExpeditionWidget;
    UPROPERTY(Transient) TSubclassOf<USSTraceWidget> TraceClass;
    UPROPERTY(Transient) TObjectPtr<USSTraceConfig> TraceConfig;
    UPROPERTY(Transient) TObjectPtr<USSTraceWidget> TraceWidget;
    UPROPERTY(Transient) TObjectPtr<USSDialDecodeWidget> DecodeWidget;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> DayText;
    UPROPERTY(Transient) TObjectPtr<UVerticalBox> Entries;
    UPROPERTY(Transient) TObjectPtr<UScrollBox> EntryScroll;
    UPROPERTY(Transient) TObjectPtr<UButton> PreviousButton;
    UPROPERTY(Transient) TObjectPtr<UButton> NextButton;
    UPROPERTY(Transient) TObjectPtr<UButton> ExpeditionButton;
    UPROPERTY(Transient) TObjectPtr<UButton> TraceButton;
    UPROPERTY(Transient) TObjectPtr<UButton> DecodeButton;
    UPROPERTY(Transient) TObjectPtr<UButton> CloseButton;
    int32 ViewedDay = 1;
};
