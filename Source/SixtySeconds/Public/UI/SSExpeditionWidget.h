#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Item/SSRunSubsystem.h"
#include "SSExpeditionWidget.generated.h"

class UTextBlock;
class UButton;
class USSExpeditionDefinition;
class USSRunSubsystem;

UCLASS(Abstract)
class SIXTYSECONDS_API USSExpeditionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void RefreshDisplay();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// 표시하고 파견할 지역 DA — 위젯 BP Class Defaults에서 지정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SS|Expedition")
	TObjectPtr<USSExpeditionDefinition> ExpeditionDefinition;

	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> RegionNameText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> DurationText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> CostText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> RewardsText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> RobotStatusText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> MessageText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> RiskText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton>    DispatchButton;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton>    CloseButton;

private:
	UFUNCTION() void OnDispatchClicked();
	UFUNCTION() void OnCloseClicked();
	UFUNCTION() void OnRobotReturnedHandler(const FSSExpeditionResult& Result);

	UPROPERTY(Transient) TObjectPtr<USSRunSubsystem> RunSubsystem;
};
