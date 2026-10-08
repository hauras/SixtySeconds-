#include "Trace/SSSignalFinder.h"
#include "Trace/SSSuspectMemory.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace SSTraceTest
{
	// 목업과 같은 배치: 센서 세 대, 은신처 하나
	const TArray<FVector2D> Sensors = {FVector2D(70.0, 55.0), FVector2D(540.0, 60.0), FVector2D(300.0, 320.0)};
	const FVector2D Shelter(380.0, 175.0);

	// 잡음 없이 Source에서 신호를 Rounds번 보냈을 때의 측정
	TArray<FSSSensorReading> ExactReadings(const FVector2D& Source, int32 Rounds)
	{
		TArray<FSSSensorReading> Out;
		for (int32 Round = 0; Round < Rounds; ++Round)
		{
			for (int32 Index = 0; Index < Sensors.Num(); ++Index)
			{
				FSSSensorReading& M = Out.AddDefaulted_GetRef();
				M.SensorIndex = Index;
				M.Distance = static_cast<float>(FVector2D::Distance(Source, Sensors[Index]));
			}
		}
		return Out;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSSignalFinderConvergeTest, "SS.Trace.FinderConverge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSSignalFinderConvergeTest::RunTest(const FString& Parameters)
{
	using namespace SSTraceTest;

	// 측정이 3개보다 적으면 위치를 정할 수 없음
	TArray<FSSSensorReading> TooFew = ExactReadings(Shelter, 1);
	TooFew.SetNum(2);
	TestFalse(TEXT("Two measurements are not enough"), FSSSignalFinder::Find(Sensors, TooFew, 18.f).bFound);

	// 잡음이 없으면 은신처를 1m 안으로 맞힘
	const FSSEnemyGuess Once = FSSSignalFinder::Find(Sensors, ExactReadings(Shelter, 1), 18.f);
	TestTrue(TEXT("Guess is valid with three exact measurements"), Once.bFound);
	TestTrue(TEXT("Noise-free estimate lands within 1m"), FVector2D::Distance(Once.Position, Shelter) < 1.0);

	// 같은 신호를 여러 번 재도 결과는 같은 곳
	const FSSEnemyGuess Many = FSSSignalFinder::Find(Sensors, ExactReadings(Shelter, 4), 18.f);
	TestTrue(TEXT("Repeated exact measurements still land within 1m"), Many.bFound && FVector2D::Distance(Many.Position, Shelter) < 1.0);

	// 다른 위치도 맞힘 (센서 가운데에서 먼 곳)
	const FVector2D Corner(560.0, 300.0);
	const FSSEnemyGuess Far = FSSSignalFinder::Find(Sensors, ExactReadings(Corner, 1), 18.f);
	TestTrue(TEXT("Guess far from the start point within 1m"), Far.bFound && FVector2D::Distance(Far.Position, Corner) < 1.0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSFinderOvalTest, "SS.Trace.FinderOval",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSFinderOvalTest::RunTest(const FString& Parameters)
{
	using namespace SSTraceTest;

	// 같은 위치를 4배 많이 재면 공분산이 1/4 → 반지름은 절반 (잡음 없는 측정이라 항상 성립)
	const FSSEnemyGuess Once = FSSSignalFinder::Find(Sensors, ExactReadings(Shelter, 1), 18.f);
	const FSSEnemyGuess Four = FSSSignalFinder::Find(Sensors, ExactReadings(Shelter, 4), 18.f);
	TestTrue(TEXT("Ellipse has size"), Once.bFound && Once.OvalSize.X > 0.0);
	TestTrue(TEXT("Four times the measurements halves the long radius"),
		FMath::IsNearlyEqual(Four.OvalSize.X, Once.OvalSize.X * 0.5, 0.01 * Once.OvalSize.X));
	TestTrue(TEXT("Four times the measurements halves the short radius"),
		FMath::IsNearlyEqual(Four.OvalSize.Y, Once.OvalSize.Y * 0.5, 0.01 * Once.OvalSize.Y + 1e-6));

	// 센서 잡음이 두 배면 반지름도 두 배
	const FSSEnemyGuess Noisy = FSSSignalFinder::Find(Sensors, ExactReadings(Shelter, 1), 36.f);
	TestTrue(TEXT("Double noise doubles the radius"),
		FMath::IsNearlyEqual(Noisy.OvalSize.X, Once.OvalSize.X * 2.0, 0.01 * Once.OvalSize.X));

	// 센서가 모두 왼쪽에 있으면: 좌우는 잘 재고 위아래는 못 잼 → 위아래로 길쭉한 타원
	// (추정기는 센서 가운데에서 출발하므로, 가운데 센서를 대상 쪽으로 두어 게임처럼 출발점이 대상 방향에 있게 함)
	const TArray<FVector2D> LeftSensors = {FVector2D(0.0, -60.0), FVector2D(0.0, 60.0), FVector2D(200.0, 0.0)};
	const FVector2D Target(300.0, 0.0);
	TArray<FSSSensorReading> LeftReadings;
	for (int32 Index = 0; Index < LeftSensors.Num(); ++Index)
	{
		FSSSensorReading& M = LeftReadings.AddDefaulted_GetRef();
		M.SensorIndex = Index;
		M.Distance = static_cast<float>(FVector2D::Distance(Target, LeftSensors[Index]));
	}
	const FSSEnemyGuess Skewed = FSSSignalFinder::Find(LeftSensors, LeftReadings, 18.f);
	TestTrue(TEXT("Skewed layout still finds the target"), Skewed.bFound && FVector2D::Distance(Skewed.Position, Target) < 1.0);
	TestTrue(TEXT("Skewed layout gives a long thin ellipse"), Skewed.OvalSize.X > 3.0 * Skewed.OvalSize.Y);
	TestTrue(TEXT("Long radius points up-down (perpendicular to the sensors)"), FMath::Abs(FMath::Cos(Skewed.OvalAngle)) < 0.3f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSFinderMismatchTest, "SS.Trace.FinderMismatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSFinderMismatchTest::RunTest(const FString& Parameters)
{
	using namespace SSTraceTest;
	const float Sigma = 18.f;

	// 잡음이 없으면 남는 틀림도 없음 → 정규화 카이제곱 ≈ 0
	const FSSEnemyGuess Exact = FSSSignalFinder::Find(Sensors, ExactReadings(Shelter, 2), Sigma);
	TestTrue(TEXT("Noise-free measurements have near-zero chi-square"), Exact.bFound && Exact.Mismatch < 0.01f);

	// 고정 시드로 σ만큼 잡음을 넣은 은신처 신호 20번 → 정규화 카이제곱이 1 근처
	FRandomStream Random(1234);
	const auto Gaussian = [&Random]()
	{
		// 박스-뮬러: 균등 난수 두 개로 표준 정규분포 하나
		const double U1 = FMath::Max(double(Random.FRand()), 1e-12);
		const double U2 = double(Random.FRand());
		return FMath::Sqrt(-2.0 * FMath::Loge(U1)) * FMath::Cos(2.0 * PI * U2);
	};
	TArray<FSSSensorReading> Noisy = ExactReadings(Shelter, 20);
	for (FSSSensorReading& M : Noisy)
	{
		M.Distance += float(Gaussian() * Sigma);
	}
	const FSSEnemyGuess Honest = FSSSignalFinder::Find(Sensors, Noisy, Sigma);
	TestTrue(TEXT("Honest noisy measurements are consistent (0.5 ~ 1.6)"),
		Honest.bFound && Honest.Mismatch > 0.5f && Honest.Mismatch < 1.6f);

	// 고정 배치: 은신처 신호 4번 + 미끼 신호 4번 (잡음 없음)
	// 두 곳의 신호를 한 점으로 설명할 수 없으니 일치도가 크게 나빠지고, 추정이 미끼 쪽으로 끌려감
	const FVector2D Decoy(200.0, 120.0);
	TArray<FSSSensorReading> Mixed = ExactReadings(Shelter, 4);
	Mixed.Append(ExactReadings(Decoy, 4));
	const FSSEnemyGuess Fooled = FSSSignalFinder::Find(Sensors, Mixed, Sigma);
	TestTrue(TEXT("Mixed decoy signals are flagged inconsistent (> 3)"), Fooled.bFound && Fooled.Mismatch > 3.f);
	TestTrue(TEXT("Guess is pulled toward the decoy"),
		FVector2D::Distance(Fooled.Position, Decoy) < FVector2D::Distance(Shelter, Decoy));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSSuspectMemoryTest, "SS.Trace.SuspectMemory",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSSuspectMemoryTest::RunTest(const FString& Parameters)
{
	using namespace SSTraceTest;
	const float Sigma = 18.f;
	TArray<FSSSuspectSpot> Spots;

	// 읽은 값이 부족한 송신은 기억하지 않음
	TArray<FSSSensorReading> TooFew = ExactReadings(Shelter, 1);
	TooFew.SetNum(2);
	TestEqual(TEXT("Burst without enough readings is ignored"), FSSSuspectMemory::AddBurst(Spots, Sensors, TooFew, Sigma), int32(INDEX_NONE));
	TestEqual(TEXT("No spot yet"), Spots.Num(), 0);

	// 첫 송신 → 새 의심 장소
	TestEqual(TEXT("First burst makes spot 0"), FSSSuspectMemory::AddBurst(Spots, Sensors, ExactReadings(Shelter, 1), Sigma), 0);
	const float FirstRadius = Spots.Num() == 1 ? Spots[0].Radius : 0.f;

	// 같은 곳에서 또 송신 → 같은 장소에 합쳐지고 더 확신 (반경 감소)
	TestEqual(TEXT("Same place joins spot 0"), FSSSuspectMemory::AddBurst(Spots, Sensors, ExactReadings(Shelter, 1), Sigma), 0);
	TestEqual(TEXT("Still one spot"), Spots.Num(), 1);
	TestTrue(TEXT("Spot remembers two bursts and six readings"), Spots.Num() == 1 && Spots[0].Bursts == 2 && Spots[0].Readings.Num() == 6);
	TestTrue(TEXT("More bursts shrink the radius"), Spots.Num() == 1 && Spots[0].Radius < FirstRadius);
	TestTrue(TEXT("Spot stays on the shelter"), Spots.Num() == 1 && FVector2D::Distance(Spots[0].Position, Shelter) < 1.0);

	// 멀리 떨어진 미끼(고정 위치)에서 송신 → 따로 기억 (은신처 장소에 섞이지 않음)
	const FVector2D Decoy(160.0, 90.0);
	TestEqual(TEXT("Far decoy makes spot 1"), FSSSuspectMemory::AddBurst(Spots, Sensors, ExactReadings(Decoy, 1), Sigma), 1);
	TestEqual(TEXT("Two spots now"), Spots.Num(), 2);
	TestTrue(TEXT("Shelter spot was not polluted by the decoy"), Spots.Num() == 2 && Spots[0].Readings.Num() == 6);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSSuspectOvalTest, "SS.Trace.SuspectOval",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSSuspectOvalTest::RunTest(const FString& Parameters)
{
	// 가로로 긴 타원: 긴 반지름 20, 짧은 반지름 5, 중심 (100, 100)
	FSSSuspectSpot Spot;
	Spot.Position = FVector2D(100.0, 100.0);
	Spot.OvalSize = FVector2D(20.0, 5.0);
	Spot.OvalAngle = 0.f;

	TestTrue(TEXT("Center is inside"), FSSSuspectMemory::IsInsideOval(Spot, FVector2D(100.0, 100.0)));
	TestTrue(TEXT("Along the long axis is inside"), FSSSuspectMemory::IsInsideOval(Spot, FVector2D(115.0, 100.0)));
	TestFalse(TEXT("Same distance along the short axis is outside"), FSSSuspectMemory::IsInsideOval(Spot, FVector2D(100.0, 115.0)));
	TestFalse(TEXT("Past the long radius is outside"), FSSSuspectMemory::IsInsideOval(Spot, FVector2D(121.0, 100.0)));

	// 90도 돌리면 세로로 긴 타원 → 안팎이 뒤바뀜
	Spot.OvalAngle = float(PI * 0.5);
	TestTrue(TEXT("Rotated: vertical point is inside"), FSSSuspectMemory::IsInsideOval(Spot, FVector2D(100.0, 115.0)));
	TestFalse(TEXT("Rotated: horizontal point is outside"), FSSSuspectMemory::IsInsideOval(Spot, FVector2D(115.0, 100.0)));

	// 45도 대각선 타원
	Spot.OvalAngle = float(PI * 0.25);
	TestTrue(TEXT("Diagonal: point on the tilted long axis is inside"), FSSSuspectMemory::IsInsideOval(Spot, FVector2D(110.0, 110.0)));
	TestFalse(TEXT("Diagonal: point on the other diagonal is outside"), FSSSuspectMemory::IsInsideOval(Spot, FVector2D(110.0, 90.0)));

	// 기억에 넣으면 타원 정보가 채워지고, 반경 = 긴 반지름
	using namespace SSTraceTest;
	TArray<FSSSuspectSpot> Spots;
	FSSSuspectMemory::AddBurst(Spots, Sensors, ExactReadings(Shelter, 1), 18.f);
	TestTrue(TEXT("AddBurst fills the oval"), Spots.Num() == 1 && Spots[0].OvalSize.X >= Spots[0].OvalSize.Y && Spots[0].OvalSize.Y > 0.0);
	TestTrue(TEXT("Radius equals the long radius"), Spots.Num() == 1 && FMath::IsNearlyEqual(Spots[0].Radius, float(Spots[0].OvalSize.X)));

	return true;
}
#endif
