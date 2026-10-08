#pragma once

#include "CoreMinimal.h"
#include "SSSignalFinder.generated.h"

// ─────────────────────────────────────────────
// 센서가 신호를 한 번 읽은 값
// "센서 몇 번이 신호까지 거리를 얼마로 쟀는지"
// 날을 넘어 저장할 거라서 USTRUCT
// ─────────────────────────────────────────────
USTRUCT()
struct SIXTYSECONDS_API FSSSensorReading
{
	GENERATED_BODY()

	// 어느 센서가 쟀는지 (센서 배열의 번호)
	UPROPERTY()
	int32 SensorIndex = 0;

	// 잰 거리 (잡음이 섞여 있음)
	UPROPERTY()
	float Distance = 0.f;

	// 이 값을 얼마나 믿을지 (오래된 값일수록 작아짐)
	UPROPERTY()
	float Weight = 1.f;
};

// ─────────────────────────────────────────────
// 적의 추측 결과
// "신호가 어디서 왔다고 생각하는지, 얼마나 확신하는지"
// ─────────────────────────────────────────────
struct FSSEnemyGuess
{
	// 위치를 찾았는지 (읽은 값이 부족하면 false)
	bool bFound = false;

	// 적이 생각하는 신호 위치
	FVector2D Position = FVector2D::ZeroVector;

	// 의심 범위(타원)의 긴 반지름, 짧은 반지름. 작을수록 확신
	FVector2D OvalSize = FVector2D::ZeroVector;

	// 의심 범위가 기울어진 각도 (라디안)
	float OvalAngle = 0.f;

	// 어긋남 정도: 읽은 값들이 서로 얼마나 안 맞는지
	//   1 근처   = 잘 맞음 (센서 오차 정도만 있음)
	//   3 이상   = 이상하게 안 맞음 (미끼처럼 다른 곳 신호가 섞였을 수 있음)
	//   0        = 계산 안 됨 (읽은 값이 3개 이하)
	// 통계 용어로는 "정규화 카이제곱"
	float Mismatch = 0.f;
};

// ─────────────────────────────────────────────
// 신호 위치 찾기
// 센서들이 읽은 거리로 신호가 어디서 왔는지 계산한다 (적이 우리를 찾는 능력)
// 부모 없음. 기억 없이 계산만 하는 함수 모음
// ─────────────────────────────────────────────
class SIXTYSECONDS_API FSSSignalFinder
{
public:
	// 센서 위치와 읽은 값들로 신호 위치를 찾음
	//   Sensors    : 센서들의 위치
	//   Readings   : 센서들이 읽은 값들
	//   NoiseSigma : 센서 한 번 읽을 때의 오차 크기
	//
	// 방법: 센서들의 가운데에서 출발해, 모든 거리가 가장 잘 맞는 쪽으로 조금씩 옮겨감 (가우스-뉴턴)
	// 주의: 신호가 센서들에 둘러싸여 있어야 함. 센서 무리 바깥 멀리 있으면 엉뚱한 곳에 멈출 수 있음.
	//       센서가 부서지거나 옮겨지는 규칙이 생기면 여러 출발점에서 풀어보는 방식으로 보강할 것.
	static FSSEnemyGuess Find(
		const TArray<FVector2D>& Sensors,
		const TArray<FSSSensorReading>& Readings,
		float NoiseSigma);

private:
	// 읽은 값 하나로 방향과 빗나간 정도를 구함 (Find 안에서 여러 번 씀)
	//   OutDir  : 센서 → Pos 방향 (길이 1)
	//   OutMiss : 빗나간 정도. +면 너무 멂, −면 너무 가까움
	// 센서 번호가 잘못됐거나 Pos가 센서 바로 위면 false
	static bool TryGetMiss(
		const TArray<FVector2D>& Sensors,
		const FSSSensorReading& Reading,
		const FVector2D& Pos,
		FVector2D& OutDir,
		double& OutMiss);
};
