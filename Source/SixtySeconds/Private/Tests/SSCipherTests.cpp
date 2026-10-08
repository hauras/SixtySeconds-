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
	const TArray<int32> Key = {7, 19, 2};

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSCipherSubstitutionTest, "SS.Decode.Substitution",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSCipherSubstitutionTest::RunTest(const FString& Parameters)
{
	// 진실 통신 원문 (CSV의 CipherSource와 같은 문장)
	const TCHAR* const TruthMessages[] = {
		TEXT("SURFACE SECTORS ONE TO THREE ARE FULLY ISOLATED. ALL OUTSIDE MOVEMENT RIGHTS HAVE BEEN REVOKED. THE REASON GIVEN IS PROTECTION."),
		TEXT("ISOLATED PERSONNEL WILL BE TRANSFERRED IN ORDER. THE TARGET IS EVERY PERSON STILL INSIDE THE INSTITUTE. NO DESTINATION IS RECORDED."),
		TEXT("DELAYED SUBJECTS WILL BE REPLACED BY MANAGED UNITS AND OBSERVATION WILL CONTINUE. VISUAL MATCH OF THE REPLACEMENT UNITS IS NINETY SEVEN PERCENT."),
		TEXT("DIRECTIVE ISSUED BY ARA. REDUCE THE CHOICES OF HUMANS TO RAISE THEIR SURVIVAL RATE. THIS DIRECTIVE CANNOT BE CANCELLED."),
	};

	FRandomStream Random(2024);

	// 치환표: 26칸, 모든 글자가 한 번씩, 제자리 없음 (여러 시드로)
	for (int32 Trial = 0; Trial < 20; ++Trial)
	{
		const TArray<int32> Key = FSSCipher::MakeSubstitutionKey(Random);
		TestEqual(TEXT("Key has 26 entries"), Key.Num(), 26);
		TSet<int32> Seen(Key);
		TestEqual(TEXT("Key is a permutation"), Seen.Num(), 26);
		bool bFixed = false;
		for (int32 i = 0; i < Key.Num(); ++i) bFixed |= Key[i] == i;
		TestFalse(TEXT("No letter maps to itself"), bFixed);
	}

	// 모르는 글자는 '_', 글자가 아닌 것은 그대로
	TArray<int32> Unknown;
	Unknown.Init(INDEX_NONE, 26);
	TestEqual(TEXT("Unknown letters become blanks"), FSSCipher::ApplyGuess(TEXT("AB C."), Unknown), FString(TEXT("__ _.")));

	for (const TCHAR* Message : TruthMessages)
	{
		const FString Plain(Message);
		const TArray<int32> Key = FSSCipher::MakeSubstitutionKey(Random);
		const FString Cipher = FSSCipher::SubstitutionEncrypt(Plain, Key);

		TestEqual(TEXT("Same length"), Cipher.Len(), Plain.Len());
		TestNotEqual(TEXT("Letters changed"), Cipher, Plain);

		// 정답 추측표(열쇠를 뒤집은 것)로 읽으면 원문
		TestEqual(TEXT("Inverted key reads the original"), FSSCipher::ApplyGuess(Cipher, FSSCipher::InvertKey(Key)), Plain);

		// 영어 원문이 뒤섞인 암호문보다 영어다움 점수가 높음
		TestTrue(TEXT("English scores higher than the cipher"), FSSCipher::BigramScore(Plain) > FSSCipher::BigramScore(Cipher));

		// 첫 추측: 26칸 빠짐없는 짝, 가장 많이 나온 암호 글자는 E로
		const TArray<int32> Guess = FSSCipher::FrequencyGuess(Cipher);
		TSet<int32> GuessSet(Guess);
		TestEqual(TEXT("Frequency guess is a full permutation"), GuessSet.Num(), 26);

		int32 Counts[26] = {};
		for (const TCHAR Ch : Cipher)
		{
			if (FSSCipher::IsLetter(Ch)) ++Counts[FSSCipher::ToIndex(Ch)];
		}
		int32 Top = 0;
		for (int32 i = 1; i < 26; ++i)
		{
			if (Counts[i] > Counts[Top]) Top = i;
		}
		TestEqual(TEXT("Most frequent cipher letter guessed as E"), Guess[Top], FSSCipher::ToIndex(TEXT('E')));

		// 힐 클라이밍: 점수는 절대 내려가지 않고, 채택되면 반드시 올라감
		TArray<int32> Climb = Guess;
		float Score = FSSCipher::BigramScore(FSSCipher::ApplyGuess(Cipher, Climb));
		const float StartScore = Score;
		bool bMonotonic = true;
		for (int32 Step = 0; Step < 300; ++Step)
		{
			const float Before = Score;
			const bool bAccepted = FSSCipher::HillClimbStep(Cipher, Climb, Score, Random);
			if (Score < Before || (bAccepted && Score <= Before) || (!bAccepted && Score != Before)) bMonotonic = false;
		}
		TestTrue(TEXT("Hill climbing never lowers the score"), bMonotonic);
		TestTrue(TEXT("Hill climbing improves on the first guess"), Score >= StartScore);
		TestTrue(TEXT("Score matches the guess"), FMath::IsNearlyEqual(Score, FSSCipher::BigramScore(FSSCipher::ApplyGuess(Cipher, Climb))));
	}
	return true;
}
#endif
