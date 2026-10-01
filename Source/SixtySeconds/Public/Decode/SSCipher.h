#pragma once

#include "CoreMinimal.h"
#include "Math/RandomStream.h"

// ─────────────────────────────────────────────
// 암호 계산 (감청 해독)
// 비즈네르(일반 통신 다이얼), 치환(진실 통신), 영어다움 점수
// 부모 없음. 기억 없이 계산만 하는 함수 모음
// ─────────────────────────────────────────────
class SIXTYSECONDS_API FSSCipher
{
public:
	// 대문자 A~Z인지
	static bool IsLetter(TCHAR Ch);

	// 'A' → 0, 'B' → 1 … 'Z' → 25
	static int32 ToIndex(TCHAR Ch);

	// 0 → 'A' … 25 → 'Z'
	static TCHAR ToLetter(int32 Index);

	// 비즈네르 암호화: 글자마다 Key[0], Key[1], Key[2], Key[0] … 만큼 뒤로 밈
	// 글자가 아닌 것(띄어쓰기·마침표)은 그대로, 다이얼 순서도 안 넘어감
	static FString VigenereEncrypt(const FString& Plain, const TArray<int32>& Key);

	// 비즈네르 되돌리기: 다이얼 값만큼 앞으로 당김 (다이얼이 정답이면 원문이 나옴)
	static FString VigenereDecrypt(const FString& Cipher, const TArray<int32>& Key);

	// 다이얼 DialIndex가 맡은 글자들만 Shift만큼 되돌렸을 때, 영어 글자 빈도와 얼마나 다른지
	// 작을수록 영어 같음 (정답 Shift에서 가장 작아짐)
	static float DialChiSquare(const FString& Cipher, int32 KeyLength, int32 DialIndex, int32 Shift);

	// ── B. 치환 (진실 통신) ──
	// 열쇠(Key)와 추측표(Guess)는 26칸 배열
	//   Key[원래 글자] = 암호 글자      (암호화할 때)
	//   Guess[암호 글자] = 원래 글자    (읽을 때, 모르면 INDEX_NONE)

	// 무작위 치환표. 제자리(A→A 같은)로 남는 글자가 없게 섞음
	static TArray<int32> MakeSubstitutionKey(FRandomStream& Random);

	// 치환 암호화: 글자마다 Key대로 바꿈, 글자가 아닌 것은 그대로
	static FString SubstitutionEncrypt(const FString& Plain, const TArray<int32>& Key);

	// 열쇠를 뒤집어 정답 추측표로 (Key[원래]=암호 → Guess[암호]=원래)
	static TArray<int32> InvertKey(const TArray<int32>& Key);

	// 추측표로 암호문 읽기. 모르는 글자(INDEX_NONE)는 '_'
	static FString ApplyGuess(const FString& Cipher, const TArray<int32>& Guess);

	// ── C. 영어다움 점수 (진실 통신 자동 해독기) ──

	// 글자쌍 First→Second가 영어에서 나올 확률의 로그 (학습 문장으로 만든 표, 한 번만 계산)
	static float BigramLogProb(int32 First, int32 Second);

	// 문장이 얼마나 영어다운지: 단어 안 이웃 글자쌍 로그 확률의 평균 ('_'가 낀 쌍은 뺌)
	// 클수록 영어다움. 셀 쌍이 없으면 매우 작은 값
	static float BigramScore(const FString& Text);

	// 첫 추측: 암호문에서 많이 나온 글자부터 영어에서 흔한 글자(E, T, A, O …)를 대입
	static TArray<int32> FrequencyGuess(const FString& Cipher);

	// 힐 클라이밍 한 걸음: 암호문에 나온 글자 두 개의 추측을 맞바꿔보고, 점수가 오르면 채택
	//   InOutScore : 지금 추측의 점수 (채택되면 새 점수로 바뀜)
	// 반환: 채택했으면 true (점수는 절대 내려가지 않음)
	static bool HillClimbStep(const FString& Cipher, TArray<int32>& Guess, float& InOutScore, FRandomStream& Random);
};
