#if WITH_EDITOR
#include "UI/Companion/SSCompanionTalkWidget.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "HAL/IConsoleManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"
#include "WidgetBlueprint.h"

namespace SSCompanionTalkEditor
{
	FLinearColor Color(const TCHAR* Hex)
	{
		return FLinearColor::FromSRGBColor(FColor::FromHex(Hex));
	}

	bool Save(UBlueprint* Blueprint)
	{
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		const FString Filename = FPackageName::LongPackageNameToFilename(
			Blueprint->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
		return UPackage::SavePackage(Blueprint->GetOutermost(), Blueprint, *Filename, Args);
	}

	// Run once through UnrealEditor-Cmd: SS.Editor.CreateCompanionTalk.
	// Creates real designer widgets using Unreal's factory/compiler; never replaces an existing layout.
	void Create()
	{
		const TCHAR* Path = TEXT("/Game/UI/WBP_CompanionTalk");
		UWidgetBlueprint* BP = LoadObject<UWidgetBlueprint>(nullptr, TEXT("/Game/UI/WBP_CompanionTalk.WBP_CompanionTalk"), nullptr, LOAD_NoWarn);
		if (!BP)
		{
			UPackage* Package = CreatePackage(Path);
			BP = CastChecked<UWidgetBlueprint>(FKismetEditorUtilities::CreateBlueprint(
				USSCompanionTalkWidget::StaticClass(), Package, TEXT("WBP_CompanionTalk"), BPTYPE_Normal,
				UWidgetBlueprint::StaticClass(), UWidgetBlueprintGeneratedClass::StaticClass()));
			UWidgetTree* Tree = BP->WidgetTree;
			auto Label = [Tree](const TCHAR* Name, const TCHAR* Text, int32 Size, const TCHAR* Shade)
			{
				UTextBlock* Item = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
				Item->SetText(FText::FromString(Text));
				FSlateFontInfo Font = Item->GetFont();
				Font.Size = Size;
				Item->SetFont(Font);
				Item->SetAutoWrapText(true);
				Item->SetColorAndOpacity(FSlateColor(Color(Shade)));
				Item->SetVisibility(ESlateVisibility::HitTestInvisible);
				return Item;
			};
			auto Border = [Tree](const TCHAR* Name, const TCHAR* Shade, FMargin Padding)
			{
				UBorder* Item = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), Name);
				Item->SetBrushColor(Color(Shade));
				Item->SetPadding(Padding);
				return Item;
			};
			auto Button = [Tree, &Label](const TCHAR* Name, const TCHAR* Caption, const TCHAR* Shade)
			{
				UButton* Item = Tree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
				FButtonStyle Style = Item->GetStyle();
				Style.Normal.TintColor = FSlateColor(Color(Shade));
				Style.Hovered.TintColor = FSlateColor(Color(TEXT("67503B")));
				Style.Pressed.TintColor = FSlateColor(Color(TEXT("241C16")));
				Item->SetStyle(Style);
				Item->SetCursor(EMouseCursor::Hand);
				const FString CaptionName = FString(Name) + TEXT("Caption");
				Item->SetContent(Label(*CaptionName, Caption, 18, TEXT("F2E4D0")));
				CastChecked<UButtonSlot>(Item->GetContent()->Slot)->SetPadding(FMargin(16, 10));
				return Item;
			};
			UCanvasPanel* Root = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
			Tree->RootWidget = Root;
			UBorder* Blocker = Border(TEXT("Backdrop"), TEXT("000000"), FMargin(0));
			Blocker->SetBrushColor(FLinearColor(0, 0, 0, .45f));
			UCanvasPanelSlot* BackSlot = Root->AddChildToCanvas(Blocker);
			BackSlot->SetAnchors(FAnchors(0, 0, 1, 1));
			BackSlot->SetOffsets(FMargin(0));
			UBorder* Frame = Border(TEXT("TalkFrame"), TEXT("B07A45"), FMargin(2));
			UCanvasPanelSlot* FrameSlot = Root->AddChildToCanvas(Frame);
			FrameSlot->SetAnchors(FAnchors(.15f, .43f, .85f, .9f));
			FrameSlot->SetOffsets(FMargin(0));
			UBorder* Body = Border(TEXT("TalkBody"), TEXT("15110E"), FMargin(26, 20));
			Frame->SetContent(Body);
			UVerticalBox* Column = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("TalkColumn"));
			Body->SetContent(Column);
			Column->AddChildToVerticalBox(Label(TEXT("NameText"), TEXT("동료 이름"), 26, TEXT("E8B07A")))->SetPadding(FMargin(0, 0, 0, 10));
			UScrollBox* Scroll = Tree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("DialogueScroll"));
			UVerticalBoxSlot* ScrollSlot = Column->AddChildToVerticalBox(Scroll);
			ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			ScrollSlot->SetPadding(FMargin(0, 0, 0, 12));
			UVerticalBox* Dialogue = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("DialogueColumn"));
			Scroll->AddChild(Dialogue);
			Dialogue->AddChildToVerticalBox(Label(TEXT("LineText"), TEXT("동료의 대화와 조사 보고가 여기에 표시됩니다."), 20, TEXT("F2E4D0")))->SetPadding(FMargin(0, 0, 0, 12));
			UBorder* Card = Border(TEXT("ClueCard"), TEXT("241C16"), FMargin(16, 12));
			Dialogue->AddChildToVerticalBox(Card);
			UVerticalBox* CardColumn = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ClueColumn"));
			Card->SetContent(CardColumn);
			CardColumn->AddChildToVerticalBox(Label(TEXT("ClueHeaderText"), TEXT("어젯밤 · 단서 · 단말 로그 1/3"), 14, TEXT("C9925A")))->SetPadding(FMargin(0, 0, 0, 6));
			CardColumn->AddChildToVerticalBox(Label(TEXT("ClueBodyText"), TEXT("단서 제목\n조사로 발견한 내용이 표시됩니다. 공유한 단서는 컴퓨터 기록에 남습니다."), 18, TEXT("E6D8C4")));
			UHorizontalBox* Choices = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Choices"));
			Column->AddChildToVerticalBox(Choices)->SetPadding(FMargin(0, 0, 0, 10));
			UHorizontalBoxSlot* ReportSlot = Choices->AddChildToHorizontalBox(Button(TEXT("ReportButton"), TEXT("조사 보고 듣기"), TEXT("345143")));
			ReportSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			ReportSlot->SetPadding(FMargin(0, 0, 10, 0));
			Choices->AddChildToHorizontalBox(Button(TEXT("OrderButton"), TEXT("조사 장소 정하기 · 행동력 1"), TEXT("4B3828")))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			UHorizontalBox* Spots = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("SpotList"));
			Spots->SetVisibility(ESlateVisibility::Collapsed);
			Column->AddChildToVerticalBox(Spots)->SetPadding(FMargin(0, 0, 0, 10));
			UHorizontalBox* Bottom = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("BottomRow"));
			Column->AddChildToVerticalBox(Bottom);
			UHorizontalBoxSlot* HintSlot = Bottom->AddChildToHorizontalBox(Label(TEXT("HintText"), TEXT("장소를 정하지 않으면 동료가 알아서 고릅니다."), 14, TEXT("A89C8C")));
			HintSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			HintSlot->SetVerticalAlignment(VAlign_Center);
			Bottom->AddChildToHorizontalBox(Button(TEXT("CloseButton"), TEXT("대화 끝내기"), TEXT("2A2420")));
			TArray<UWidget*> Widgets;
			Tree->GetAllWidgets(Widgets);
			for (UWidget* Widget : Widgets)
			{
				Widget->bIsVariable = true;
				BP->WidgetVariableNameToGuidMap.Add(Widget->GetFName(), FGuid::NewGuid());
			}
			FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
			FAssetRegistryModule::AssetCreated(BP);
		}
		FKismetEditorUtilities::CompileBlueprint(BP);
		if (BP->Status == BS_Error || !BP->GeneratedClass || !Save(BP))
		{
			UE_LOG(LogTemp, Error, TEXT("SS_TALK_ASSET_FAILED"));
			return;
		}
		UBlueprint* HUD = LoadObject<UBlueprint>(nullptr, TEXT("/Game/UI/Scramble/WBP_ShalterHUD.WBP_ShalterHUD"));
		FClassProperty* Property = HUD && HUD->GeneratedClass
			? FindFProperty<FClassProperty>(HUD->GeneratedClass, TEXT("CompanionTalkWidgetClass"))
			: nullptr;
		if (!Property)
		{
			UE_LOG(LogTemp, Error, TEXT("SS_TALK_HUD_PROPERTY_MISSING"));
			return;
		}
		Property->SetObjectPropertyValue_InContainer(HUD->GeneratedClass->GetDefaultObject(), BP->GeneratedClass);
		HUD->MarkPackageDirty();
		if (!Save(HUD))
		{
			UE_LOG(LogTemp, Error, TEXT("SS_TALK_HUD_SAVE_FAILED"));
			return;
		}
		UE_LOG(LogTemp, Display, TEXT("SS_TALK_ASSET_READY %s"), Path);
	}

	FAutoConsoleCommand CreateCommand(TEXT("SS.Editor.CreateCompanionTalk"),
		TEXT("Create and connect the editable companion talk Widget Blueprint."), FConsoleCommandDelegate::CreateStatic(&Create));
}
#endif
