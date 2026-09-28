#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SSRobotInfoContentWidget.generated.h"

class USSRunSubsystem;
class UTextBlock;
class UButton;

// 공용 정보창의 ContentSlot에 들어가는 로봇 전용 내용.
UCLASS()
class SIXTYSECONDS_API USSRobotInfoContentWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UWidget* GetActionWidget() const;
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
private:
    UFUNCTION() void RefreshDisplay();
    UFUNCTION() void OnRepairClicked();

    UPROPERTY(Transient) TObjectPtr<USSRunSubsystem> RunSubsystem;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> StatusText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> DescriptionText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> ObservationText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> RepairInfoText;
    UPROPERTY(Transient) TObjectPtr<UButton> RepairButton;
};
