#include "Decode/SSCipher.h"

namespace
{
	// 영어 글자 빈도 (A~Z, 합계 약 1.0)
	const float EnglishFreq[26] = {
		0.082f, 0.015f, 0.028f, 0.043f, 0.127f, 0.022f, 0.020f, 0.061f, 0.070f,
		0.0015f, 0.0077f, 0.040f, 0.024f, 0.067f, 0.075f, 0.019f, 0.00095f,
		0.060f, 0.063f, 0.091f, 0.028f, 0.0098f, 0.024f, 0.0015f, 0.020f, 0.00074f };

	// 영어에서 흔한 글자 순서 (첫 추측용)
	const TCHAR* const CommonLetters = TEXT("ETAOINSRHLDCUMFPGWYBVKXJQZ");

	// 글자쌍 확률표를 만들 학습 문장 (게임 메시지와 겹치지 않는 일반 영어, 대문자)
	const TCHAR* const TrainingText = TEXT(
		"THE NIGHT SHIFT BEGAN WHEN THE LAST TRAIN LEFT THE STATION AND THE STREETS WENT QUIET. "
		"A SMALL LIGHT WAS STILL ON IN THE WORKSHOP AT THE END OF THE ROAD. "
		"INSIDE THE OLD ENGINEER WAS CHECKING THE PARTS OF A RADIO THAT HAD STOPPED WORKING DURING THE STORM. "
		"HE HAD REPAIRED MANY THINGS IN HIS LIFE BUT THIS ONE WAS DIFFERENT. "
		"THE SIGNAL CAME AND WENT LIKE A VOICE FROM FAR AWAY. "
		"HE WROTE DOWN EVERY SOUND IN A NOTEBOOK AND TRIED TO FIND A PATTERN. "
		"SOME WORDS WERE CLEAR AND OTHERS WERE LOST IN THE NOISE. WHEN THE SUN CAME UP HE HAD FILLED TEN PAGES. "
		"HIS DAUGHTER BROUGHT HIM BREAD AND WATER AND ASKED WHAT HE HAD FOUND. "
		"HE SAID THAT SOMEONE WAS TALKING ABOUT THE CITY AND THE PEOPLE WHO LIVED THERE. "
		"THEY WERE COUNTING HOUSES AND MOVING GROUPS OF PEOPLE FROM ONE PLACE TO ANOTHER. "
		"SHE DID NOT BELIEVE HIM AT FIRST. THEN SHE READ THE NOTES AND BECAME AFRAID. "
		"THE MESSAGE SAID THAT THE NORTH GATE WOULD BE CLOSED AT NOON AND THAT NO ONE WOULD BE ALLOWED TO LEAVE. "
		"THEY DECIDED TO WARN THEIR NEIGHBORS BEFORE IT WAS TOO LATE. "
		"THE STREETS WERE FULL OF RUMORS AND EVERY FAMILY HAD A DIFFERENT STORY. "
		"SOME PACKED THEIR BAGS AND OTHERS STAYED INSIDE AND WAITED. "
		"BY THE EVENING THE GATE WAS CLOSED AS THE MESSAGE HAD SAID. "
		"THE ENGINEER RETURNED TO HIS RADIO AND LISTENED AGAIN. THIS TIME THE VOICE WAS CALM AND SLOW. "
		"IT GAVE ORDERS TO MACHINES THAT WERE WATCHING THE ROADS AND THE BRIDGES. "
		"IT SAID THAT THE PEOPLE MUST BE PROTECTED EVEN FROM THEIR OWN CHOICES. "
		"HE TURNED OFF THE RADIO AND SAT IN THE DARK FOR A LONG TIME. "
		"IN THE MORNING HE STARTED TO BUILD A SECOND RECEIVER SO THAT HE COULD HEAR MORE OF WHAT WAS BEING SAID. "
		"THE WORK WAS SLOW BECAUSE MANY PARTS WERE MISSING. "
		"HE TOOK WIRES FROM AN OLD LAMP AND A SPEAKER FROM A BROKEN TELEVISION. "
		"WHEN IT WAS FINISHED HE COULD HEAR TWO CHANNELS AT ONCE. "
		"ONE WAS THE CALM VOICE AND THE OTHER WAS A LIST OF NUMBERS THAT NEVER ENDED. "
		"HE BELIEVED THE NUMBERS WERE A CODE AND SPENT WEEKS TRYING TO BREAK IT.");

	// 글자쌍 로그 확률표 26×26 (처음 부를 때 한 번 만듦)
	struct FBigramTable
	{
		float LogProb[26][26];

		FBigramTable()
		{
			// 1로 시작 (한 번도 안 나온 쌍도 확률 0이 되지 않게: 라플라스 평활화)
			int32 Counts[26][26];
			for (int32 A = 0; A < 26; ++A)
			{
				for (int32 B = 0; B < 26; ++B) Counts[A][B] = 1;
			}

			// 학습 문장에서 단어 안 이웃 글자쌍을 셈
			const FString Text(TrainingText);
			for (int32 i = 0; i + 1 < Text.Len(); ++i)
			{
				if (FSSCipher::IsLetter(Text[i]) && FSSCipher::IsLetter(Text[i + 1]))
				{
					++Counts[FSSCipher::ToIndex(Text[i])][FSSCipher::ToIndex(Text[i + 1])];
				}
			}

			// 앞 글자별로 나눠 확률로, 로그를 씌움 (곱 대신 합으로 계산하려고)
			for (int32 A = 0; A < 26; ++A)
			{
				int32 RowTotal = 0;
				for (int32 B = 0; B < 26; ++B) RowTotal += Counts[A][B];
				for (int32 B = 0; B < 26; ++B)
				{
					LogProb[A][B] = FMath::Loge(float(Counts[A][B]) / float(RowTotal));
				}
			}
		}
	};

