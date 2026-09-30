#pragma once

#include "CoreMinimal.h"

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
};
