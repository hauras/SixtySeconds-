#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SSScrambleHUD.generated.h"

class USSCarryInventoryWidget;
class USSScrambleTimerWidget;
class USSCarryComponent;

// 스크램블 페이즈 HUD. WBP_ScrambleHUD Blueprint의 부모 클래스
UCLASS(Abstract)
class SIXTYSECONDS_API USSScrambleHUD : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitHUD(USSCarryComponent* InCarry);

protected:
	virtual void NativeConstruct() override;

	// Blueprint에서 이름 맞춰서 배치
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<USSScrambleTimerWidget> TimerWidget;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<USSCarryInventoryWidget> InventoryWidget;

private:
	void InitializeChildren();

	UPROPERTY()
	TObjectPtr<USSCarryComponent> CarryComponent;

};