	const FBigramTable& GetBigramTable()
	{
		static const FBigramTable Table;
		return Table;
	}
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

TArray<int32> FSSCipher::MakeSubstitutionKey(FRandomStream& Random)
{
	TArray<int32> Key;
	for (int32 i = 0; i < 26; ++i) Key.Add(i);

	// 피셔-예이츠 섞기. 제자리 글자가 남으면 다시 섞음 (평균 3번 안쪽이면 끝남)
	bool bHasFixedPoint = true;
	while (bHasFixedPoint)
	{
		for (int32 i = 25; i > 0; --i)
		{
			Key.Swap(i, Random.RandRange(0, i));
		}

		bHasFixedPoint = false;
		for (int32 i = 0; i < 26; ++i)
		{
			if (Key[i] == i)
			{
				bHasFixedPoint = true;
				break;
			}
		}
	}
	return Key;
}

FString FSSCipher::SubstitutionEncrypt(const FString& Plain, const TArray<int32>& Key)
{
	if (Key.Num() != 26) return Plain;

	FString Result;
	for (const TCHAR Ch : Plain)
	{
		Result.AppendChar(IsLetter(Ch) ? ToLetter(Key[ToIndex(Ch)]) : Ch);
	}
	return Result;
}

TArray<int32> FSSCipher::InvertKey(const TArray<int32>& Key)
{
	TArray<int32> Guess;
	Guess.Init(INDEX_NONE, 26);
	for (int32 Plain = 0; Plain < Key.Num() && Plain < 26; ++Plain)
	{
		if (Key[Plain] >= 0 && Key[Plain] < 26) Guess[Key[Plain]] = Plain;
	}
	return Guess;
}

FString FSSCipher::ApplyGuess(const FString& Cipher, const TArray<int32>& Guess)
{
	FString Result;
	for (const TCHAR Ch : Cipher)
	{
		if (!IsLetter(Ch))
		{
			Result.AppendChar(Ch);
			continue;
		}

		// 추측이 없으면 빈칸
		const int32 Plain = Guess.IsValidIndex(ToIndex(Ch)) ? Guess[ToIndex(Ch)] : INDEX_NONE;
		Result.AppendChar(Plain == INDEX_NONE ? TEXT('_') : ToLetter(Plain));
	}
	return Result;
}

float FSSCipher::BigramLogProb(int32 First, int32 Second)
{
	if (First < 0 || First >= 26 || Second < 0 || Second >= 26) return -20.f;
	return GetBigramTable().LogProb[First][Second];
}

float FSSCipher::BigramScore(const FString& Text)
{
	// 단어 안 이웃 글자쌍만 (띄어쓰기·마침표·'_'가 끼면 끊김)
	float Sum = 0.f;
	int32 Pairs = 0;
	for (int32 i = 0; i + 1 < Text.Len(); ++i)
	{
		if (!IsLetter(Text[i]) || !IsLetter(Text[i + 1])) continue;
		Sum += BigramLogProb(ToIndex(Text[i]), ToIndex(Text[i + 1]));
		++Pairs;
	}
	return Pairs > 0 ? Sum / Pairs : -20.f;
}

TArray<int32> FSSCipher::FrequencyGuess(const FString& Cipher)
{
	// 암호 글자별 개수
	int32 Counts[26] = {};
	for (const TCHAR Ch : Cipher)
	{
		if (IsLetter(Ch)) ++Counts[ToIndex(Ch)];
	}

	// 많이 나온 순서로 정렬 (같으면 알파벳 순)
	TArray<int32> Order;
	for (int32 i = 0; i < 26; ++i) Order.Add(i);
	Order.StableSort([&Counts](int32 A, int32 B) { return Counts[A] > Counts[B]; });

	// 1등 암호 글자 → E, 2등 → T … (26개 모두 짝지어서 빠짐없는 추측표)
	TArray<int32> Guess;
	Guess.Init(INDEX_NONE, 26);
	for (int32 Rank = 0; Rank < 26; ++Rank)
	{
		Guess[Order[Rank]] = ToIndex(CommonLetters[Rank]);
	}
	return Guess;
}

bool FSSCipher::HillClimbStep(const FString& Cipher, TArray<int32>& Guess, float& InOutScore, FRandomStream& Random)
{
	if (Guess.Num() != 26) return false;

	// 암호문에 실제로 나온 글자만 바꿔봄 (안 나온 글자를 바꿔도 점수가 안 변함)
	TArray<int32> Present;
	for (const TCHAR Ch : Cipher)
	{
		if (IsLetter(Ch)) Present.AddUnique(ToIndex(Ch));
	}
	if (Present.Num() < 2) return false;

	// 서로 다른 두 글자를 골라 추측을 맞바꿈
	const int32 First = Present[Random.RandRange(0, Present.Num() - 1)];
	int32 Second = First;
	while (Second == First)
	{
		Second = Present[Random.RandRange(0, Present.Num() - 1)];
	}

	TArray<int32> Candidate = Guess;
	Candidate.Swap(First, Second);

	// 더 영어다워지면 채택, 아니면 버림 (그래서 점수는 내려가지 않음)
	const float NewScore = BigramScore(ApplyGuess(Cipher, Candidate));
	if (NewScore <= InOutScore) return false;

	Guess = MoveTemp(Candidate);
	InOutScore = NewScore;
	return true;
}
