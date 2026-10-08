#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SSScrambleHUD.generated.h"

class USSCarryInventoryWidget;
class USSScrambleTimerWidget;
class USSCarryComponent;
class USSInteractPromptWidget;

// 스크램블 페이즈 HUD. WBP_ScrambleHUD Blueprint의 부모 클래스
UCLASS(Abstract)
class SIXTYSECONDS_API USSScrambleHUD : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitHUD(USSCarryComponent* InCarry);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// 매 프레임: 가장 가까운 상호작용 대상 위에 안내를 띄움
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// Blueprint에서 이름 맞춰서 배치
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<USSScrambleTimerWidget> TimerWidget;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<USSCarryInventoryWidget> InventoryWidget;

private:
	void InitializeChildren();

	UPROPERTY()
	TObjectPtr<USSCarryComponent> CarryComponent;

	// 아이템·동료 위 안내 ("식량 · [E] 줍기")
	UPROPERTY(Transient)
	TObjectPtr<USSInteractPromptWidget> PromptWidget;

	// 안내를 대상 머리 위 얼마나 높이 띄울지 (cm)
	static constexpr float PromptHeightAboveTarget = 25.f;
};
