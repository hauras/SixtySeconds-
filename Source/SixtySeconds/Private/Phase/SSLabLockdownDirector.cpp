#include "Phase/SSLabLockdownDirector.h"

#include "Camera/CameraComponent.h"
#include "Components/LightComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

#include "GameMode/SSGameMode.h"
#include "Phase/SSGuardAndroid.h"

namespace SSLockdownTiming
{
	// 연출 시간표 (초). 시작 → 대피실 문 닫힘 → 연구동 장면 → 문 열림 → 경비 걸음 → 결과
	constexpr float ShelterDoorStart = 0.35f;
	constexpr float ShelterDoorDuration = 1.4f;
	constexpr float CutToResearch = 2.f;
	constexpr float ResearchDoorStart = 2.2f;
	constexpr float ResearchDoorDuration = 1.1f;
	constexpr float GuardWalkStart = 3.3f;
	constexpr float GuardWalkDuration = 2.1f;
	constexpr float Finish = 5.7f;

	// 시작할 때 카메라가 넘어가는 시간
	constexpr float CameraBlend = 0.35f;
}

ASSLabLockdownDirector::ASSLabLockdownDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("LockdownCamera"));
	RootComponent = Camera;
	Camera->FieldOfView = 65.f;
}

void ASSLabLockdownDirector::BeginPlay()
{
	Super::BeginPlay();
	CacheScene();
}

void ASSLabLockdownDirector::CacheScene()
{
	Doors.Reset();
	Guards.Reset();
	Lights.Reset();

	TArray<AActor*> SceneActors;
	UGameplayStatics::GetAllActorsOfClass(this, AActor::StaticClass(), SceneActors);
	for (AActor* Actor : SceneActors)
	{
		for (const FName DoorTag : { FName(TEXT("SSLockdownShelterDoor")), FName(TEXT("SSLockdownResearchDoor")) })
		{
			if (Actor->ActorHasTag(DoorTag)) Doors.Add({ Actor, Actor->GetActorLocation(), DoorTag });
		}

		if (Actor->ActorHasTag(TEXT("SSLockdownGuard")))
		{
			Guards.Add({ Actor, Actor->GetActorLocation(), NAME_None });
			Actor->SetActorHiddenInGame(true);
		}

		if (Actor->ActorHasTag(TEXT("SSLockdownAlarm")))
		{
			TArray<ULightComponent*> Components;
			Actor->GetComponents(Components);
			for (ULightComponent* Light : Components)
			{
				// 실행 중에 이동성을 바꾸지 않음 (정적으로 구운 라이트는 경고만 남기고 건너뜀)
				if (Light->Mobility != EComponentMobility::Movable)
				{
					UE_LOG(LogTemp, Warning, TEXT("[Lockdown] Alarm light on %s is not Movable; set it Movable in the map."), *Actor->GetName());
					continue;
				}
				Light->SetLightColor(FLinearColor(1.f, 0.035f, 0.015f));
				Lights.Add({ Light, Light->Intensity });
			}
		}
	}

	// 문마다 닫히는 방향 = 같은 태그 문들의 가운데 쪽 (수평으로만)
	// 한 짝뿐이면 그 문이 선 줄의 X=0을 가운데로 봄 (지금 맵은 문이 X=0 기준 좌우 대칭)
	for (FDoorMotion& Door : Doors)
	{
		FVector Center = FVector::ZeroVector;
		int32 Count = 0;
		for (const FDoorMotion& Other : Doors)
		{
			if (Other.Tag != Door.Tag) continue;
			Center += Other.InitialLocation;
			++Count;
		}
		Center = Count > 1 ? Center / Count : FVector(0.f, Door.InitialLocation.Y, Door.InitialLocation.Z);

		const FVector ToCenter = FVector(Center.X - Door.InitialLocation.X, Center.Y - Door.InitialLocation.Y, 0.f);
		Door.CloseDirection = ToCenter.GetSafeNormal();
	}
}

void ASSLabLockdownDirector::MoveCameraTo(const AActor* Shot, const FTransform& Default)
{
	const FTransform Target = IsValid(Shot) ? Shot->GetActorTransform() : Default;
	SetActorLocationAndRotation(Target.GetLocation(), Target.Rotator());
}

