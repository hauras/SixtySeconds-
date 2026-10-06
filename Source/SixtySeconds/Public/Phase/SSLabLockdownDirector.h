#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SSLabLockdownDirector.generated.h"

class UCameraComponent;
class ULightComponent;

// ─────────────────────────────────────────────
// 격벽 폐쇄 연출 (스크램블 시간이 끝난 뒤 몇 초)
// 1) 대피실 문이 닫히는 장면 → 2) 연구동 문이 열리고 경비 안드로이드가 걸어 나오는 장면
// → 끝나면 GameMode가 시간 종료 순간에 정해 둔 결과(은신처 / 붙잡힘)를 적용
// 스크램블 동안에는 비상등을 깜빡임 (10초 이하면 빠르게)
//
// 맵에서 태그로 찾는 것
//   SSLockdownShelterDoor : 닫히는 대피실 문 (짝을 이룬 문들이 가운데로 모임)
//   SSLockdownResearchDoor: 열리는 연구동 문 (가운데에서 바깥으로 벌어짐)
//   SSLockdownGuard       : 연구동에서 걸어 나오는 경비 (처음엔 숨김)
//   SSLockdownAlarm       : 깜빡일 비상등 (라이트는 맵에서 Movable이어야 함)
// 카메라 위치는 맵에 둔 액터(ShelterShot, ResearchShot)를 따르고, 없으면 기본 위치 값을 씀
// ─────────────────────────────────────────────
UCLASS()
class SIXTYSECONDS_API ASSLabLockdownDirector : public AActor
{
	GENERATED_BODY()

public:
	ASSLabLockdownDirector();
	virtual void Tick(float DeltaSeconds) override;

	// 연출 시작 (GameMode가 시간 종료 때 부름)
	void BeginLockdown();

	// ── 맵에서 정하는 값 ──

	// 대피실 문 장면 카메라 위치로 쓸 액터 (비우면 아래 기본 위치)
	UPROPERTY(EditAnywhere, Category="SS|Lockdown")
	TObjectPtr<AActor> ShelterShot;

	// 대피실 문 장면 기본 카메라 위치
	UPROPERTY(EditAnywhere, Category="SS|Lockdown")
	FTransform ShelterShotDefault = FTransform(FRotator(-3.f, -90.f, 0.f), FVector(0.f, -1350.f, 205.f));

	// 연구동 문 장면 카메라 위치로 쓸 액터 (비우면 아래 기본 위치)
	UPROPERTY(EditAnywhere, Category="SS|Lockdown")
	TObjectPtr<AActor> ResearchShot;

	// 연구동 문 장면 기본 카메라 위치
	UPROPERTY(EditAnywhere, Category="SS|Lockdown")
	FTransform ResearchShotDefault = FTransform(FRotator(-2.f, 90.f, 0.f), FVector(0.f, 1200.f, 185.f));

	// 문 한 짝이 움직이는 거리 (cm). 닫힐 땐 가운데로, 열릴 땐 바깥으로
	UPROPERTY(EditAnywhere, Category="SS|Lockdown", meta=(ClampMin="0"))
	float DoorSlideDistance = 207.f;

	// 경비가 걸어 나오는 거리 (cm, -Y 방향)
	UPROPERTY(EditAnywhere, Category="SS|Lockdown", meta=(ClampMin="0"))
	float GuardWalkDistance = 420.f;

protected:
	virtual void BeginPlay() override;

private:
	// 태그 붙은 문·경비·비상등을 찾아 처음 상태를 기억
	void CacheScene();

	// 같은 태그의 문들을 Alpha(0~1)만큼 닫거나 엶
	void MoveTaggedDoors(FName Tag, float Alpha, bool bClose);

	// 카메라를 그 장면 위치로 (액터가 있으면 액터, 없으면 기본 값)
	void MoveCameraTo(const AActor* Shot, const FTransform& Default);

	UPROPERTY(VisibleAnywhere, Category="SS|Lockdown")
	TObjectPtr<UCameraComponent> Camera;

	// 문이나 경비 하나의 처음 상태
	struct FDoorMotion
	{
		TWeakObjectPtr<AActor> Actor;
		FVector InitialLocation;
		FName Tag;

		// 닫힐 때 움직이는 방향 (짝의 가운데 쪽). 열릴 땐 반대
		FVector CloseDirection = FVector::ZeroVector;
	};

	// 비상등 하나와 원래 밝기
	struct FAlarmLight
	{
		TWeakObjectPtr<ULightComponent> Light;
		float InitialIntensity = 0.f;
	};

	TArray<FDoorMotion> Doors;
	TArray<FDoorMotion> Guards;
	TArray<FAlarmLight> Lights;

	// 비상등 깜빡임에 쓰는 시간
	float AlarmElapsed = 0.f;

	// 연출이 시작된 뒤 지난 시간
	float SequenceElapsed = 0.f;

	bool bPlaying = false;
	bool bShowingResearchDoor = false;
};
