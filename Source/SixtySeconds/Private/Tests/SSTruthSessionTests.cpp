#include "Decode/SSTruthSession.h"
#include "Decode/SSCipher.h"
#include "Comms/SSCommsState.h"
#include "Trace/SSTraceSession.h"
#include "Trace/SSTraceConfig.h"
#include "Trace/SSTraceMessage.h"
#include "Item/SSRunSubsystem.h"
#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace SSTruthSessionTest
{
	const TCHAR* const Plain = TEXT("DELAYED SUBJECTS WILL BE REPLACED BY MANAGED UNITS AND OBSERVATION WILL CONTINUE. VISUAL MATCH OF THE REPLACEMENT UNITS IS NINETY SEVEN PERCENT.");

	// 진실 단서 하나를 대기함에 넣은 RunSubsystem
	USSRunSubsystem* MakeRunWithTruth()
	{
		UGameInstance* GameInstance = NewObject<UGameInstance>();
		USSRunSubsystem* Run = NewObject<USSRunSubsystem>(GameInstance);
		Run->InitializeShelterStats(100.f, 100.f, 100.f);
		Run->GetComms()->SetMessageSeed(5);

		UDataTable* Table = NewObject<UDataTable>();
		Table->RowStruct = FSSTraceMessageRow::StaticStruct();
		FSSTraceMessageRow Row;
		Row.Kind = ESSTraceMessageKind::Truth;
		Row.Order = 1;
		Row.Title = FText::FromString(TEXT("Replace"));
		Row.Text = FText::FromString(TEXT("Replace"));
		Row.CipherSource = Plain;
		Table->AddRow(TEXT("Truth_Replace"), Row);

		USSTraceConfig* Config = NewObject<USSTraceConfig>();
		Config->SensorPositions = { FVector2D(60.0, 50.0), FVector2D(550.0, 55.0), FVector2D(300.0, 360.0) };
		Config->ShelterPosition = FVector2D(360.0, 190.0);
		Config->MessageTable = Table;
		Config->TruthEvery = 1;

		USSTraceSession* Session = NewObject<USSTraceSession>();
		Session->Initialize(Config, {}, {}, Config->ReceiveGoal - 1);
		Session->SetSeed(1234);
		Session->SendDirect();
		Run->GetComms()->FinishTrace(*Session);
		return Run;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSTruthSessionTest, "SS.Decode.TruthSession",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSTruthSessionTest::RunTest(const FString& Parameters)
{
	using namespace SSTruthSessionTest;
	USSRunSubsystem* Run = MakeRunWithTruth();
	USSCommsState* Comms = Run->GetComms();

	if (!TestEqual(TEXT("Truth waiting"), Comms->GetPendingMessages().Num(), 1)) return false;
	const FSSPendingMessage& Pending = Comms->GetPendingMessages()[0];
	TestEqual(TEXT("Truth has a substitution key"), Pending.SubKey.Num(), 26);
	TestEqual(TEXT("Guess table starts empty"), Pending.Guess.Num(), 26);
	const TArray<int32> Answer = FSSCipher::InvertKey(Pending.SubKey);

	USSTruthSession* Session = NewObject<USSTruthSession>();
	if (!TestTrue(TEXT("Initialize"), Session->Initialize(Run, 0))) return false;
	Session->SetSeed(11);

	// 자동 해독 전엔 입력 불가, 문장은 암호문 그대로
	TestFalse(TEXT("No guessing before the solver"), Session->SetGuess(Session->GetCipherLetters()[0], 0));
	TestEqual(TEXT("Shows the cipher before solving"), Session->GetShownText(), Session->GetCipher());

	// 자동 해독: 점수는 내려가지 않음 (채택 수가 걸음 수를 넘지 않음)
	Session->StartSolver();
	TestTrue(TEXT("Solver running"), Session->IsSolverRunning());
	for (int32 Frame = 0; Frame < 60; ++Frame)
	{
		Session->StepSolver(6);
	}
	TestEqual(TEXT("Tries counted"), Session->GetSolverTries(), 360);
	TestTrue(TEXT("Accepted never exceeds tries"), Session->GetSolverAccepted() <= Session->GetSolverTries());

	// 멈춤: 정답률이 65% 이상 80% 미만, 틀린 글자는 없음 (모르는 칸은 빈칸)
	Session->FinishSolver();
	TestTrue(TEXT("Solver done"), Session->IsSolverDone());
	const float Stopped = Session->GetCorrectRatio();
	TestTrue(TEXT("Stops at or above StopCoverage"), Stopped >= USSTruthSession::StopCoverage - KINDA_SMALL_NUMBER);
	TestTrue(TEXT("Stops below UnlockCoverage"), Stopped < USSTruthSession::UnlockCoverage);
	bool bNoWrong = true;
	for (const int32 Letter : Session->GetCipherLetters())
	{
		const int32 Guess = Session->GetGuessFor(Letter);
		if (Guess != INDEX_NONE && Guess != Answer[Letter]) bNoWrong = false;
	}
	TestTrue(TEXT("No wrong letters after the solver"), bNoWrong);
	TestTrue(TEXT("Shown text has blanks"), Session->GetShownText().Contains(TEXT("_")));
	TestFalse(TEXT("Not unlocked yet"), Session->IsUnlocked());

	// 추측표는 대기함에 저장 → 다시 열면 빈칸 채우기부터
	TestTrue(TEXT("Solver result saved"), Comms->GetPendingMessages()[0].bSolverDone);
	USSTruthSession* Reopened = NewObject<USSTruthSession>();
	Reopened->Initialize(Run, 0);
	TestTrue(TEXT("Reopen skips the solver"), Reopened->IsSolverDone());
	TestTrue(TEXT("Reopen keeps the guesses"), FMath::IsNearlyEqual(Reopened->GetCorrectRatio(), Stopped));

	// 같은 원래 글자는 한 칸에만
	TArray<int32> Blanks;
	for (const int32 Letter : Session->GetCipherLetters())
	{
		if (Session->GetGuessFor(Letter) == INDEX_NONE) Blanks.Add(Letter);
	}
	if (Blanks.Num() >= 2)
	{
		Session->SetGuess(Blanks[0], 25);
		Session->SetGuess(Blanks[1], 25);
		TestEqual(TEXT("Same plain letter moves to the new cell"), Session->GetGuessFor(Blanks[0]), INDEX_NONE);
		Session->SetGuess(Blanks[1], INDEX_NONE);
	}

	// 빈칸을 정답으로 채우다 80%가 되면 잠금 해제 → 대기함에서 빠지고 진실 단서 +1
	for (const int32 Letter : Blanks)
	{
		if (Session->IsUnlocked()) break;
		Session->SetGuess(Letter, Answer[Letter]);
	}
	TestTrue(TEXT("Unlocked"), Session->IsUnlocked());
	TestEqual(TEXT("Shows the original"), Session->GetShownText(), FString(Plain));
	TestEqual(TEXT("Truth clue counted"), Comms->GetTruthCluesFound(), 1);
	TestEqual(TEXT("Message left the queue"), Comms->GetPendingMessages().Num(), 0);
	TestFalse(TEXT("No guessing after unlock"), Session->SetGuess(Blanks.IsEmpty() ? 0 : Blanks[0], 0));
	return true;
}
#endif
