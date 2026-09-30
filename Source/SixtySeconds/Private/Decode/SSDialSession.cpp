#include "Decode/SSDialSession.h"
#include "Decode/SSCipher.h"
#include "Comms/SSCommsState.h"
#include "Item/SSRunSubsystem.h"

bool USSDialSession::Initialize(USSRunSubsystem* InRun, int32 InPendingIndex)
{
	if (!IsValid(InRun)) return false;

	// 대기함에서 풀 메시지를 찾음
	const TArray<FSSPendingMessage>& PendingList = InRun->GetComms()->GetPendingMessages();
	if (!PendingList.IsValidIndex(InPendingIndex)) return false;

	const FSSPendingMessage& Pending = PendingList[InPendingIndex];

	Run = InRun;
	PendingIndex = InPendingIndex;
	Key = Pending.Key;
	Dials = Pending.Dials;
	bUnlocked = false;

	if (Key.IsEmpty()) return false;

	// 저장된 다이얼 개수가 열쇠와 다르면(예전 데이터 등) 0부터 다시
	if (Dials.Num() != Key.Num())
	{
		Dials.Init(0, Key.Num());
	}

	// 암호문: 영어 원문을 정답 열쇠로 암호화
	Cipher = FSSCipher::VigenereEncrypt(Pending.Row.CipherSource, Key);

	// 다이얼마다 26칸 카이제곱을 미리 계산 (ChiTable[다이얼][위치])
	ChiTable.Reset();
	for (int32 DialIndex = 0; DialIndex < Key.Num(); ++DialIndex)
	{
		TArray<float>& Row = ChiTable.AddDefaulted_GetRef();
		for (int32 Shift = 0; Shift < 26; ++Shift)
		{
			Row.Add(FSSCipher::DialChiSquare(Cipher, Key.Num(), DialIndex, Shift));
		}
	}

	return true;
}

void USSDialSession::TurnDial(int32 DialIndex, int32 Step)
{
	// 풀린 뒤엔 못 돌림
	if (bUnlocked || !Dials.IsValidIndex(DialIndex)) return;

	// 0~25 안에서 돌기 (0에서 왼쪽으로 돌면 25)
	Dials[DialIndex] = (Dials[DialIndex] + Step % 26 + 26) % 26;

	// 창을 닫아도 이어서 풀 수 있게 대기함에 저장
	Run->GetComms()->SaveDials(PendingIndex, Dials);

	CheckUnlock();
	OnDialChanged.Broadcast();
}

bool USSDialSession::UseHint()
{
	if (bUnlocked || !IsValid(Run)) return false;

	// 아직 틀린 첫 번째 다이얼
	int32 Wrong = INDEX_NONE;
	for (int32 DialIndex = 0; DialIndex < Dials.Num(); ++DialIndex)
	{
		if (Dials[DialIndex] != Key[DialIndex])
		{
			Wrong = DialIndex;
			break;
		}
	}
	if (Wrong == INDEX_NONE) return false;

	// 행동력 1 (부족하면 힌트 없음)
	if (!Run->SpendActionPoints(1)) return false;

	// 정답 바로 옆으로. 이미 그 자리면 반대쪽 옆 (마지막 한 칸은 직접 돌리게)
	const int32 Right = (Key[Wrong] + 1) % 26;
	const int32 Left = (Key[Wrong] + 25) % 26;
	Dials[Wrong] = Dials[Wrong] == Right ? Left : Right;

	Run->GetComms()->SaveDials(PendingIndex, Dials);
	OnDialChanged.Broadcast();
	return true;
}

int32 USSDialSession::GetDial(int32 DialIndex) const
{
	return Dials.IsValidIndex(DialIndex) ? Dials[DialIndex] : 0;
}

FString USSDialSession::GetShownText() const
{
	// 지금 다이얼로 되돌린 문장 (정답이면 원문)
	return FSSCipher::VigenereDecrypt(Cipher, Dials);
}

float USSDialSession::GetDialStrength(int32 DialIndex) const
{
	if (bUnlocked) return 1.f;
	if (!ChiTable.IsValidIndex(DialIndex) || !Dials.IsValidIndex(DialIndex)) return 0.f;

	// 이 다이얼의 26칸 중 가장 작은/큰 카이제곱
	const TArray<float>& Row = ChiTable[DialIndex];
	float Min = TNumericLimits<float>::Max();
	float Max = TNumericLimits<float>::Lowest();
	for (const float Chi : Row)
	{
		Min = FMath::Min(Min, Chi);
		Max = FMath::Max(Max, Chi);
	}
	if (Max - Min < KINDA_SMALL_NUMBER) return 0.f;

	// 카이제곱은 작을수록 영어다움 → 가장 작으면 1, 가장 크면 0
	return (Max - Row[Dials[DialIndex]]) / (Max - Min);
}

void USSDialSession::CheckUnlock()
{
	// 다이얼이 전부 정답일 때만 (개수와 값이 모두 같아야 ==)
	if (Dials != Key) return;

	bUnlocked = true;

	// 공개 + 효과 + 저널 + 대기함에서 제거
	Run->GetComms()->DecodeMessage(PendingIndex);

	// 대기함에서 빠졌으니 더 이상 그 번호가 아님
	PendingIndex = INDEX_NONE;
}
