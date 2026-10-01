#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SSEncryptedDataRewardWidget.generated.h"

class UButton;
class UTextBlock;

/** 외부 통신 완료 후 새 암호문이 대기함에 들어왔음을 알리는 보상 화면. */
UCLASS()
class SIXTYSECONDS_API USSEncryptedDataRewardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ShowReward(int32 PendingCount, const FText& Deadline);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION() void HandleConfirm();

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DetailText;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ConfirmButton;
};
