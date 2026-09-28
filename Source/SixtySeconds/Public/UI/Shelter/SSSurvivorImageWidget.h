#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SSSurvivorImageWidget.generated.h"

class UImage;
class UButton;
class USSSurvivorDefinition;
class USSRunSubsystem;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSSOnSurvivorSelected, FName, SurvivorId);

// One fixed shelter position per identity. No Tick or Event Graph required.
UCLASS()
class SIXTYSECONDS_API USSSurvivorImageWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    FSSOnSurvivorSelected OnSurvivorSelected;

    // Visible character bounds inside the texture; transparent margins do not intercept clicks.
    UPROPERTY(EditAnywhere, Category="SS|Survivor")
    FVector2D ClickAreaMin = FVector2D::ZeroVector;
    UPROPERTY(EditAnywhere, Category="SS|Survivor")
    FVector2D ClickAreaMax = FVector2D(1, 1);
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Survivor")
    TObjectPtr<USSSurvivorDefinition> SurvivorDefinition;

    UPROPERTY(EditAnywhere, Category="SS|Survivor")
    bool bPreviewRescued = true;

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:
    UFUNCTION()
    void OnSurvivorClicked();
    UPROPERTY(Transient)
    TObjectPtr<UButton> SurvivorButton;

    UFUNCTION()
    void RefreshSurvivor();

    UPROPERTY(Transient)
    TObjectPtr<UImage> SurvivorImage;

    UPROPERTY(Transient)
    TObjectPtr<USSRunSubsystem> RunSubsystem;
};
