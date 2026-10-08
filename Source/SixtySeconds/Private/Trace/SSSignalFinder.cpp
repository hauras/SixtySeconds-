

#include "Trace/SSSignalFinder.h"

FSSEnemyGuess FSSSignalFinder::Find(const TArray<FVector2D>& Sensors,
	const TArray<FSSSensorReading>& Readings, float NoiseSigma)
{
	FSSEnemyGuess Result;
	if (Readings.Num() < 3 || Sensors.Num() == 0) return Result;

	FVector2D Pos = FVector2D::ZeroVector;

	for (const FVector2D& Sensor : Sensors)
	{
		Pos += Sensor;
	}
	Pos /= Sensors.Num();

	// 집계표: 반복이 끝난 뒤에도 오차 타원 계산에 써야 해서 반복 밖에 만듦
	double Axx = 0;
	double Axy = 0;
	double Ayy = 0;
	double Bx = 0;
	double By = 0;

	for (int32 i = 0; i < 20; ++i)
	{
		// 위치가 바뀌었으니 의견을 처음부터 다시 모음
		Axx = 0;
		Axy = 0;
		Ayy = 0;
		Bx = 0;
		By = 0;

		for (const auto& Reading : Readings)
		{
			// 방향 U와 틀린 양 R (계산할 수 없는 측정은 건너뜀)
			FVector2D U;
			double R = 0;
			if (!TryGetMiss(Sensors, Reading, Pos, U, R)) continue;

			// 이 측정을 믿는 정도
			const double W = Reading.Weight;

			// 집계표에 이 측정의 의견을 더함
			Axx += W * U.X * U.X;
			Axy += W * U.X * U.Y;
			Ayy += W * U.Y * U.Y;
			Bx += W * U.X * R;
			By += W * U.Y * R;
		}

		// 2×2 연립방정식  A · Δ = −B  풀기 (크래머 공식)
		const double Det = Axx * Ayy - Axy * Axy;

		// Det가 사실상 0이면 한쪽 방향을 전혀 못 재는 상황 (센서가 일직선 등) → 실패
		if (FMath::Abs(Det) < 1e-9) return Result;

		const FVector2D Delta(
			(-Bx * Ayy + By * Axy) / Det,
			(-Axx * By + Axy * Bx) / Det);

		Pos += Delta;

		// 거의 안 움직였으면 다 찾은 것
		if (Delta.Size() < 0.01) break;
	}

	// ── 확신도: 의심 범위(타원) ──
	// 잘 잴수록 A가 커짐 → 의심 범위(공분산)는 A의 역행렬 × 센서 오차²
	const double FinalDet = Axx * Ayy - Axy * Axy;
	if (FMath::Abs(FinalDet) < 1e-9) return Result;

	const double Sigma2 = double(NoiseSigma) * double(NoiseSigma);
	const double Cxx = Ayy / FinalDet * Sigma2;
	const double Cxy = -Axy / FinalDet * Sigma2;
	const double Cyy = Axx / FinalDet * Sigma2;

	// 공분산의 고윳값 두 개 = 타원의 긴 방향·짧은 방향의 퍼짐
	const double Mean = (Cxx + Cyy) * 0.5;
	const double Half = FMath::Sqrt(FMath::Square((Cxx - Cyy) * 0.5) + Cxy * Cxy);
	const double Major = FMath::Max(Mean + Half, 0.0); // 계산 오차로 살짝 음수가 되는 것 방지
	const double Minor = FMath::Max(Mean - Half, 0.0);

	// 반지름 = 2 × 표준편차 (약 86% 확률로 이 안에 있음)
	Result.OvalSize = FVector2D(2.0 * FMath::Sqrt(Major), 2.0 * FMath::Sqrt(Minor));

	// 긴 반지름이 향하는 각도 (라디안)
	Result.OvalAngle = float(0.5 * FMath::Atan2(2.0 * Cxy, Cxx - Cyy));

	// ── 어긋남 정도: 정규화 카이제곱 ──
	// 최종 위치에서 남은 틀림이 잡음(σ)으로 설명되는 수준인지 확인
	double ChiSum = 0;
	int32 UsedCount = 0;

	for (const auto& Reading : Readings)
	{
		FVector2D U;
		double R = 0;
		if (!TryGetMiss(Sensors, Reading, Pos, U, R)) continue;

		// 틀림을 제곱해서 더함 (부호 제거 + 크게 틀린 측정이 도드라짐)
		ChiSum += Reading.Weight * R * R;
		++UsedCount;
	}

	// 위치 x, y를 구하는 데 측정 2개를 써버리므로 자유도 = 쓴 측정 수 − 2
	if (UsedCount > 2 && Sigma2 > 0.0)
	{
		Result.Mismatch = float(ChiSum / Sigma2 / (UsedCount - 2));
	}

	// 반복이 끝난 위치가 적의 추측
	Result.bFound = true;
	Result.Position = Pos;
	return Result;
}

bool FSSSignalFinder::TryGetMiss(const TArray<FVector2D>& Sensors, const FSSSensorReading& Reading,
	const FVector2D& Pos, FVector2D& OutDir, double& OutMiss)
{
	// 센서 번호가 잘못된 측정
	if (!Sensors.IsValidIndex(Reading.SensorIndex)) return false;

	const FVector2D SensorPos = Sensors[Reading.SensorIndex];
	const FVector2D ToPos = Pos - SensorPos;
	const double Dist = ToPos.Size();

	// 추정 위치가 센서 바로 위면 방향을 정할 수 없음 (0으로 나누기 방지)
	if (Dist < 1e-3) return false;

	// 센서 → 추정 위치 방향 (길이 1)
	OutDir = ToPos / Dist;

	// 틀린 양: +면 너무 멂, −면 너무 가까움
	OutMiss = Dist - Reading.Distance;
	return true;
}
