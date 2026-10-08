#include "Phase/SSGuardAndroid.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"

ASSGuardAndroid::ASSGuardAndroid()
{
	PrimaryActorTick.bCanEverTick = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	const auto MakePart = [this](FName Name, USceneComponent* Parent, FVector Location)
	{
		UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Part->SetupAttachment(Parent);
		Part->SetRelativeLocation(Location);
		Part->SetMobility(EComponentMobility::Movable);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		return Part;
	};
	Body = MakePart(TEXT("Body"), SceneRoot, FVector(0.f, 0.f, 101.f));
	Head = MakePart(TEXT("Head"), Body, FVector(0.f, 0.f, 62.f));
	LeftArm = MakePart(TEXT("LeftArm"), Body, FVector(-35.f, 0.f, 48.f));
	RightArm = MakePart(TEXT("RightArm"), Body, FVector(35.f, 0.f, 48.f));
	LeftForearm = MakePart(TEXT("LeftForearm"), LeftArm, FVector(0.f, 0.f, -36.f));
	RightForearm = MakePart(TEXT("RightForearm"), RightArm, FVector(0.f, 0.f, -36.f));
	LeftThigh = MakePart(TEXT("LeftThigh"), Body, FVector(-14.f, 0.f, -7.f));
	RightThigh = MakePart(TEXT("RightThigh"), Body, FVector(14.f, 0.f, -7.f));
	LeftShin = MakePart(TEXT("LeftShin"), LeftThigh, FVector(0.f, 0.f, -46.f));
	RightShin = MakePart(TEXT("RightShin"), RightThigh, FVector(0.f, 0.f, -46.f));
	LeftFoot = MakePart(TEXT("LeftFoot"), LeftShin, FVector(0.f, -6.f, -42.f));
	RightFoot = MakePart(TEXT("RightFoot"), RightShin, FVector(0.f, -6.f, -42.f));
}

void ASSGuardAndroid::PoseLeg(UStaticMeshComponent* Thigh, UStaticMeshComponent* Shin, UStaticMeshComponent* Foot, float Phase)
{
	// 발을 앞뒤로 옮기고 들어 올리는 높이에 맞춰 무릎 각도를 구한다.
	const float Forward = 20.f * FMath::Cos(Phase);
	const float Down = 85.f - 9.f * FMath::Max(0.f, FMath::Sin(Phase));
	const float Reach = FMath::Clamp(FMath::Sqrt(Forward * Forward + Down * Down), 5.f, 87.9f);
	const float Hip = FMath::Atan2(Forward, Down) + FMath::Acos(FMath::Clamp((46.f * 46.f + Reach * Reach - 42.f * 42.f) / (2.f * 46.f * Reach), -1.f, 1.f));
	const float Knee = -FMath::Acos(FMath::Clamp(
		(Reach * Reach - 46.f * 46.f - 42.f * 42.f) / (2.f * 46.f * 42.f), -1.f, 1.f));
	Thigh->SetRelativeRotation(FRotator(0.f, 0.f, FMath::RadiansToDegrees(Hip)));
	Shin->SetRelativeRotation(FRotator(0.f, 0.f, FMath::RadiansToDegrees(Knee)));
	Foot->SetRelativeRotation(FRotator(0.f, 0.f, -FMath::RadiansToDegrees(Hip + Knee)));
}

void ASSGuardAndroid::UpdateWalk(float Distance, float PhaseOffset)
{
	const float Phase = Distance / 120.f * 2.f * PI + PhaseOffset;
	Body->SetRelativeLocation(FVector(0.f, 0.f, 98.f + 0.7f * FMath::Cos(2.f * Phase)));
	PoseLeg(LeftThigh, LeftShin, LeftFoot, Phase);
	PoseLeg(RightThigh, RightShin, RightFoot, Phase + PI);
	LeftArm->SetRelativeRotation(FRotator(0.f, 0.f, -12.f * FMath::Cos(Phase)));
	RightArm->SetRelativeRotation(FRotator(0.f, 0.f, 12.f * FMath::Cos(Phase)));
	LeftForearm->SetRelativeRotation(FRotator(0.f, 0.f, -8.f));
	RightForearm->SetRelativeRotation(FRotator(0.f, 0.f, -8.f));
}
