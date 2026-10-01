#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SSCipherCellWidget.generated.h"

class UButton;
class UTextBlock;

// 글자 칸을 누름 (어느 암호 글자인지). 진실 해독 창이 듣고 그 칸을 선택
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSSOnCipherCellClicked, int32, CipherLetter);

// ─────────────────────────────────────────────
// 치환표 한 칸: 위에 추측 글자(모르면 빈칸), 아래 암호 글자
// 판단은 하지 않음. 진실 해독 창(USSTruthDecodeWidget)이 값을 넣어주고 눌림만 알림
// ─────────────────────────────────────────────
UCLASS()
class SIXTYSECONDS_API USSCipherCellWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 어느 암호 글자 칸인지 (0~25)
	void Setup(int32 InCipherLetter, int32 InFrequency = 0);

	// 추측(INDEX_NONE이면 빈칸), 선택됐는지, 입력을 막을지
	void ShowState(int32 GuessLetter, bool bSelected, bool bLocked);

	UPROPERTY(BlueprintAssignable, Category="SS|Decode")
	FSSOnCipherCellClicked OnCellClicked;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION() void HandleClicked();

	int32 CipherLetter = 0;
	int32 Frequency = 0;

	UPROPERTY(Transient)
	TObjectPtr<UButton> CellButton;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> GuessText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CipherText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> FrequencyText;
};
