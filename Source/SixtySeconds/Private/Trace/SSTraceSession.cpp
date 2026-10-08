#include "Trace/SSTraceSession.h"
#include "Trace/SSTraceConfig.h"

void USSTraceSession::Initialize(USSTraceConfig* InConfig, const TArray<FSSSuspectSpot>& InSpots,
	const TArray<int32>& InBlockedRelays, int32 InReceived)
{
	Config = InConfig;
	Spots = InSpots;
	BlockedRelays = InBlockedRelays;
	NewlyBlockedRelays.Reset();
	SelectedRelay = INDEX_NONE;
	Received = FMath::Max(0, InReceived);
	Turn = 0;
	AlertLevel = 0;
	Random.GenerateNewSeed(); // 판마다 다른 잡음 (테스트는 이 뒤에 SetSeed)

	// 규칙이 없거나 센서가 3개 미만이면 위치를 잴 수 없어서 판을 열지 않음
	if (!IsValid(Config) || Config->SensorPositions.Num() < 3)
	{
		Outcome = ESSTraceOutcome::Stopped;
		return;
	}

	// 날을 넘어오며 흐려진 기억으로 위치와 반경을 다시 계산
	FSSSuspectMemory::Refresh(Spots, Config->SensorPositions, ScaledSigma(Config->NoiseSigma));

	// 지난번에 이미 다 받았으면 받을 게 없음
	Outcome = Received >= Config->ReceiveGoal ? ESSTraceOutcome::Completed : ESSTraceOutcome::InProgress;
}

bool USSTraceSession::SendDirect()
{
	if (!CanAct()) return false;

	const int32 SpotCountBefore = Spots.Num();
	++Turn;
	++Received;

	// 은신처에서 선명한 신호 → 적의 기억에 넣음
	FSSSuspectMemory::AddBurst(Spots, Config->SensorPositions,
		MakeReadings(Config->ShelterPosition, Config->NoiseSigma), ScaledSigma(Config->NoiseSigma));

	AfterSend(SpotCountBefore);
	return true;
}

bool USSTraceSession::SendViaRelay()
{
	if (!CanAct() || !HasRelay()) return false;

	const int32 SpotCountBefore = Spots.Num();
	++Turn;
	++Received;

	// 단자에서 선명한 신호
	FSSSuspectMemory::AddBurst(Spots, Config->SensorPositions,
		MakeReadings(GetRelayPosition(), Config->NoiseSigma), ScaledSigma(Config->NoiseSigma));

	// 은신처에서 새어 나간 약한 신호 (흐릿하지만 쌓임)
	FSSSuspectMemory::AddBurst(Spots, Config->SensorPositions,
		MakeReadings(Config->ShelterPosition, Config->LeakNoiseSigma), ScaledSigma(Config->NoiseSigma));

	AfterSend(SpotCountBefore);
	return true;
}

bool USSTraceSession::SelectRelay(int32 RelayIndex)
{
	if (!CanAct()) return false;

	// 맵에 있는 단자이고, 차단되지 않았어야 함
	if (!Config->RelayPositions.IsValidIndex(RelayIndex) || IsRelayBlocked(RelayIndex)) return false;

	SelectedRelay = RelayIndex;
	OnTraceChanged.Broadcast();
	return true;
}

FVector2D USSTraceSession::GetRelayPosition() const
{
	return (IsValid(Config) && Config->RelayPositions.IsValidIndex(SelectedRelay))
		? Config->RelayPositions[SelectedRelay]
		: FVector2D::ZeroVector;
}

void USSTraceSession::EndSession()
{
	if (Outcome == ESSTraceOutcome::InProgress)
	{
		Outcome = ESSTraceOutcome::Stopped;
	}
	OnTraceChanged.Broadcast();
}

bool USSTraceSession::CanAct() const
{
	return IsValid(Config) && Outcome == ESSTraceOutcome::InProgress;
}

