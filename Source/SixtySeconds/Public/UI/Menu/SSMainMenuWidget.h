#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SSMainMenuWidget.generated.h"

class UButton;
class UTextBlock;

// ─────────────────────────────────────────────
// 메인 화면 (시작 / 종료)
// 동작은 코드, 모양(배경·로고·버튼 배치)은 WBP_MainMenu에서 편집
// ─────────────────────────────────────────────
UCLASS(Abstract)
class SIXTYSECONDS_API USSMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// [시작]: 새 판을 시작하고 스크램블 맵으로
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> StartButton;

	// [종료]
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UButton> QuitButton;

	// 아래 작은 글자 (버전·제작자 등. 비우면 그대로)
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> FooterText;

	// 시작하면 열 맵 (스크램블)
	UPROPERTY(EditAnywhere, Category="SS|Menu")
	FName GameLevel = TEXT("L_SSResearchLab");

private:
	UFUNCTION()
	void OnStartClicked();

	UFUNCTION()
	void OnQuitClicked();

	// 두 번 눌러 맵을 두 번 여는 일 방지
	bool bStarting = false;
};
