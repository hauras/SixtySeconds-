#include "Character/SSSurvivorDefinition.h"
#include "Item/SSRunSubsystem.h"
#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSSurvivorRescueTest, "SS.Survivors.Rescue",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSSSurvivorRescueTest::RunTest(const FString& Parameters)
{
    UGameInstance* Instance = NewObject<UGameInstance>();
    USSRunSubsystem* Run = NewObject<USSRunSubsystem>(Instance);
    USSSurvivorDefinition* Person = NewObject<USSSurvivorDefinition>(Run);
    Person->SurvivorId = TEXT("TestSurvivor");
    Person->InitialStats.Health = 70.f;
    USSSurvivorDefinition* Duplicate = NewObject<USSSurvivorDefinition>(Run);
    Duplicate->SurvivorId = Person->SurvivorId;
    TestFalse(TEXT("Reject null"), Run->RecruitSurvivor(nullptr));
    TestTrue(TEXT("Recruit"), Run->RecruitSurvivor(Person));
    TestFalse(TEXT("Duplicate ID rejected"), Run->RecruitSurvivor(Duplicate));
    TestEqual(TEXT("Not rescued before entry"), Run->GetRescuedSurvivors().Num(), 0);
    TestFalse(TEXT("Following is not visible in shelter"), Run->IsSurvivorRescued(Person->SurvivorId));
    TestFalse(TEXT("Empty identity is never visible"), Run->IsSurvivorRescued(NAME_None));
    TestEqual(TEXT("Empty inventory does not prevent rescue"), Run->RescueFollowingSurvivors(), 1);
    TestEqual(TEXT("Pending emptied"), Run->GetFollowingSurvivorCount(), 0);
    TestTrue(TEXT("Rescued identity is visible even through another DA"), Run->IsSurvivorRescued(Duplicate->SurvivorId));
    TestFalse(TEXT("Other identities remain hidden"), Run->IsSurvivorRescued(TEXT("OtherPerson")));
    TestEqual(TEXT("Starting stats copied"), Run->GetRescuedSurvivors()[0].Stats.Health, 70.f);
    Person->InitialStats.Health = 100.f;
    TestEqual(TEXT("Runtime stats independent"), Run->GetRescuedSurvivors()[0].Stats.Health, 70.f);
    TestEqual(TEXT("Reentry no duplication"), Run->RescueFollowingSurvivors(), 0);
    TestFalse(TEXT("Rescued ID rejected"), Run->RecruitSurvivor(Duplicate));
    Run->ResetRun();
    TestEqual(TEXT("Reset rescued"), Run->GetRescuedSurvivors().Num(), 0);
    TestFalse(TEXT("Reset hides survivor again"), Run->IsSurvivorRescued(Person->SurvivorId));
    TestTrue(TEXT("New run can recruit"), Run->RecruitSurvivor(Person));
    Run->ClearFollowingSurvivors();
    TestEqual(TEXT("Death clears pending"), Run->RescueFollowingSurvivors(), 0);
    Run->RecruitSurvivor(Person);
    Run->ResetRun();
    TestEqual(TEXT("Reset pending"), Run->GetFollowingSurvivorCount(), 0);
    return true;
}
#endif
