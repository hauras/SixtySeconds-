#include "Decode/SSCipher.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace SSCipherTest
{
	// 일반 통신 원문 (Design/Trace/SS_TraceMessages.csv의 CipherSource와 같은 문장)
	const TCHAR* const InfoMessages[] = {
		TEXT("PATROL UNIT SHIFT CHANGE AT SIX HUNDRED HOURS. CORRIDOR B ONE IS UNWATCHED FOR TWELVE MINUTES."),
		TEXT("MAINTENANCE LOG. WATER VALVE THREE ON LEVEL B ONE WAS NOT INSPECTED. THE PIPE STILL HOLDS WATER."),
		TEXT("POWER REPORT. THE SPARE BATTERY FOR THE EMERGENCY LINE ON B ONE WAS NOT REPLACED AND REMAINS IN THE STORAGE BOX."),
		TEXT("MEDICAL INVENTORY. ONE FIRST AID KIT WAS LEFT OFF THE RECOVERY LIST AND IS STILL IN THE CLINIC."),
		TEXT("ROUTINE STATUS REPORT. ALL SECTORS REMAIN ISOLATED. NO UNUSUAL ACTIVITY WAS DETECTED TODAY."),
	};

	// 다이얼 하나에서 카이제곱이 가장 작은 Shift (0~25)
	int32 BestShift(const FString& Cipher, int32 KeyLength, int32 DialIndex)
	{
		int32 Best = 0;
		float BestChi = TNumericLimits<float>::Max();
		for (int32 Shift = 0; Shift < 26; ++Shift)
		{
			const float Chi = FSSCipher::DialChiSquare(Cipher, KeyLength, DialIndex, Shift);
			if (Chi < BestChi)
			{
				BestChi = Chi;
				Best = Shift;
			}
		}
		return Best;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSCipherLettersTest, "SS.Decode.Letters",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSCipherLettersTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("A is a letter"), FSSCipher::IsLetter(TEXT('A')));
	TestTrue(TEXT("Z is a letter"), FSSCipher::IsLetter(TEXT('Z')));
	TestFalse(TEXT("Space is not a letter"), FSSCipher::IsLetter(TEXT(' ')));
	TestFalse(TEXT("Period is not a letter"), FSSCipher::IsLetter(TEXT('.')));
	TestFalse(TEXT("Lowercase is not a letter"), FSSCipher::IsLetter(TEXT('a')));

	TestEqual(TEXT("A -> 0"), FSSCipher::ToIndex(TEXT('A')), 0);
	TestEqual(TEXT("Z -> 25"), FSSCipher::ToIndex(TEXT('Z')), 25);
	TestEqual(TEXT("2 -> C"), int32(FSSCipher::ToLetter(2)), int32(TEXT('C')));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSCipherVigenereTest, "SS.Decode.Vigenere",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSCipherVigenereTest::RunTest(const FString& Parameters)
{
	using namespace SSCipherTest;
	const TArray<int32> Key = { 7, 19, 2 };

	// 손으로 계산한 값: P+7=W, A+19=T, T+2=V, 띄어쓰기는 그대로·다이얼 순서 안 넘어감, A+7=H
	TestEqual(TEXT("Hand-checked example"), FSSCipher::VigenereEncrypt(TEXT("PAT A"), Key), FString(TEXT("WTV H")));

	// Z를 넘으면 A로 돌아옴: Y(24)+7 = 31 → 5(F)
	TestEqual(TEXT("Wraps past Z"), FSSCipher::VigenereEncrypt(TEXT("Y"), Key), FString(TEXT("F")));

	// 열쇠가 없으면 그대로
	TestEqual(TEXT("Empty key keeps text"), FSSCipher::VigenereEncrypt(TEXT("ABC"), {}), FString(TEXT("ABC")));
	TestEqual(TEXT("Empty key decrypt keeps text"), FSSCipher::VigenereDecrypt(TEXT("ABC"), {}), FString(TEXT("ABC")));

	for (const TCHAR* Message : InfoMessages)
	{
		const FString Plain(Message);
		const FString Cipher = FSSCipher::VigenereEncrypt(Plain, Key);

		// 길이·띄어쓰기·마침표는 그대로, 글자는 바뀜
		TestEqual(TEXT("Same length"), Cipher.Len(), Plain.Len());
		TestNotEqual(TEXT("Letters changed"), Cipher, Plain);

		// 정답 열쇠로 되돌리면 원문
		TestEqual(TEXT("Decrypt(Encrypt) is the original"), FSSCipher::VigenereDecrypt(Cipher, Key), Plain);

		// 다이얼마다 카이제곱이 가장 작은 곳 = 정답 다이얼 값 (신호 세기가 정답에서 최대)
		for (int32 Dial = 0; Dial < Key.Num(); ++Dial)
		{
			TestEqual(FString::Printf(TEXT("Dial %d best shift"), Dial), BestShift(Cipher, Key.Num(), Dial), Key[Dial]);
		}
	}

	// 열쇠 길이가 0 이하면 계산 안 함
	TestEqual(TEXT("Zero key length gives 0"), FSSCipher::DialChiSquare(TEXT("ABC"), 0, 0, 0), 0.f);
	return true;
}
#endif
