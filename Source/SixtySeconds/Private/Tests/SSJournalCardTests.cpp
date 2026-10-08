#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "UI/Shelter/SSComputerWidget.h"
#include "Item/SSRunSubsystem.h"
#include "Item/SSItemDefinition.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Slate/WidgetRenderer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "Serialization/BufferArchive.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSSJournalCardTest, "SS.Journal.CardOpensSnapshot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSSJournalCardTest::RunTest(const FString& Parameters)
{
	UGameInstance* Game = NewObject<UGameInstance>(GEngine);
	Game->InitializeStandalone();
	USSRunSubsystem* Run = Game->GetSubsystem<USSRunSubsystem>();
	USSItemDefinition* Item = NewObject<USSItemDefinition>(Game);
	Item->ItemId = TEXT("Food");
	Item->DisplayName = FText::FromString(TEXT("Archive test food"));
	FSSItemStack Stack;
	Stack.Item = Item;
	Stack.Quantity = 1;
	Run->DepositItems({Stack});
	const FText Snapshot = Run->GetJournalEntries()[0].Message;
	USSComputerWidget* Computer = CreateWidget<USSComputerWidget>(Game);
	const TSharedRef<SWidget> Slate = Computer->TakeWidget();
	const auto RenderPreview = [&](const TCHAR* Name)
	{
		if (!FParse::Param(FCommandLine::Get(), TEXT("SSRenderJournal"))) return;
		FWidgetRenderer Renderer(false);
		UTextureRenderTarget2D* Target = Renderer.DrawWidget(Slate, FVector2D(1920, 1080));
		FBufferArchive PNG;
		const FString Directory = FPaths::ProjectSavedDir() / TEXT("Tests");
		IFileManager::Get().MakeDirectory(*Directory, true);
		TestTrue(TEXT("Journal preview renders"), Target && FImageUtils::ExportRenderTarget2DAsPNG(Target, PNG) && FFileHelper::SaveArrayToFile(PNG, *(Directory / Name)));
	};
	RenderPreview(TEXT("SS_JournalArchive.png"));
	TArray<UWidget*> Widgets;
	Computer->WidgetTree->GetAllWidgets(Widgets);
	USSJournalCardWidget* Card = nullptr;
	for (UWidget* Widget : Widgets)
		if (auto* Candidate = Cast<USSJournalCardWidget>(Widget))
			if (!Candidate->IsNavigation())
			{
				Card = Candidate;
				break;
			}
	TestNotNull(TEXT("Journal creates clickable cards"), Card);
	if (Card)
	{
		Card->ProcessEvent(Card->FindFunctionChecked(TEXT("SelectCard")), nullptr);
		bool bShowsSnapshot = false;
		for (UWidget* Widget : Widgets)
			if (auto* Text = Cast<UTextBlock>(Widget))
				bShowsSnapshot |= Text->GetText().EqualTo(Snapshot);
		TestTrue(TEXT("Click shows the complete historical message"), bShowsSnapshot);
		RenderPreview(TEXT("SS_JournalDetail.png"));
		Computer->ProcessEvent(Computer->FindFunctionChecked(TEXT("CloseRecord")), nullptr);
	}
	// There is one supply record and no night event: actual menu buttons must filter it out and back in.
	const auto SelectMenu = [&](int32 Index)
	{
		struct FCategoryParameter
		{
			int32 CategoryIndex;
		} Parameter{Index};
		Computer->ProcessEvent(Computer->FindFunctionChecked(TEXT("SelectCategory")), &Parameter);
		Widgets.Reset();
		Computer->WidgetTree->GetAllWidgets(Widgets);
		int32 Cards = 0;
		for (UWidget* Widget : Widgets)
			if (auto* Candidate = Cast<USSJournalCardWidget>(Widget))
				if (!Candidate->IsNavigation()) ++Cards;
		return Cards;
	};
	TestEqual(TEXT("Night filter excludes supply entries"), SelectMenu(1), 0);
	TestEqual(TEXT("Supply filter restores stored entry"), SelectMenu(5), 1);
	Run->AddDetailedEventJournal(FText::FromString(TEXT("Night event snapshot")),
		FText::FromString(TEXT("멎춘 발소리")), FText::FromString(TEXT("문 밖의 발소리가 멎었다. 붉은 스캔 빛이 문틈을 지나간다.")),
		FText::FromString(TEXT("불을 끄고 숨을 죽인다")), FText::FromString(TEXT("한참을 숨죽였다. 빛은 지나갔다.")),
		FText::FromString(TEXT("행동력 −1")));
	const auto& Recorded = Run->GetJournalEntries().Last();
	TestEqual(TEXT("Event choice preserved as a snapshot"), Recorded.Choice.ToString(), FString(TEXT("불을 끄고 숨을 죽인다")));
	TestEqual(TEXT("Event filter includes structured night record"), SelectMenu(1), 1);
	SelectMenu(0);
	Run->AddJournal(ESSJournalEvent::Investigation, FText::FromString(TEXT("강태오의 보고 — 순찰 로봇 두 대가 번갈아 지나간다.")));
	Run->AddJournal(ESSJournalEvent::Signal, FText::FromString(TEXT("암호화된 외부 기록을 수신했다. 무전기에서 해독할 수 있다.")));
	Run->AddJournal(ESSJournalEvent::Expedition, FText::FromString(TEXT("B1 보급 창고 탐사 — 식량 3개를 가져왔다.")));
	Run->AddJournal(ESSJournalEvent::Rations, FText::FromString(TEXT("오늘 배급 — 식량 배급함 / 물 배급함")));
	RenderPreview(TEXT("SS_JournalArchive.png"));
	struct FEntryParameter
	{
		int32 EntryIndex;
	} EntryParameter{1};
	Computer->ProcessEvent(Computer->FindFunctionChecked(TEXT("OpenRecord")), &EntryParameter);
	RenderPreview(TEXT("SS_JournalDetail.png"));
	Computer->CloseWindows();
	Game->Shutdown();
	return true;
}
#endif
