#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Math/RandomStream.h"
#include "SSTruthSession.generated.h"

class USSRunSubsystem;

// 진실 해독 상태가 바뀜 (자동 해독 걸음, 추측 입력, 잠금 해제). 화면이 듣고 다시 그림
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSSOnTruthChanged);

// ─────────────────────────────────────────────
// 진실 통신 해독 한 판 (치환 암호)
// 1단계 자동 해독: 빈도로 첫 추측 → 힐 클라이밍(글자쌍 점수가 오르는 쪽으로 글자 맞바꾸기)을 화면에 보여줌
// 멈춤: 짧은 문장에선 해독기가 엉뚱한 곳에 멈출 수 있어서, 해독기가 맞힌 글자를 우선으로
//       정답률 StopCoverage(65%)까지만 남기고 나머지는 빈칸으로 비움 (결과를 안정적으로)
// 2단계 플레이어: 빈칸에 글자를 넣음. 정답률 UnlockCoverage(80%)가 되면 잠금 해제
// 시간제한 없음, 닫아도 진행 저장
// ─────────────────────────────────────────────
UCLASS()
class SIXTYSECONDS_API USSTruthSession : public UObject
{
	GENERATED_BODY()

public:
	// 자동 해독기가 멈추는 정답률, 잠금이 풀리는 정답률 (글자 등장 횟수 기준)
	static constexpr float StopCoverage = 0.65f;
	static constexpr float UnlockCoverage = 0.8f;

	// 해독 시작. 대기함 번호가 잘못됐거나 진실 통신이 아니면 false
	bool Initialize(USSRunSubsystem* InRun, int32 InPendingIndex);

	// ── 1단계: 자동 해독 ──

	// 자동 해독 시작 (빈도로 첫 추측). 이미 돌렸으면 아무 일 없음
	void StartSolver();

	// 힐 클라이밍 Steps걸음. 채택된 걸음 수를 돌려줌
	int32 StepSolver(int32 Steps);

	// 자동 해독 끝: 멈춤 규칙대로 추측표를 정리하고 저장 → 2단계로
	void FinishSolver();

	// ── 2단계: 빈칸 채우기 ──

	// 암호 글자 CipherLetter를 원래 글자 PlainLetter로 추측 (INDEX_NONE이면 지움)
	// 같은 원래 글자를 다른 칸에 넣어뒀으면 그 칸은 비움. 자동 해독 전이거나 풀렸으면 false
	bool SetGuess(int32 CipherLetter, int32 PlainLetter);

	// ── 화면이 읽는 값 ──

	// 지금 보이는 문장 (자동 해독 중엔 해독기의 추측, 끝나면 플레이어 추측표)
	FString GetShownText() const;

	// 암호문, 암호문에 나온 글자들(많이 나온 순)
	const FString& GetCipher() const { return Cipher; }
	const TArray<int32>& GetCipherLetters() const { return CipherLetters; }
	int32 GetLetterCount(int32 CipherLetter) const
	{
		return CipherLetter >= 0 && CipherLetter < 26 ? LetterCounts[CipherLetter] : 0;
	}

	// 암호 글자 하나의 지금 추측 (없으면 INDEX_NONE)
	int32 GetGuessFor(int32 CipherLetter) const;

	// 영어다움 0~1 (원문 점수를 1로 맞춘 비율)
	float GetFitness() const;

	bool IsSolverRunning() const { return bSolverRunning; }
	bool IsSolverDone() const { return bSolverDone; }
	bool IsUnlocked() const { return bUnlocked; }
	int32 GetSolverTries() const { return SolverTries; }
	int32 GetSolverAccepted() const { return SolverAccepted; }

	// 숨겨진 정답률 (글자 등장 횟수 기준, 테스트·디버그용. 화면에는 보여주지 않음)
	float GetCorrectRatio() const;

	// 난수 고정 (테스트용)
	void SetSeed(int32 Seed) { Random.Initialize(Seed); }

	UPROPERTY(BlueprintAssignable, Category="SS|Decode")
	FSSOnTruthChanged OnTruthChanged;

private:
	// 추측표 Test의 정답률 (글자 등장 횟수 기준)
	float CorrectRatioOf(const TArray<int32>& Test) const;

	// 정답률이 UnlockCoverage 이상이면 잠금 해제하고 해독 성공 처리
	void CheckUnlock();

	UPROPERTY()
	TObjectPtr<USSRunSubsystem> Run;

	int32 PendingIndex = INDEX_NONE;

	// 암호문, 정답 추측표(열쇠를 뒤집은 것), 암호문에 나온 글자별 개수
	FString Cipher;
	TArray<int32> Answer;
	int32 LetterCounts[26] = {};
	int32 TotalLetters = 0;

	// 암호문에 나온 글자들 (많이 나온 순)
	TArray<int32> CipherLetters;

	// 플레이어 추측표 (자동 해독이 끝난 뒤부터 씀)
	TArray<int32> Guess;

	// 자동 해독기의 추측표와 점수
	TArray<int32> SolverGuess;
	float SolverScore = 0.f;
	int32 SolverTries = 0;
	int32 SolverAccepted = 0;

	// 영어다움 비율의 기준: 원문 점수(1), 바닥 점수(0)
	float BestScore = 0.f;
	float FloorScore = -6.f;

	bool bSolverRunning = false;
	bool bSolverDone = false;
	bool bUnlocked = false;

	FRandomStream Random = FRandomStream(FPlatformTime::Cycles());
};