float USSTraceSession::ScaledSigma(float Sigma) const
{
	// 경계 1단계마다 AlertNoiseScale을 곱함 (예: 0.8² = 0.64 → 오차가 줄어 적이 정밀해짐)
	return Sigma * FMath::Pow(Config->AlertNoiseScale, float(AlertLevel));
}

TArray<FSSSensorReading> USSTraceSession::MakeReadings(const FVector2D& Source, float Sigma)
{
	const float Noise = ScaledSigma(Sigma);

	// 흐릿한 신호일수록 적이 덜 믿음 (기본 오차 대비: 오차가 2배면 믿음은 1/4)
	const float Weight = FMath::Square(Config->NoiseSigma / Sigma);

	TArray<FSSSensorReading> Readings;

	for (int32 i = 0; i < Config->SensorPositions.Num(); ++i)
	{
		FSSSensorReading& Reading = Readings.AddDefaulted_GetRef();
		Reading.SensorIndex = i;
		Reading.Distance = float(FVector2D::Distance(Source, Config->SensorPositions[i]) + NextGaussian() * Noise);
		Reading.Weight = Weight;
	}

	return Readings;
}

void USSTraceSession::AfterSend(int32 SpotCountBefore)
{
	// 1. 새로운 곳에서 신호가 나와 의심 장소가 2곳 이상이 되면, 적이 눈치채고 경계를 올림
	if (Spots.Num() > SpotCountBefore && Spots.Num() >= 2)
	{
		AlertLevel = FMath::Min(AlertLevel + 1, Config->MaxAlertLevel);
	}

	// 2. 확신한 의심 장소는 적이 확인하러 옴 (뒤에서부터 돌아야 지워도 번호가 안 꼬임)
	for (int32 i = Spots.Num() - 1; i >= 0; --i)
	{
		const FSSSuspectSpot& Spot = Spots[i];
		if (Spot.Radius > Config->ConfirmRadius) continue; // 아직 확신 못 함

		// 적은 의심 범위(타원) 전체를 수색함. 은신처가 타원 안이면 → 들킴
		if (FSSSuspectMemory::IsInsideOval(Spot, Config->ShelterPosition))
		{
			Outcome = ESSTraceOutcome::Exposed;
			break;
		}

		// 타원 안에 있던 통신 단자 → 차단 (고른 단자였으면 선택도 풀림)
		for (int32 Relay = 0; Relay < Config->RelayPositions.Num(); ++Relay)
		{
			if (IsRelayBlocked(Relay)) continue;
			if (!FSSSuspectMemory::IsInsideOval(Spot, Config->RelayPositions[Relay])) continue;

			BlockedRelays.Add(Relay);
			NewlyBlockedRelays.Add(Relay);
			if (SelectedRelay == Relay) SelectedRelay = INDEX_NONE;
		}

		// 확인이 끝난 장소는 기억에서 지움 (단자였든, 아무것도 없었든)
		Spots.RemoveAt(i);
	}

	// 3. 판이 끝났는지
	if (Outcome == ESSTraceOutcome::InProgress)
	{
		if (Received >= Config->ReceiveGoal)
		{
			Outcome = ESSTraceOutcome::Completed;
		}
		else if (Turn >= Config->MaxTurns)
		{
			Outcome = ESSTraceOutcome::Stopped;
		}
	}

	OnTraceChanged.Broadcast();
}

double USSTraceSession::NextGaussian()
{
	// 균등 난수 두 개 (U1이 0이면 로그가 무한대라 아주 작은 수 밑으로 안 내려가게)
	const double U1 = FMath::Max(double(Random.FRand()), 1e-12);
	const double U2 = double(Random.FRand());

	// 박스-뮬러: 평균 0, 표준편차 1인 정규분포 난수 하나
	return FMath::Sqrt(-2.0 * FMath::Loge(U1)) * FMath::Cos(2.0 * PI * U2);
}
