#include "Decode/SSCipher.h"

namespace
{
	// 영어 글자 빈도 (A~Z, 합계 약 1.0)
	const float EnglishFreq[26] = {
		0.082f, 0.015f, 0.028f, 0.043f, 0.127f, 0.022f, 0.020f, 0.061f, 0.070f,
		0.0015f, 0.0077f, 0.040f, 0.024f, 0.067f, 0.075f, 0.019f, 0.00095f,
		0.060f, 0.063f, 0.091f, 0.028f, 0.0098f, 0.024f, 0.0015f, 0.020f, 0.00074f };
}

bool FSSCipher::IsLetter(TCHAR Ch)
{
	return Ch >= TEXT('A') && Ch <= TEXT('Z');
}

int32 FSSCipher::ToIndex(TCHAR Ch)
{
	return Ch - TEXT('A');
}

TCHAR FSSCipher::ToLetter(int32 Index)
{
	return TCHAR(TEXT('A') + Index);
}

FString FSSCipher::VigenereEncrypt(const FString& Plain, const TArray<int32>& Key)
{
	// 열쇠가 없으면 암호화할 수 없어서 그대로 (Key.Num()이 0이면 % 0으로 멈춤)
	if (Key.IsEmpty()) return Plain;

	// 결과 문장
	FString Result;

	// 지나온 글자 수 (몇 번 다이얼 차례인지 정하는 번호표)
	int32 LetterCount = 0;

	for (const TCHAR Ch : Plain)
	{
		// 띄어쓰기·마침표는 그대로 붙이고, 다이얼 순서도 안 넘어감
		if (!IsLetter(Ch))
		{
			Result.AppendChar(Ch);
			continue;
		}

		// 이번 글자는 몇 번 다이얼 차례인지 (0, 1, 2, 0, 1, 2 …)
		const int32 Dial = LetterCount % Key.Num();

		// 글자 번호 + 다이얼 값, Z를 넘으면 A로 돌아옴
		const int32 NewIndex = (ToIndex(Ch) + Key[Dial]) % 26;

		Result.AppendChar(ToLetter(NewIndex));
		++LetterCount;
	}

	return Result;
}

FString FSSCipher::VigenereDecrypt(const FString& Cipher, const TArray<int32>& Key)
{
	// 7칸 당기기 = 19칸(26-7) 밀기 → 반대 열쇠로 암호화하면 원문
	TArray<int32> Reverse;
	for (const int32 K : Key)
	{
		Reverse.Add((26 - K % 26) % 26);
	}
	return VigenereEncrypt(Cipher, Reverse);
}

float FSSCipher::DialChiSquare(const FString& Cipher, int32 KeyLength, int32 DialIndex, int32 Shift)
{
	if (KeyLength <= 0) return 0.f;

	// 되돌린 글자별 개수 (A~Z), 이 다이얼이 맡은 글자 수, 지나온 글자 수
	int32 Counts[26] = {};
	int32 Total = 0;
	int32 LetterCount = 0;

	for (const TCHAR Ch : Cipher)
	{
		if (!IsLetter(Ch)) continue;

		// 이번 글자의 다이얼. 번호표는 담당 여부와 상관없이 항상 넘김 (안 그러면 순서가 밀림)
		const int32 Dial = LetterCount % KeyLength;
		++LetterCount;

		// 다른 다이얼이 맡은 글자는 건너뜀
		if (Dial != DialIndex) continue;

		// Shift만큼 되돌린 글자를 셈
		const int32 Original = (ToIndex(Ch) - Shift + 26) % 26;
		++Counts[Original];
		++Total;
	}

	if (Total == 0) return 0.f;

	// 카이제곱: 글자마다 (실제 개수 - 영어라면 나왔을 개수)² / 영어라면 나왔을 개수 의 합
	float Chi = 0.f;
	for (int32 i = 0; i < 26; ++i)
	{
		const float Expected = Total * EnglishFreq[i];
		Chi += FMath::Square(Counts[i] - Expected) / Expected;
	}

	return Chi;
}
