#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Item/SSRunSubsystem.h"
#include "SSExpeditionWidget.generated.h"

class UTextBlock;
class UButton;
class USSExpeditionDefinition;
class USSRunSubsystem;
class USSExplorationWidget;

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

	// 직접 탐사: 지역 DA에 StealthMap(DirectExplorationMap)이 있을 때만 보임
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> DirectExploreButton;

	// 직접 탐사 화면 WBP (예: WBP_Exploration_SupplyDepot) — Class Defaults에서 지정
	UPROPERTY(EditDefaultsOnly, Category="SS|Exploration")
	TSubclassOf<USSExplorationWidget> ExplorationWidgetClass;

private:
	UFUNCTION() void OnDispatchClicked();
	UFUNCTION() void OnCloseClicked();
	UFUNCTION() void OnDirectExploreClicked();
	UFUNCTION() void OnRobotReturnedHandler(const FSSExpeditionResult& Result);

	UPROPERTY(Transient) TObjectPtr<USSRunSubsystem> RunSubsystem;
	UPROPERTY(Transient) TObjectPtr<USSExplorationWidget> ExplorationWidget;   // 중복으로 여는 것 방지
};
