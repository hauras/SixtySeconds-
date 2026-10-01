#include "Decode/SSTruthSession.h"
#include "Decode/SSCipher.h"
#include "Comms/SSCommsState.h"
#include "Item/SSRunSubsystem.h"

bool USSTruthSession::Initialize(USSRunSubsystem* InRun, int32 InPendingIndex)
{
	if (!IsValid(InRun)) return false;

	const TArray<FSSPendingMessage>& PendingList = InRun->GetComms()->GetPendingMessages();
	if (!PendingList.IsValidIndex(InPendingIndex)) return false;

	const FSSPendingMessage& Pending = PendingList[InPendingIndex];
	if (Pending.Row.Kind != ESSTraceMessageKind::Truth || Pending.SubKey.Num() != 26) return false;

	Run = InRun;
	PendingIndex = InPendingIndex;
	bUnlocked = false;
	bSolverRunning = false;
	bSolverDone = Pending.bSolverDone;

	// 암호문과 정답
	Cipher = FSSCipher::SubstitutionEncrypt(Pending.Row.CipherSource, Pending.SubKey);
	Answer = FSSCipher::InvertKey(Pending.SubKey);
	BestScore = FSSCipher::BigramScore(Pending.Row.CipherSource);

	// 저장된 추측표 (없거나 크기가 다르면 전부 모름)
	Guess = Pending.Guess;
	if (Guess.Num() != 26) Guess.Init(INDEX_NONE, 26);

	// 암호문에 나온 글자별 개수, 많이 나온 순서
	FMemory::Memzero(LetterCounts, sizeof(LetterCounts));
	TotalLetters = 0;
	for (const TCHAR Ch : Cipher)
	{
		if (!FSSCipher::IsLetter(Ch)) continue;
		++LetterCounts[FSSCipher::ToIndex(Ch)];
		++TotalLetters;
	}
	CipherLetters.Reset();
	for (int32 Letter = 0; Letter < 26; ++Letter)
	{
		if (LetterCounts[Letter] > 0) CipherLetters.Add(Letter);
	}
	CipherLetters.StableSort([this](int32 A, int32 B) { return LetterCounts[A] > LetterCounts[B]; });

	return TotalLetters > 0;
}

void USSTruthSession::StartSolver()
{
	if (bSolverDone || bSolverRunning || bUnlocked) return;

	SolverGuess = FSSCipher::FrequencyGuess(Cipher);
	SolverScore = FSSCipher::BigramScore(FSSCipher::ApplyGuess(Cipher, SolverGuess));
	SolverTries = 0;
	SolverAccepted = 0;
	bSolverRunning = true;
	OnTruthChanged.Broadcast();
}

int32 USSTruthSession::StepSolver(int32 Steps)
{
	if (!bSolverRunning) return 0;

	int32 Accepted = 0;
	for (int32 Step = 0; Step < Steps; ++Step)
	{
		++SolverTries;
		if (FSSCipher::HillClimbStep(Cipher, SolverGuess, SolverScore, Random))
		{
			++Accepted;
			++SolverAccepted;
		}
	}
	OnTruthChanged.Broadcast();
	return Accepted;
}

void USSTruthSession::FinishSolver()
{
	if (!bSolverRunning) return;
	bSolverRunning = false;

	// 멈춤 규칙: 해독기가 맞힌 글자를 먼저, 그다음 정답에서 많이 나온 글자 순으로
	//           정답률이 StopCoverage에 닿을 때까지만 채우고 나머지는 빈칸
	Guess.Init(INDEX_NONE, 26);
	int32 Covered = 0;
	const int32 Target = FMath::CeilToInt(TotalLetters * StopCoverage);

	// 1) 해독기가 맞힌 글자 (많이 나온 순)
	for (const int32 Letter : CipherLetters)
	{
		if (Covered >= Target) break;
		if (SolverGuess.IsValidIndex(Letter) && SolverGuess[Letter] == Answer[Letter])
		{
			Guess[Letter] = Answer[Letter];
			Covered += LetterCounts[Letter];
		}
	}

	// 2) 모자라면 정답에서 많이 나온 글자 순으로 보충
	for (const int32 Letter : CipherLetters)
	{
		if (Covered >= Target) break;
		if (Guess[Letter] != INDEX_NONE) continue;
		Guess[Letter] = Answer[Letter];
		Covered += LetterCounts[Letter];
	}

	bSolverDone = true;
	Run->GetComms()->SaveGuess(PendingIndex, Guess, bSolverDone);
	OnTruthChanged.Broadcast();
}

