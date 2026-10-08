
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Exploration/SSExplorationTypes.h"
#include "SSExplorationWidget.generated.h"

class UButton;
class UTextBlock;
class USSExplorationSession;
class USSExplorationRoomWidget;
class USSExplorationMapDefinition;
class USSExplorationResultWidget;

// 탐사가 끝나고 결과를 확인했을 때 한 번 방송. 출발시킨 쪽(탐사 창)이 받아서 은신처 정산
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSSOnExplorationFinished, const FSSExplorationResult&, Result);

UCLASS()
class SIXTYSECONDS_API USSExplorationWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	bool StartExploration(USSExplorationMapDefinition* Map, int32 TurnBudget = 0); // TurnBudget 0 이하면 지도 기본 턴

	FSSOnExplorationFinished OnExplorationFinished;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TurnsText;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> CloseButton;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> ResultText; // 발각·시간 초과 등 결과 문구

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UButton> SearchButton;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UButton> WaitButton;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UButton> ReturnButton;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> CurrentRoomText;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> SearchStatusText;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> RoomLootText;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> GuardStatusText;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> CarryText;

	// 탐사가 끝나면 띄울 결과창 WBP (WBP_ExplorationResult) — Class Defaults에서 지정
	UPROPERTY(EditDefaultsOnly, Category="SS|Exploration")
	TSubclassOf<USSExplorationResultWidget> ResultWidgetClass;

private:
	UFUNCTION()
	void Refresh();

	UFUNCTION()
	void HandleRoomClicked(int32 RoomIndex);

	UFUNCTION()
	void HandleCloseClicked(); // RemoveFromParent

	UFUNCTION()
	void HandleSearchClicked();

	UFUNCTION()
	void HandleWaitClicked();

	UFUNCTION()
	void HandleReturnClicked();

	UFUNCTION()
	void HandleResultConfirmed(); // 결과창 확인 → 탐사 화면 닫기 (5단계에서 은신처 정산 추가)

	void ShowResultWindow();  // 탐사가 끝난 순간 한 번만 호출
	void FinishExploration(); // 결과 방송(한 번만) → 탐사 화면 닫기

	UPROPERTY(Transient) // GC가 세션을 지우지 않게 잡아둠
	TObjectPtr<USSExplorationSession> Session;

	UPROPERTY(Transient) // 지도와 연결된 방 위젯만 모아둠
	TArray<TObjectPtr<USSExplorationRoomWidget>> RoomWidgets;

	UPROPERTY(Transient)
	TObjectPtr<USSExplorationResultWidget> ResultWidget; // 이미 띄웠는지 확인용 (중복 방지)

	bool bFinishBroadcast = false; // 정산이 두 번 되지 않게
};
