#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SSSurvivorImageWidget.generated.h"

class UImage;
class UButton;
class UTextBlock;
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

	// 밤에는 "!"를 숨김 (보고는 아침에 듣는 것). HUD가 낮/밤이 바뀔 때 부름
	void SetReportMarkAllowed(bool bAllowed);

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

	// 머리 위 "!" (어젯밤 조사 보고를 아직 안 들었을 때)
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ReportMark;

	// 지금 "!"를 보여도 되는 때인지 (낮 = true, 밤 = false)
	bool bReportMarkAllowed = true;

	UPROPERTY(Transient)
	TObjectPtr<USSRunSubsystem> RunSubsystem;
};