bool USSTruthSession::SetGuess(int32 CipherLetter, int32 PlainLetter)
{
	if (!bSolverDone || bUnlocked || !Guess.IsValidIndex(CipherLetter)) return false;
	if (PlainLetter != INDEX_NONE && (PlainLetter < 0 || PlainLetter >= 26)) return false;

	// 같은 원래 글자는 한 칸에만 (다른 칸에 있으면 비움)
	if (PlainLetter != INDEX_NONE)
	{
		for (int32 Letter = 0; Letter < 26; ++Letter)
		{
			if (Letter != CipherLetter && Guess[Letter] == PlainLetter) Guess[Letter] = INDEX_NONE;
		}
	}
	Guess[CipherLetter] = PlainLetter;

	Run->GetComms()->SaveGuess(PendingIndex, Guess, bSolverDone);
	CheckUnlock();
	OnTruthChanged.Broadcast();
	return true;
}

FString USSTruthSession::GetShownText() const
{
	if (bUnlocked) return FSSCipher::ApplyGuess(Cipher, Answer);
	if (bSolverRunning) return FSSCipher::ApplyGuess(Cipher, SolverGuess);
	if (bSolverDone) return FSSCipher::ApplyGuess(Cipher, Guess);

	// 자동 해독 전: 암호문 그대로
	return Cipher;
}

int32 USSTruthSession::GetGuessFor(int32 CipherLetter) const
{
	if (bSolverRunning) return SolverGuess.IsValidIndex(CipherLetter) ? SolverGuess[CipherLetter] : INDEX_NONE;
	return Guess.IsValidIndex(CipherLetter) ? Guess[CipherLetter] : INDEX_NONE;
}

float USSTruthSession::GetFitness() const
{
	if (bUnlocked) return 1.f;

	const FString Shown = GetShownText();
	const float Score = FSSCipher::BigramScore(Shown);

	// 빈칸이 많으면 셀 쌍이 적어서 점수가 부풀 수 있음 → 채운 글자 비율을 곱함
	int32 Known = 0;
	int32 All = 0;
	for (const TCHAR Ch : Shown)
	{
		if (Ch == TEXT('_')) { ++All; continue; }
		if (FSSCipher::IsLetter(Ch)) { ++Known; ++All; }
	}
	const float Filled = All > 0 ? float(Known) / All : 0.f;

	const float Range = BestScore - FloorScore;
	const float Quality = Range > KINDA_SMALL_NUMBER ? FMath::Clamp((Score - FloorScore) / Range, 0.f, 1.f) : 0.f;
	return Quality * Filled;
}

float USSTruthSession::GetCorrectRatio() const
{
	return CorrectRatioOf(bSolverRunning ? SolverGuess : Guess);
}

float USSTruthSession::CorrectRatioOf(const TArray<int32>& Test) const
{
	if (TotalLetters <= 0 || Test.Num() != 26) return 0.f;

	int32 Correct = 0;
	for (int32 Letter = 0; Letter < 26; ++Letter)
	{
		if (LetterCounts[Letter] > 0 && Test[Letter] == Answer[Letter]) Correct += LetterCounts[Letter];
	}
	return float(Correct) / TotalLetters;
}

void USSTruthSession::CheckUnlock()
{
	if (CorrectRatioOf(Guess) < UnlockCoverage) return;

	bUnlocked = true;

	// 공개 + 저널 + 진실 단서 수 + 대기함에서 제거
	Run->GetComms()->DecodeMessage(PendingIndex);
	PendingIndex = INDEX_NONE;
}
