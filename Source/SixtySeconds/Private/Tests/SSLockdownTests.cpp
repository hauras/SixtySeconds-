#include "Misc/AutomationTest.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "TimerManager.h"

#include "GameMode/SSGameMode.h"
#include "Item/SSDepositZone.h"
#include "Phase/SSLabLockdownDirector.h"
#include "Phase/SSGuardAndroid.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSSLockdownFlowTest,
	"SS.Lockdown.Sequence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSLockdownFlowTest::RunTest(const FString& Parameters)
{
	UGameInstance* Instance = NewObject<UGameInstance>(GEngine);
	Instance->InitializeStandalone();
	UWorld* World = Instance->GetWorld();
	FURL URL;
	URL.AddOption(TEXT("game=/Script/SixtySeconds.SSGameMode"));
	World->SetGameMode(URL);
	ASSGameMode* Mode = World->GetAuthGameMode<ASSGameMode>();
	if (!TestNotNull(TEXT("GameMode created"), Mode))
	{
		Instance->Shutdown();
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}

	World->SpawnActor<ASSDepositZone>();
	AActor* ShelterDoor = World->SpawnActor<AActor>();
	// 이동 가능한 바닥 컴포넌트를 붙여 실제 문 위치를 확인한다.
	USceneComponent* Root = NewObject<USceneComponent>(ShelterDoor);
	ShelterDoor->SetRootComponent(Root);
	Root->RegisterComponent();
	ShelterDoor->Tags.Add(TEXT("SSLockdownShelterDoor"));
	ShelterDoor->SetActorLocation(FVector(-305.f, -1900.f, 0.f));

	AActor* ResearchDoor = World->SpawnActor<AActor>();
	USceneComponent* ResearchRoot = NewObject<USceneComponent>(ResearchDoor);
	ResearchDoor->SetRootComponent(ResearchRoot);
	ResearchRoot->RegisterComponent();
	ResearchDoor->Tags.Add(TEXT("SSLockdownResearchDoor"));
	ResearchDoor->SetActorLocation(FVector(98.f, 1840.f, 0.f));
	ASSLabLockdownDirector* Director = World->SpawnActor<ASSLabLockdownDirector>();
	ASSGuardAndroid* Guard = World->SpawnActor<ASSGuardAndroid>();
	Guard->Tags.Add(TEXT("SSLockdownGuard"));
	Guard->SetActorLocation(FVector(-70.f, 1970.f, 0.f));

	World->InitializeActorsForPlay(URL);
	World->BeginPlay();
	TestEqual(TEXT("Starts in scramble"), Mode->GetCurrentPhase(), ESSGamePhase::Scramble);
	// 새 타이머는 첫 프레임에 등록되고 다음 프레임부터 시간이 흐른다.
	World->GetTimerManager().Tick(0.f);
	++GFrameCounter;
	World->GetTimerManager().Tick(60.1f);
	TestEqual(TEXT("Expiry starts cinematic before failure"), Mode->GetCurrentPhase(), ESSGamePhase::Lockdown);
	TestEqual(TEXT("No remaining pickup time"), Mode->GetScrambleTimeRemaining(), 0.f);

	Director->Tick(1.8f);
	TestTrue(TEXT("Shelter door closed"), FMath::IsNearlyEqual(ShelterDoor->GetActorLocation().X, -98.f));
	TestTrue(TEXT("Research door still closed"), FMath::IsNearlyEqual(ResearchDoor->GetActorLocation().X, 98.f));
	TestTrue(TEXT("Android waits behind closed door"), Guard->IsHidden());
	Director->Tick(1.6f);
	TestTrue(TEXT("Research door opens after shelter"), FMath::IsNearlyEqual(ResearchDoor->GetActorLocation().X, 305.f));
	TestEqual(TEXT("Outcome waits for robot entry"), Mode->GetCurrentPhase(), ESSGamePhase::Lockdown);
	TestFalse(TEXT("Android becomes visible after opening"), Guard->IsHidden());
	TestTrue(TEXT("Android advances toward corridor"), Guard->GetActorLocation().Y < 1970.f);
	TestTrue(TEXT("Knee bends during walking"), FMath::Abs(Guard->LeftShin->GetRelativeRotation().Roll) > 1.f);
	TestTrue(TEXT("Arms swing in opposite directions"), Guard->LeftArm->GetRelativeRotation().Roll * Guard->RightArm->GetRelativeRotation().Roll < 0.f);
	Director->Tick(2.4f);
	TestEqual(TEXT("Outside survivor fails after cinematic"), Mode->GetCurrentPhase(), ESSGamePhase::Dead);
	Mode->CompleteScrambleTransition();
	TestEqual(TEXT("Completion cannot restart settled phase"), Mode->GetCurrentPhase(), ESSGamePhase::Dead);

	World->EndPlay(EEndPlayReason::Quit);
	Instance->Shutdown();
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}
#endif
