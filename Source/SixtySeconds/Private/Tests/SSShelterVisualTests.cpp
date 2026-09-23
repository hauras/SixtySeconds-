#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "UI/SSShelterHUD.h"
#include "UI/SSStockItemWidget.h"
#include "UI/SSSurvivorImageWidget.h"
#include "UI/SSInfoPanelWidget.h"
#include "UI/SSSurvivorInfoContentWidget.h"
#include "Components/NamedSlot.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Character/SSSurvivorDefinition.h"
#include "Item/SSItemDefinition.h"
#include "Item/SSRunSubsystem.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Slate/WidgetRenderer.h"
#include "ImageUtils.h"
#include "Serialization/BufferArchive.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "AssetCompilingManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSShelterVisualTest, "SS.Shelter.VisualBindings",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSShelterVisualTest::RunTest(const FString& Parameters)
{
    UGameInstance* Instance = NewObject<UGameInstance>(GEngine);
    Instance->InitializeStandalone();
    UWorld* World = Instance->GetWorld();
    USSRunSubsystem* Run = Instance->GetSubsystem<USSRunSubsystem>();
    UClass* HUDClass = LoadClass<USSShelterHUD>(nullptr, TEXT("/Game/UI/Scramble/WBP_ShalterHUD.WBP_ShalterHUD_C"));
    USSShelterHUD* HUD = HUDClass ? CreateWidget<USSShelterHUD>(Instance, HUDClass) : nullptr;
    if (!TestNotNull(TEXT("Shelter HUD loads"), HUD))
    {
        Instance->Shutdown(); GEngine->DestroyWorldContext(World); World->DestroyWorld(false);
        return false;
    }
    {
        TSharedRef<SWidget> Slate = HUD->TakeWidget();
        HUD->InitHUD(1); HUD->RefreshStats(100,100,100);
        auto ImageIn = [HUD](const TCHAR* Slot, const TCHAR* Name) -> UImage*
        {
            UUserWidget* Child = Cast<UUserWidget>(HUD->GetWidgetFromName(Slot));
            return Child ? Cast<UImage>(Child->GetWidgetFromName(Name)) : nullptr;
        };
        const TCHAR* PersonSlots[] = {TEXT("Survivor_Researcher"),TEXT("Survivor_Technician"),TEXT("Survivor_Operator")};
        for (const TCHAR* Slot : PersonSlots)
        {
            USSSurvivorImageWidget* Child = Cast<USSSurvivorImageWidget>(HUD->GetWidgetFromName(Slot));
            UImage* Im = ImageIn(Slot,TEXT("SurvivorImage"));
            if (!TestNotNull(TEXT("Native survivor image built"),Im) || !TestNotNull(TEXT("Survivor slot"),Child)) continue;
            TestEqual(TEXT("Unrescued is hidden"),Im->GetVisibility(),ESlateVisibility::Hidden);
            TestTrue(TEXT("Recruit defined companion"),Run->RecruitSurvivor(Child->SurvivorDefinition));
            TestEqual(TEXT("Following remains hidden"),Im->GetVisibility(),ESlateVisibility::Hidden);
            Run->RescueFollowingSurvivors();
            TestEqual(TEXT("Rescue refreshes visible image"),Im->GetVisibility(),ESlateVisibility::HitTestInvisible);
            TestTrue(TEXT("Uses DA image"),Im->GetBrush().GetResourceObject()==Child->SurvivorDefinition->ShelterImage);
            UButton* Select = Cast<UButton>(Child->GetWidgetFromName(TEXT("SurvivorButton")));
            if (TestNotNull(TEXT("Companion has selection button"), Select))
            {
                TestEqual(TEXT("Companion permits child hit testing"),Child->GetVisibility(),ESlateVisibility::SelfHitTestInvisible);
                TestTrue(TEXT("Selection button is connected"), Select->OnClicked.IsBound());
                Select->OnClicked.Broadcast();
                USSInfoPanelWidget* Panel=HUD->GetOpenInfoPanel();
                if (TestNotNull(TEXT("Click opens shared info panel"),Panel))
                {
                    TSharedRef<SWidget> PanelSlate=Panel->TakeWidget();
                    UNamedSlot* ContentSlot=Cast<UNamedSlot>(Panel->GetWidgetFromName(TEXT("ContentSlot")));
                    USSSurvivorInfoContentWidget* Content=ContentSlot ? Cast<USSSurvivorInfoContentWidget>(ContentSlot->GetContent()) : nullptr;
                    if(TestNotNull(TEXT("Companion content inserted"),Content))
                    {
                        TSharedRef<SWidget> ContentSlate=Content->TakeWidget();
                        UProgressBar* Bar=Cast<UProgressBar>(Content->GetWidgetFromName(TEXT("HealthBar")));
                        if(TestNotNull(TEXT("Health bar is bound"),Bar))
                            TestEqual(TEXT("Shows selected companion health"),Bar->GetPercent(),Child->SurvivorDefinition->InitialStats.Health/100.f);
                        UNamedSlot* Observations=Cast<UNamedSlot>(Panel->GetWidgetFromName(TEXT("ObservationSlot")));
                        TestTrue(TEXT("Observation moved to full-width slot"),Observations && Observations->GetContent()==Content->GetObservationWidget());
                    }
                    if(FParse::Param(FCommandLine::Get(),TEXT("SSRenderShelter")))
                    {
                        FAssetCompilingManager::Get().FinishAllCompilation();
                        FWidgetRenderer Renderer(false);
                        UTextureRenderTarget2D* Target=Renderer.DrawWidget(PanelSlate,FVector2D(1920,1080));
                        FBufferArchive PNG;
                        if(Target && FImageUtils::ExportRenderTarget2DAsPNG(Target,PNG))
                            FFileHelper::SaveArrayToFile(PNG,*(FPaths::ProjectSavedDir()/FString::Printf(TEXT("Tests/Info_%s.png"),Slot)));
                    }
                    UButton* Close=Cast<UButton>(Panel->GetWidgetFromName(TEXT("CloseButton")));
                    if(TestNotNull(TEXT("Close button exists"),Close))
                    {
                        TestTrue(TEXT("Close button connected"),Close->OnClicked.IsBound());
                        Close->OnClicked.Broadcast();
                        TestFalse(TEXT("Close removes panel"),Panel->IsInViewport());
                    }
                }
            }
        }
        for (const TCHAR* Id : {TEXT("Food"),TEXT("Water"),TEXT("Medkit"),TEXT("Battery"),TEXT("RepairKit")})
        {
            USSItemDefinition* Item=LoadObject<USSItemDefinition>(nullptr,*FString::Printf(TEXT("/Game/Blueprints/Item/Data/DA_%s.DA_%s"),Id,Id));
            UImage* Im=ImageIn(*FString::Printf(TEXT("%sStockWidget"),Id),TEXT("StockImage"));
            if (!TestNotNull(TEXT("Stock image bound"),Im) || !TestNotNull(TEXT("Item DA"),Item)) continue;
            TestEqual(TEXT("Zero stock hidden"),Im->GetVisibility(),ESlateVisibility::Hidden);
            for(int32 Quantity=1;Quantity<=5;++Quantity)
            {
                FSSItemStack Stack; Stack.Item=Item;Stack.Quantity=1;Run->DepositItems({Stack});
                TestEqual(TEXT("Deposit refreshes stock"),Im->GetVisibility(),ESlateVisibility::HitTestInvisible);
                TestTrue(TEXT("Correct threshold image"),Im->GetBrush().GetResourceObject()==Item->GetStockTexture(Quantity));
            }
        }
        if (FParse::Param(FCommandLine::Get(),TEXT("SSRenderShelter")))
        {
            FAssetCompilingManager::Get().FinishAllCompilation();
            FWidgetRenderer Renderer(false);
            UTextureRenderTarget2D* Target=Renderer.DrawWidget(Slate,FVector2D(1920,1080));
            FBufferArchive PNG;
            if (Target && FImageUtils::ExportRenderTarget2DAsPNG(Target,PNG))
                TestTrue(TEXT("Save actual UMG render"),FFileHelper::SaveArrayToFile(PNG,*(FPaths::ProjectSavedDir()/TEXT("Tests/SS_Shelter_Full.png"))));
            else AddError(TEXT("Could not render shelter HUD"));
        }
        Run->ResetRun();
        for(const TCHAR* Slot:PersonSlots)
            if(UImage* Im=ImageIn(Slot,TEXT("SurvivorImage")))TestEqual(TEXT("Reset hides rendered companion"),Im->GetVisibility(),ESlateVisibility::Hidden);
        if(UImage* Im=ImageIn(TEXT("FoodStockWidget"),TEXT("StockImage")))
            TestEqual(TEXT("Reset hides rendered stock"),Im->GetVisibility(),ESlateVisibility::Hidden);
        HUD->RemoveFromParent();
    }
    Instance->Shutdown();
    GEngine->DestroyWorldContext(World);World->DestroyWorld(false);
    return true;
}
#endif
