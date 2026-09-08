#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameMode/SSGameMode.h"
#include "SSPhaseManager.generated.h"

UCLASS(ClassGroup=(SixtySeconds), meta=(BlueprintSpawnableComponent))
class SIXTYSECONDS_API USSPhaseManager : public UActorComponent
{
	GENERATED_BODY()

public:
	USSPhaseManager();

	// 현재 페이즈 반환
	UFUNCTION(BlueprintPure, Category="SS|Phase")
	ESSGamePhase GetCurrentPhase() const { return CurrentPhase; }

	// 스크램블 페이즈 초기화 (GameMode가 호출)
	UFUNCTION(BlueprintCallable, Category="SS|Phase")
	void OnScrambleStarted();

	// 은신처 페이즈 초기화 (GameMode가 호출)
	UFUNCTION(BlueprintCallable, Category="SS|Phase")
	void OnShelterStarted();

	// 은신처 턴 진행 — 매 턴 GameMode가 호출
	UFUNCTION(BlueprintCallable, Category="SS|Phase")
	void AdvanceShelterTurn();

	UFUNCTION(BlueprintPure, Category="SS|Phase")
	int32 GetCurrentDay() const { return CurrentDay; }

	UFUNCTION(BlueprintPure, Category="SS|Phase")
	int32 GetCurrentTurn() const { return CurrentTurn; }

protected:
	virtual void BeginPlay() override;

	// 하루당 최대 턴 수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SS|Phase")
	int32 TurnsPerDay = 3;

private:
	ESSGamePhase CurrentPhase = ESSGamePhase::Scramble;
	int32 CurrentDay = 1;
	int32 CurrentTurn = 1;

	void OnPhaseChanged(ESSGamePhase NewPhase);
};
