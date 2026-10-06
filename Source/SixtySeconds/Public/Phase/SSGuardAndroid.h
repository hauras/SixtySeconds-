#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SSGuardAndroid.generated.h"

class USceneComponent;
class UStaticMeshComponent;

// ─────────────────────────────────────────────
// 격벽 폐쇄 연출에서 걸어 나오는 경비 안드로이드 (메시 조각 13개를 계층으로 붙임)
// 걸은 거리에 맞춰 관절을 움직임: 다리는 두 관절 IK(코사인 법칙으로 엉덩이·무릎 각도), 팔은 반대로 흔듦
// 애니메이션 에셋 없이 코드만으로 걷는 모습을 만듦
// ─────────────────────────────────────────────
UCLASS()
class SIXTYSECONDS_API ASSGuardAndroid : public AActor
{
	GENERATED_BODY()

public:
	ASSGuardAndroid();
	// 걸은 거리(cm)에 맞춰 자세를 잡음. PhaseOffset으로 여러 경비의 발걸음을 어긋나게
	void UpdateWalk(float Distance, float PhaseOffset);

	UPROPERTY(VisibleAnywhere, Category="SS|Android")
	TObjectPtr<USceneComponent> SceneRoot;

	// ── 몸 부품 (몸통 → 머리·팔·허벅지 → 아래팔·정강이 → 발 순서로 붙음) ──

	UPROPERTY(VisibleAnywhere, Category="SS|Android")
	TObjectPtr<UStaticMeshComponent> Body;

	UPROPERTY(VisibleAnywhere, Category="SS|Android")
	TObjectPtr<UStaticMeshComponent> Head;

	UPROPERTY(VisibleAnywhere, Category="SS|Android")
	TObjectPtr<UStaticMeshComponent> LeftArm;

	UPROPERTY(VisibleAnywhere, Category="SS|Android")
	TObjectPtr<UStaticMeshComponent> RightArm;

	UPROPERTY(VisibleAnywhere, Category="SS|Android")
	TObjectPtr<UStaticMeshComponent> LeftForearm;

	UPROPERTY(VisibleAnywhere, Category="SS|Android")
	TObjectPtr<UStaticMeshComponent> RightForearm;

	UPROPERTY(VisibleAnywhere, Category="SS|Android")
	TObjectPtr<UStaticMeshComponent> LeftThigh;

	UPROPERTY(VisibleAnywhere, Category="SS|Android")
	TObjectPtr<UStaticMeshComponent> RightThigh;

	UPROPERTY(VisibleAnywhere, Category="SS|Android")
	TObjectPtr<UStaticMeshComponent> LeftShin;

	UPROPERTY(VisibleAnywhere, Category="SS|Android")
	TObjectPtr<UStaticMeshComponent> RightShin;

	UPROPERTY(VisibleAnywhere, Category="SS|Android")
	TObjectPtr<UStaticMeshComponent> LeftFoot;

	UPROPERTY(VisibleAnywhere, Category="SS|Android")
	TObjectPtr<UStaticMeshComponent> RightFoot;

private:
	// 다리 하나: 발이 갈 위치를 정하고 IK로 허벅지·정강이·발 각도를 맞춤 (Phase = 걸음 주기의 위치)
	void PoseLeg(UStaticMeshComponent* Thigh, UStaticMeshComponent* Shin, UStaticMeshComponent* Foot, float Phase);
};
