#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Item/SSRunSubsystem.h"
#include "Exploration/SSExplorationTypes.h"   // UFUNCTION 매개변수 FSSExplorationResult
#include "SSExpeditionWidget.generated.h"

class UTextBlock;
class UButton;
class UCheckBox;
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

	// 직접 탐사를 가면 그날 하루가 끝나므로 오늘 배급을 출발 전에 정함 (동료 몫은 동료 정보창의 예약 그대로)
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UCheckBox> ExploreFoodCheckBox;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UCheckBox> ExploreWaterCheckBox;

	// 직접 탐사 화면 WBP (예: WBP_Exploration_SupplyDepot) — Class Defaults에서 지정
	UPROPERTY(EditDefaultsOnly, Category="SS|Exploration")
	TSubclassOf<USSExplorationWidget> ExplorationWidgetClass;

private:
	UFUNCTION() void OnDispatchClicked();
	UFUNCTION() void OnCloseClicked();
	UFUNCTION() void OnDirectExploreClicked();
	UFUNCTION() void OnRobotReturnedHandler(const FSSExpeditionResult& Result);
	UFUNCTION() void OnExplorationFinished(const FSSExplorationResult& Result);   // 결과 확인 → 은신처 정산

	UPROPERTY(Transient) TObjectPtr<USSRunSubsystem> RunSubsystem;
	UPROPERTY(Transient) TObjectPtr<USSExplorationWidget> ExplorationWidget;   // 중복으로 여는 것 방지

	// 출발할 때 고른 오늘 배급. 정산 때 그대로 사용
	bool bExploreGiveFood = false;
	bool bExploreGiveWater = false;
};
