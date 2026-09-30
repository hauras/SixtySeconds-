#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SSDialControlWidget.generated.h"

class UButton;
class UTextBlock;
class UProgressBar;
class UBorder;

// 다이얼을 돌림 (몇 번 다이얼인지, +1 오른쪽 / -1 왼쪽). 해독 창이 듣고 세션에 전달
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSSOnDialTurn, int32, DialIndex, int32, Step);

// ─────────────────────────────────────────────
// 다이얼 한 칸: 이름, 현재 값, ◀ ▶ 버튼, 이 다이얼의 신호 세기
// 판단은 하지 않음. 해독 창(USSDialDecodeWidget)이 값을 넣어주고 돌림 요청만 알림
// 버튼마다 다이얼 번호를 넘겨야 해서 한 칸을 위젯으로 분리
// ─────────────────────────────────────────────
UCLASS()
class SIXTYSECONDS_API USSDialControlWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 몇 번 다이얼인지와 색 (화면 만들기 전에 불러도 되고 뒤에 불러도 됨)
	void Setup(int32 InDialIndex, const FLinearColor& InColor);

	// 현재 값·신호 세기 표시. bLocked면 버튼을 끔 (잠금 해제 후)
	void ShowState(int32 Value, float Strength, bool bLocked);

	UPROPERTY(BlueprintAssignable, Category="SS|Decode")
	FSSOnDialTurn OnDialTurn;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// 다이얼 위에서 마우스 휠을 굴리면 돌림 (위 = 오른쪽 +1, 아래 = 왼쪽 -1)
	virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	UFUNCTION() void HandleLeft();
	UFUNCTION() void HandleRight();

	int32 DialIndex = 0;
	FLinearColor DialColor = FLinearColor::White;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ValueText;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> Needle;

	UPROPERTY(Transient)
	TObjectPtr<UButton> LeftButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> RightButton;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> StrengthBar;
};
