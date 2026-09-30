

#include "Trace/SSSuspectMemory.h"

namespace
{
	// 적의 추측을 의심 장소에 옮겨 적음 (위치, 반경, 타원)
	void ApplyGuess(FSSSuspectSpot& Spot, const FSSEnemyGuess& Guess)
	{
		Spot.Position = Guess.Position;
		Spot.Radius = float(Guess.OvalSize.X);
		Spot.OvalSize = Guess.OvalSize;
		Spot.OvalAngle = Guess.OvalAngle;
	}
}

int32 FSSSuspectMemory::AddBurst(TArray<FSSSuspectSpot>& Spots, const TArray<FVector2D>& Sensors,
                                 const TArray<FSSSensorReading>& Readings, float NoiseSigma)
{
	FSSEnemyGuess Guess = FSSSignalFinder::Find(Sensors, Readings, NoiseSigma);
	if (!Guess.bFound) return INDEX_NONE;

	int32 BestIndex = INDEX_NONE;
	double BestDist = TNumericLimits<double>::Max();

	for (int32 i = 0; i < Spots.Num(); ++i)
	{
		const double Dist = FVector2D::Distance(Guess.Position, Spots[i].Position);
		const double Gate = 2.5 * FMath::Sqrt(Spots[i].Radius * Spots[i].Radius + Guess.OvalSize.X * Guess.OvalSize.X);

		if (Dist < Gate && Dist < BestDist)
		{
			BestDist = Dist;
			BestIndex = i;
		}
	}
	if (BestIndex != INDEX_NONE)
	{
		FSSSuspectSpot& Spot = Spots[BestIndex];
		Spot.Readings.Append(Readings);
		Spot.Bursts++;
		ApplyGuess(Spot, FSSSignalFinder::Find(Sensors, Spot.Readings, NoiseSigma));
		return BestIndex;
	}

	FSSSuspectSpot NewSpot;
	NewSpot.Readings = Readings;
	ApplyGuess(NewSpot, Guess);
	NewSpot.Bursts = 1;
	return Spots.Add(NewSpot);
}

void FSSSuspectMemory::DecayDaily(TArray<FSSSuspectSpot>& Spots, float Factor, float ForgetWeight)
{
	// 뒤에서부터 돌아야 지워도 번호가 안 꼬임
	for (int32 i = Spots.Num() - 1; i >= 0; --i)
	{
		TArray<FSSSensorReading>& Readings = Spots[i].Readings;

		for (FSSSensorReading& Reading : Readings)
		{
			Reading.Weight *= Factor;
		}

		// 믿음이 거의 없어진 값은 잊음
		Readings.RemoveAll([ForgetWeight](const FSSSensorReading& Reading)
		{
			return Reading.Weight < ForgetWeight;
		});

		if (Readings.IsEmpty())
		{
			Spots.RemoveAt(i);
		}
	}
}

void FSSSuspectMemory::Refresh(TArray<FSSSuspectSpot>& Spots, const TArray<FVector2D>& Sensors, float NoiseSigma)
{
	for (int32 i = Spots.Num() - 1; i >= 0; --i)
	{
		FSSSuspectSpot& Spot = Spots[i];
		const FSSEnemyGuess Guess = FSSSignalFinder::Find(Sensors, Spot.Readings, NoiseSigma);

		// 믿음이 줄어든 만큼 반경이 커짐 (덜 확신)
		if (!Guess.bFound)
		{
			Spots.RemoveAt(i);
			continue;
		}
		ApplyGuess(Spot, Guess);
	}
}

bool FSSSuspectMemory::IsInsideOval(const FSSSuspectSpot& Spot, const FVector2D& Point)
{
	// 타원 중심에서 점까지
	const FVector2D Offset = Point - Spot.Position;

	// 타원 기울기만큼 되돌려서, 긴 반지름이 X축에 오게 돌림
	const double Cos = FMath::Cos(double(Spot.OvalAngle));
	const double Sin = FMath::Sin(double(Spot.OvalAngle));
	const double AlongLong  =  Offset.X * Cos + Offset.Y * Sin;
	const double AlongShort = -Offset.X * Sin + Offset.Y * Cos;

	// 반지름이 0이면 나눌 수 없어서 아주 작은 값으로 막음
	const double LongR  = FMath::Max(double(Spot.OvalSize.X), 1e-3);
	const double ShortR = FMath::Max(double(Spot.OvalSize.Y), 1e-3);

	// 타원 방정식: (x/a)² + (y/b)² ≤ 1 이면 안쪽
	return FMath::Square(AlongLong / LongR) + FMath::Square(AlongShort / ShortR) <= 1.0;
}
