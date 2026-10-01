#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SSRadioWidget.generated.h"

class USSRunSubsystem;
class USSTraceWidget;
class USSTraceConfig;
class USSTruthDecodeWidget;
class UButton;
class UTextBlock;

// ─────────────────────────────────────────────
// 무전기 메뉴 (B1 비상 수신기, 아라 네트워크와 연결 안 됨)
// 은신처의 무전기를 누르면 뜸: [외부 통신] [해독 (N)] [닫기]
// 역추적 창·해독 창을 열고 그 수명을 관리 (컴퓨터 화면에서 옮겨 옴). 화면은 코드로 만듦
// ─────────────────────────────────────────────
UCLASS()
class SIXTYSECONDS_API USSRadioWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 역추적 창 클래스와 규칙 (HUD가 넘겨줌)
	void SetTraceSetup(TSubclassOf<USSTraceWidget> InClass, USSTraceConfig* InConfig)
	{
		TraceClass = InClass;
		TraceConfig = InConfig;
	}
	void SetTruthDecodeClass(TSubclassOf<USSTruthDecodeWidget> InClass) { TruthDecodeClass = InClass; }

	// 무전기에서 연 창(역추적·해독)이 떠 있는지
	bool HasOpenWindow() const;

	// 메뉴와 연 창을 모두 닫음 (은신처 화면이 사라질 때)
	void CloseWindows();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION() void OpenTrace();
	UFUNCTION() void OpenDecode();
	UFUNCTION() void CloseRadio();

	UPROPERTY(Transient)
	TObjectPtr<USSRunSubsystem> RunSubsystem;

	UPROPERTY(Transient)
	TSubclassOf<USSTraceWidget> TraceClass;

	UPROPERTY(Transient)
	TObjectPtr<USSTraceConfig> TraceConfig;

	UPROPERTY(Transient)
	TSubclassOf<USSTruthDecodeWidget> TruthDecodeClass;

	// 무전기에서 연 창들
	UPROPERTY(Transient)
	TObjectPtr<USSTraceWidget> TraceWidget;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> DecodeWidget;   // 다이얼 창 또는 진실 해독 창

	UPROPERTY(Transient)
	TObjectPtr<UButton> TraceButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> DecodeButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> CloseButton;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatusText;
};
