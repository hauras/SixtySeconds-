#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SSDepositZone.generated.h"

class USSCarryComponent;
class USSRunSubsystem;
class UBoxComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSSOnDeposited);

// 은신처 입구 트리거. 캐릭터가 진입하면 운반 중인 물자를 보관함으로 이전
UCLASS()
class SIXTYSECONDS_API ASSDepositZone : public AActor
{
	GENERATED_BODY()

public:
	ASSDepositZone();

	UPROPERTY(BlueprintAssignable, Category="SS|Deposit")
	FSSOnDeposited OnDeposited;

	bool IsPlayerInside() const { return bIsPlayerInside; }
	
protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SS|Deposit")
	TObjectPtr<UBoxComponent> TriggerBox;


private:
	bool bIsPlayerInside = false;

	UFUNCTION()
	void OnActorEntered(AActor* OverlappedActor, AActor* OtherActor);

	UFUNCTION()
	void OnActorExited(AActor* OverlappedActor, AActor* OtherActor);
};