void ASSLabLockdownDirector::BeginLockdown()
{
	if (bPlaying) return;
	bPlaying = true;
	SequenceElapsed = 0.f;
	bShowingResearchDoor = false;

	MoveCameraTo(ShelterShot, ShelterShotDefault);
	if (APlayerController* Controller = UGameplayStatics::GetPlayerController(this, 0))
	{
		Controller->SetViewTargetWithBlend(this, SSLockdownTiming::CameraBlend);
	}
}

void ASSLabLockdownDirector::MoveTaggedDoors(FName Tag, float Alpha, bool bClose)
{
	const float SmoothAlpha = FMath::SmoothStep(0.f, 1.f, Alpha);
	for (const FDoorMotion& Door : Doors)
	{
		if (Door.Tag != Tag || !Door.Actor.IsValid()) continue;

		// 닫힐 땐 가운데로, 열릴 땐 바깥으로 DoorSlideDistance만큼
		const FVector Direction = bClose ? Door.CloseDirection : -Door.CloseDirection;
		const FVector Target = Door.InitialLocation + Direction * DoorSlideDistance;
		Door.Actor->SetActorLocation(FMath::Lerp(Door.InitialLocation, Target, SmoothAlpha));
	}
}

void ASSLabLockdownDirector::Tick(float DeltaSeconds)
{
	using namespace SSLockdownTiming;
	Super::Tick(DeltaSeconds);

	ASSGameMode* Mode = GetWorld()->GetAuthGameMode<ASSGameMode>();
	if (!IsValid(Mode)) return;
	if (Mode->GetCurrentPhase() != ESSGamePhase::Scramble && !bPlaying)
	{
		SetActorTickEnabled(false);
		return;
	}

	// 비상등: 화면 전체를 번쩍이지 않고 실제 비상등만 부드럽게 밝아졌다 어두워짐 (10초 이하면 빠르게)
	AlarmElapsed += DeltaSeconds;
	const bool bUrgent = bPlaying || Mode->GetScrambleTimeRemaining() <= 10.f;
	const float Pulse = 0.25f + 0.75f * (0.5f + 0.5f * FMath::Sin(AlarmElapsed * (bUrgent ? 9.f : 5.f)));
	for (const FAlarmLight& Alarm : Lights)
	{
		if (Alarm.Light.IsValid()) Alarm.Light->SetIntensity(Alarm.InitialIntensity * Pulse);
	}
	if (!bPlaying) return;

	SequenceElapsed += DeltaSeconds;

	// 1) 대피실 문 닫힘
	MoveTaggedDoors(TEXT("SSLockdownShelterDoor"), FMath::Clamp((SequenceElapsed - ShelterDoorStart) / ShelterDoorDuration, 0.f, 1.f), true);

	// 2) 연구동 장면으로 컷 → 문 열림
	if (SequenceElapsed >= CutToResearch && !bShowingResearchDoor)
	{
		bShowingResearchDoor = true;
		MoveCameraTo(ResearchShot, ResearchShotDefault);
	}
	MoveTaggedDoors(TEXT("SSLockdownResearchDoor"), FMath::Clamp((SequenceElapsed - ResearchDoorStart) / ResearchDoorDuration, 0.f, 1.f), false);

	// 3) 경비가 걸어 나옴 (걸은 거리에 맞춰 다리를 움직임)
	if (SequenceElapsed >= GuardWalkStart)
	{
		const float Walked = GuardWalkDistance * FMath::Clamp((SequenceElapsed - GuardWalkStart) / GuardWalkDuration, 0.f, 1.f);
		for (const FDoorMotion& Guard : Guards)
		{
			if (!Guard.Actor.IsValid()) continue;
			Guard.Actor->SetActorHiddenInGame(false);
			FVector Position = Guard.InitialLocation;
			Position.Y -= Walked;
			Guard.Actor->SetActorLocation(Position);
			if (ASSGuardAndroid* Android = Cast<ASSGuardAndroid>(Guard.Actor.Get()))
			{
				// 왼쪽·오른쪽 경비가 발을 다르게 내딛도록 위상을 어긋나게
				Android->UpdateWalk(Walked, Position.X < 0.f ? 0.f : PI * 0.5f);
			}
		}
	}

	// 4) 끝: 시간 종료 순간에 정한 결과 적용
	if (SequenceElapsed >= Finish)
	{
		bPlaying = false;
		SetActorTickEnabled(false);
		Mode->CompleteScrambleTransition();
	}
}
