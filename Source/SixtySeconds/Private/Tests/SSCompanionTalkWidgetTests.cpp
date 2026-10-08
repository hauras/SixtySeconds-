#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "UI/Companion/SSCompanionTalkWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Character/SSSurvivorDefinition.h"
#include "Companion/SSCompanionState.h"
#include "Item/SSRunSubsystem.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
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
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSCompanionTalkBindingTest, "SS.Companion.TalkWidgetBindings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSCompanionTalkBindingTest::RunTest(const FString& Parameters)
{
	UGameInstance* Instance = NewObject<UGameInstance>(GEngine);
	Instance->InitializeStandalone();
	UWorld* World = Instance->GetWorld();
	USSRunSubsystem* Run = Instance->GetSubsystem<USSRunSubsystem>();
	Run->ResetRun();
	Run->GetCompanions()->SetTables(
		LoadObject<UDataTable>(nullptr, TEXT("/Game/Data/Companion/DT_Clues.DT_Clues")),
		LoadObject<UDataTable>(nullptr, TEXT("/Game/Data/Companion/DT_CompanionLines.DT_CompanionLines")));
	USSSurvivorDefinition* Person = LoadObject<USSSurvivorDefinition>(nullptr,
		TEXT("/Game/Blueprints/Character/Data/DA_Survivor_Test.DA_Survivor_Test"));
	UClass* Class = LoadClass<USSCompanionTalkWidget>(nullptr, TEXT("/Game/UI/WBP_CompanionTalk.WBP_CompanionTalk_C"));
	UClass* HUDClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/UI/Scramble/WBP_ShalterHUD.WBP_ShalterHUD_C"));
	if (TestNotNull(TEXT("Companion definition loads"), Person) && TestNotNull(TEXT("Talk WBP loads"), Class))
	{
		FClassProperty* Assigned = HUDClass ? FindFProperty<FClassProperty>(HUDClass, TEXT("CompanionTalkWidgetClass")) : nullptr;
		TestTrue(TEXT("Shelter uses the talk WBP"), Assigned && Assigned->GetObjectPropertyValue_InContainer(HUDClass->GetDefaultObject()) == Class);
		Run->RecruitSurvivor(Person);
		Run->RescueFollowingSurvivors();
		USSCompanionState* Companions = Run->GetCompanions();
		Companions->SetInvestigationSeed(7);
		// Produce one genuine clue report before opening the window.
		for (int32 Night = 0; Night < 10; ++Night)
		{
			const FSSCompanionRecord* Record = Companions->FindRecord(Person->SurvivorId);
			if (Record && Record->PendingReports.ContainsByPredicate([](const FSSInvestigationReport& Item)
			{
				return Item.bFoundClue;
			})) break;
			Companions->RunNight();
		}
		USSCompanionTalkWidget* Talk = CreateWidget<USSCompanionTalkWidget>(Instance, Class);
		Talk->SetSurvivor(Person->SurvivorId);
		TSharedRef<SWidget> Slate = Talk->TakeWidget();
		UButton* Report = Cast<UButton>(Talk->GetWidgetFromName(TEXT("ReportButton")));
		UButton* Order = Cast<UButton>(Talk->GetWidgetFromName(TEXT("OrderButton")));
		UButton* Close = Cast<UButton>(Talk->GetWidgetFromName(TEXT("CloseButton")));
		UHorizontalBox* Spots = Cast<UHorizontalBox>(Talk->GetWidgetFromName(TEXT("SpotList")));
		UBorder* Card = Cast<UBorder>(Talk->GetWidgetFromName(TEXT("ClueCard")));
		UTextBlock* Name = Cast<UTextBlock>(Talk->GetWidgetFromName(TEXT("NameText")));
		if (TestNotNull(TEXT("Report button bound"), Report) && TestNotNull(TEXT("Order button bound"), Order) && TestNotNull(TEXT("Close button bound"), Close) && TestNotNull(TEXT("Spot list bound"), Spots) && TestNotNull(TEXT("Clue card bound"), Card) && TestNotNull(TEXT("Name bound"), Name))
		{
			TestEqual(TEXT("Selected companion name"), Name->GetText().ToString(), Person->DisplayName.ToString());
			TestEqual(TEXT("Five runtime spot controls"), Spots->GetChildrenCount(), 5);
			TestTrue(TEXT("Close handler bound"), Close->OnClicked.IsBound());
			TestEqual(TEXT("Clue hidden at first"), Card->GetVisibility(), ESlateVisibility::Collapsed);
			Report->OnClicked.Broadcast();
			TestEqual(TEXT("Reading displays a real clue card"), Card->GetVisibility(), ESlateVisibility::HitTestInvisible);
			TestTrue(TEXT("Reading stores clue"), Companions->GetHeardClues().Num() > 0);
			if (FParse::Param(FCommandLine::Get(), TEXT("SSRenderTalk")))
			{
				FAssetCompilingManager::Get().FinishAllCompilation();
				FWidgetRenderer Renderer(false);
				UTextureRenderTarget2D* Target = Renderer.DrawWidget(Slate, FVector2D(1920, 1080));
				FBufferArchive PNG;
				TestTrue(TEXT("Render WBP preview"), Target && FImageUtils::ExportRenderTarget2DAsPNG(Target, PNG) && FFileHelper::SaveArrayToFile(PNG, *(FPaths::ProjectSavedDir() / TEXT("Tests/SS_CompanionTalk.png"))));
			}
			Order->OnClicked.Broadcast();
			TestEqual(TEXT("Order opens spot choices"), Spots->GetVisibility(), ESlateVisibility::Visible);
			const int32 Before = Run->GetActionPoints();
			UUserWidget* Spot = Cast<UUserWidget>(Spots->GetChildAt(0));
			UButton* SpotButton = Spot ? Cast<UButton>(Spot->WidgetTree->RootWidget) : nullptr;
			if (TestNotNull(TEXT("Spot control creates its button"), SpotButton)) SpotButton->OnClicked.Broadcast();
			TestEqual(TEXT("Clicking spot spends exactly one action"), Run->GetActionPoints(), Before - 1);
			TestFalse(TEXT("Order disabled after booking"), Order->GetIsEnabled());
			TestEqual(TEXT("Booked spot list closes"), Spots->GetVisibility(), ESlateVisibility::Collapsed);
			Talk->RemoveFromParent();
		}
	}
	Instance->Shutdown();
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}
#endif
