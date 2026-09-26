
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SSExplorationWidget.generated.h"

class UButton;
class UTextBlock;
class USSExplorationSession;
class USSExplorationRoomWidget;
class USSExplorationMapDefinition;
/**
 * 
 */
UCLASS()
class SIXTYSECONDS_API USSExplorationWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	bool StartExploration(USSExplorationMapDefinition* Map);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TurnsText;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> CloseButton;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> ResultText;   // 발각·시간 초과 등 결과 문구

private:
	UFUNCTION()
	void Refresh();

	UFUNCTION()
	void HandleRoomClicked(int32 RoomIndex);

	UFUNCTION()
	void HandleCloseClicked();                 // RemoveFromParent

	UPROPERTY(Transient)                       // GC가 세션을 지우지 않게 잡아둠
	TObjectPtr<USSExplorationSession> Session;

	UPROPERTY(Transient)                       // 지도와 연결된 방 위젯만 모아둠
	TArray<TObjectPtr<USSExplorationRoomWidget>> RoomWidgets;
};
