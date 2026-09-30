
#pragma once

#include "CoreMinimal.h"
#include "Trace/SSSignalFinder.h"

#include "SSSuspectMemory.generated.h"

USTRUCT()
struct SIXTYSECONDS_API FSSSuspectSpot
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FSSSensorReading> Readings;

	UPROPERTY()
	FVector2D Position = FVector2D::ZeroVector;

	// 의심 범위의 긴 반지름 (= OvalSize.X). 작을수록 확신
	UPROPERTY()
	float Radius = 0.f;

	// 의심 범위(타원)의 긴 반지름, 짧은 반지름
	UPROPERTY()
	FVector2D OvalSize = FVector2D::ZeroVector;

	// 의심 범위가 기울어진 각도 (라디안)
	UPROPERTY()
	float OvalAngle = 0.f;

	UPROPERTY()
	int32 Bursts = 0;
};

// ─────────────────────────────────────────────
// 적의 기억: 의심 장소들을 관리
// 새 신호가 오면 가까운 의심 장소에 합치거나, 새 의심 장소를 만든다
// 부모 없음. 의심 장소 목록은 부르는 쪽(세션, RunSubsystem)이 들고 있음
// ─────────────────────────────────────────────
class SIXTYSECONDS_API FSSSuspectMemory
{
public:
	// 송신 한 번을 기억에 넣음
	//   Spots      : 지금까지의 의심 장소들 (이 함수가 고침)
	//   Sensors    : 센서들의 위치
	//   Readings   : 이번 송신에서 센서들이 읽은 값들
	//   NoiseSigma : 센서 한 번 읽을 때의 오차 크기
	// 반환: 합쳐졌거나 새로 생긴 의심 장소의 번호. 이 송신만으로 위치를 못 찾으면 INDEX_NONE
	static int32 AddBurst(
		TArray<FSSSuspectSpot>& Spots,
		const TArray<FVector2D>& Sensors,
		const TArray<FSSSensorReading>& Readings,
		float NoiseSigma);

	// 하루가 지나면 적의 기억이 흐려짐: 모든 읽은 값의 믿음(Weight)에 Factor를 곱함
	// 믿음이 ForgetWeight보다 작아진 값은 잊고, 값이 하나도 안 남은 장소는 지움
	// (위치와 반경은 다음 판을 시작할 때 Refresh로 다시 계산)
	static void DecayDaily(
		TArray<FSSSuspectSpot>& Spots,
		float Factor = 0.5f,
		float ForgetWeight = 0.02f);

	// 남은 읽은 값으로 장소마다 위치와 반경을 다시 계산. 위치를 못 찾는 장소는 지움
	static void Refresh(
		TArray<FSSSuspectSpot>& Spots,
		const TArray<FVector2D>& Sensors,
		float NoiseSigma);

	// 점이 의심 범위(타원) 안에 있나
	// 타원 반지름이 2σ라서, 공분산으로 잰 거리(마할라노비스 거리)가 2 이하인지와 같은 판정
	static bool IsInsideOval(
		const FSSSuspectSpot& Spot,
		const FVector2D& Point);
};
