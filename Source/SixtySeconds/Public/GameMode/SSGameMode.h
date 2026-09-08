#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Item/SSDepositZone.h"
#include "SSGameMode.generated.h"

UENUM(BlueprintType)
enum class ESSGamePhase : uint8
{
	Scramble,  // 60초 스크램블 (3D 실시간)
	Shelter    // 은신처 생존 (2D 턴제)
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPhaseChanged, ESSGamePhase, NewPhase);

UCLASS()
class SIXTYSECONDS_API ASSGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASSGameMode();

	// 스크램블 페이즈 시작 (60초 타이머 가동)
	UFUNCTION(BlueprintCallable, Category="SS|Phase")
	void StartScramble();

	// 은신처 페이즈로 전환
	UFUNCTION(BlueprintCallable, Category="SS|Phase")
	void StartShelter();

	UFUNCTION(BlueprintPure, Category="SS|Phase")
	ESSGamePhase GetCurrentPhase() const { return CurrentPhase; }

	UFUNCTION(BlueprintPure, Category="SS|Phase")
	float GetScrambleTimeRemaining() const;

	// 페이즈 전환 시 Blueprint에서 구독
	UPROPERTY(BlueprintAssignable, Category="SS|Phase")
	FOnPhaseChanged OnPhaseChanged;

	// 사망 처리 (존 밖에서 타이머 종료)
	UFUNCTION(BlueprintCallable, Category="SS|Phase")
	void StartDeath();

	UPROPERTY(BlueprintAssignable, Category="SS|Phase")
	FOnPhaseChanged OnPlayerDied;

protected:
	virtual void BeginPlay() override;

	// 스크램블 제한 시간 (초)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SS|Phase")
	float ScrambleDuration = 60.f;

private:
	ESSGamePhase CurrentPhase = ESSGamePhase::Scramble;
	FTimerHandle ScrambleTimerHandle;

	UPROPERTY()
	TObjectPtr<ASSDepositZone> DepositZone;

	void OnScrambleTimeUp();
	void CacheDepositZone();
};
