#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SSMainMenuGameMode.generated.h"

class USSMainMenuWidget;

// ─────────────────────────────────────────────
// 메인 화면 맵(L_MainMenu) 전용 게임모드
// 조작할 캐릭터 없이 메인 화면 위젯만 띄우고 마우스로 고르게 함
// ─────────────────────────────────────────────
UCLASS()
class SIXTYSECONDS_API ASSMainMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASSMainMenuGameMode();

protected:
	virtual void BeginPlay() override;

	// 메인 화면 위젯 (WBP_MainMenu) — BP 게임모드 기본값에서 지정
	UPROPERTY(EditDefaultsOnly, Category="SS|Menu")
	TSubclassOf<USSMainMenuWidget> MenuWidgetClass;

private:
	UPROPERTY(Transient)
	TObjectPtr<USSMainMenuWidget> MenuWidget;
};
