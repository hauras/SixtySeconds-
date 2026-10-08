// 확인·시연용 콘솔 명령 (게임 중 ~ 키). 출시 빌드에서는 빠짐
#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING
#include "HAL/IConsoleManager.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "Ara/SSAraDirector.h"
#include "Item/SSRunSubsystem.h"
#include "Companion/SSCompanionState.h"

namespace SSDebugCommands
{
	// 출력 로그와 화면 왼쪽 위에 같이 보여줌 (화면 글자는 10초 동안)
	void Print(const FString& Message)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s"), *Message);
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Cyan, Message);
	}

	// 지금 게임의 RunSubsystem (없으면 nullptr)
	USSRunSubsystem* FindRun(UWorld* World)
	{
		UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		return GameInstance ? GameInstance->GetSubsystem<USSRunSubsystem>() : nullptr;
	}

	// SS.Ara.Target <동료ID> : 바로 표적으로 만들고 다음 밤에 검진 제안
	FAutoConsoleCommandWithWorldAndArgs AraTarget(
		TEXT("SS.Ara.Target"),
		TEXT("SS.Ara.Target <SurvivorId> - make the survivor ARA's target and schedule the checkup offer for the next night"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			USSRunSubsystem* Run = FindRun(World);
			if (!Run || Args.Num() < 1)
			{
				Print(TEXT("[SS.Ara.Target] usage: SS.Ara.Target TestResearcher (in the shelter)"));
				return;
			}

			const bool bOk = Run->GetAra()->DebugForceTarget(FName(*Args[0]));
			Print(FString::Printf(
				TEXT("[SS.Ara.Target] %s -> %s"),
				*Args[0],
				bOk ? TEXT("target set, offer next night") : TEXT("failed (unknown/dead/android, or a swap already happened)")));
		}));

	// SS.Ara.Seize : 다음 밤에 새벽 02:10 강제 교체
	FAutoConsoleCommandWithWorld AraSeize(
		TEXT("SS.Ara.Seize"),
		TEXT("SS.Ara.Seize - schedule the 02:10 forced swap for the next night (needs a target)"),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			USSRunSubsystem* Run = FindRun(World);
			const bool bOk = Run && Run->GetAra()->DebugScheduleSeize();
			Print(bOk
				? TEXT("[SS.Ara.Seize] scheduled for the next night")
				: TEXT("[SS.Ara.Seize] failed (no target, or a swap already happened)"));
		}));

	// SS.Ara.Status : 아라 판단 상태 출력
	FAutoConsoleCommandWithWorld AraStatus(
		TEXT("SS.Ara.Status"),
		TEXT("SS.Ara.Status - print ARA suspicion, target, refusals and who is an android"),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			USSRunSubsystem* Run = FindRun(World);
			if (!Run)
			{
				Print(TEXT("[SS.Ara.Status] no run"));
				return;
			}
			Print(TEXT("[SS.Ara.Status] ") + Run->GetAra()->DebugDescribe());
		}));

	// SS.Day <날짜> : 날짜를 바로 옮김 (예: SS.Day 11 → 다음 밤이 마지막 밤)
	FAutoConsoleCommandWithWorldAndArgs SetDay(
		TEXT("SS.Day"),
		TEXT("SS.Day <Day> - jump to that day (SS.Day 11 then press next day to reach the final server room night)"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			USSRunSubsystem* Run = FindRun(World);
			if (!Run || Args.Num() < 1)
			{
				Print(TEXT("[SS.Day] usage: SS.Day 11 (in the shelter)"));
				return;
			}
			Run->DebugSetDay(FCString::Atoi(*Args[0]));
			Print(FString::Printf(TEXT("[SS.Day] day = %d (final night comes when the day reaches %d)"), Run->GetCurrentDay(), USSRunSubsystem::FinalDay));
		}));

	// SS.Rescue.Setup [동료ID] : 동료를 B2에 붙잡고 덕트 단서를 들은 것으로 (패널 버튼 확인용)
	FAutoConsoleCommandWithWorldAndArgs RescueSetup(
		TEXT("SS.Rescue.Setup"),
		TEXT("SS.Rescue.Setup [SurvivorId] - capture the survivor (default TestResearcher), learn the B2 duct clue and refill action points"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			USSRunSubsystem* Run = FindRun(World);
			if (!Run)
			{
				Print(TEXT("[SS.Rescue.Setup] no run (use in the shelter)"));
				return;
			}

			const FName Target = Args.Num() > 0 ? FName(*Args[0]) : FName(TEXT("TestResearcher"));
			// 단서를 먼저 (붙잡을 때 화면이 갱신되며 패널 버튼이 나타나도록)
			Run->GetCompanions()->DebugHearClue(SSRescueIds::RouteClue());
			const bool bCaptured = Run->IsSurvivorCaptured(Target) || Run->MoveSurvivorToCaptured(Target);
			Run->AdjustActionPoints(USSRunSubsystem::MaxActionPoints);
			Print(FString::Printf(
				TEXT("[SS.Rescue.Setup] %s %s, B2 route known, AP refilled"),
				*Target.ToString(),
				bCaptured ? TEXT("captured") : TEXT("NOT captured (unknown id?)")));
		}));
}
#endif
