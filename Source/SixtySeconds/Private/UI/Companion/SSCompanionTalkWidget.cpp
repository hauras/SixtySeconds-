#include "UI/Companion/SSCompanionTalkWidget.h"
#include "Companion/SSCompanionState.h"
#include "Character/SSSurvivorDefinition.h"
#include "Item/SSRunSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"

namespace SSTalkStyle
{
	FLinearColor Color(const TCHAR* Hex)
	{
		return FLinearColor::FromSRGBColor(FColor::FromHex(Hex));
	}

	UTextBlock* Label(UWidgetTree* Tree, const FText& Text, int32 Size, const TCHAR* Hex)
	{
		UTextBlock* Widget = Tree->ConstructWidget<UTextBlock>();
		Widget->SetText(Text);
		FSlateFontInfo Font = Widget->GetFont();
		Font.Size = Size;
		Widget->SetFont(Font);
		Widget->SetColorAndOpacity(FSlateColor(Color(Hex)));
		Widget->SetAutoWrapText(true);
		return Widget;
	}

	UButton* Button(UWidgetTree* Tree, const FText& Text, int32 Size = 18)
	{
		UButton* Widget = Tree->ConstructWidget<UButton>();
		FButtonStyle Style = Widget->GetStyle();
		Style.Normal.TintColor = FSlateColor(Color(TEXT("2A2420")));
		Style.Hovered.TintColor = FSlateColor(Color(TEXT("4A3C30")));
		Style.Pressed.TintColor = FSlateColor(Color(TEXT("1A1612")));
		Widget->SetStyle(Style);
		// 버튼 글자는 한 줄로 (좁아도 줄바꿈하지 않고 버튼이 글자에 맞춰 넓어짐)
		UTextBlock* Caption = Label(Tree, Text, Size, TEXT("F2E4D0"));
		Caption->SetAutoWrapText(false);
		Widget->SetContent(Caption);
		CastChecked<UButtonSlot>(Widget->GetContent()->Slot)->SetPadding(FMargin(18, 10));
		return Widget;
	}
}

// ── 장소 버튼 ──

void USSTalkSpotButtonWidget::Setup(USSCompanionTalkWidget* InOwner, ESSInvestigationSpot InSpot)
{
	OwnerTalk = InOwner;
	Spot = InSpot;
}

TSharedRef<SWidget> USSTalkSpotButtonWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		// 장소 이름만 보여줌 (위험도·확률 숫자는 플레이어에게 안 보임)
		Button = SSTalkStyle::Button(WidgetTree, FSSInvestigation::GetSpotName(Spot), 16);
		WidgetTree->RootWidget = Button;
	}
	return Super::RebuildWidget();
}

void USSTalkSpotButtonWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (Button) Button->OnClicked.AddUniqueDynamic(this, &ThisClass::OnClicked);
}

void USSTalkSpotButtonWidget::NativeDestruct()
{
	if (Button) Button->OnClicked.RemoveDynamic(this, &ThisClass::OnClicked);
	Super::NativeDestruct();
}

void USSTalkSpotButtonWidget::OnClicked()
{
	if (IsValid(OwnerTalk)) OwnerTalk->OnSpotChosen(Spot);
}

// ── 대화창 ──

void USSCompanionTalkWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (!NameText || !LineText || !ClueCard || !ClueHeaderText || !ClueBodyText
		|| !ReportButton || !OrderButton || !SpotList || !HintText || !CloseButton)
	{
		UE_LOG(LogTemp, Error, TEXT("[CompanionTalk] Required WBP widgets are missing."));
		return;
	}
	ClueCard->SetVisibility(ESlateVisibility::Collapsed);
	SpotList->SetVisibility(ESlateVisibility::Collapsed);
	SpotList->ClearChildren();
	for (int32 Index = 0; Index < static_cast<int32>(ESSInvestigationSpot::Count); ++Index)
	{
		USSTalkSpotButtonWidget* SpotButton = CreateWidget<USSTalkSpotButtonWidget>(this);
		if (!SpotButton) continue;
		SpotButton->Setup(this, static_cast<ESSInvestigationSpot>(Index));
		UHorizontalBoxSlot* SpotSlot = SpotList->AddChildToHorizontalBox(SpotButton);
		SpotSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		SpotSlot->SetPadding(FMargin(0, 0, 8, 0));
	}
	if (UGameInstance* GameInstance = GetGameInstance()) RunSubsystem = GameInstance->GetSubsystem<USSRunSubsystem>();

	ReportButton->OnClicked.AddUniqueDynamic(this, &ThisClass::OnReportClicked);
	OrderButton->OnClicked.AddUniqueDynamic(this, &ThisClass::OnOrderClicked);
	CloseButton->OnClicked.AddUniqueDynamic(this, &ThisClass::OnCloseClicked);

	// 보고를 듣거나 장소를 정하거나 행동력이 바뀌면 버튼 상태를 다시 맞춤
	if (IsValid(RunSubsystem))
	{
		RunSubsystem->OnSurvivorsChanged.AddUniqueDynamic(this, &ThisClass::RefreshChoices);
		RunSubsystem->OnActionPointsChanged.AddUniqueDynamic(this, &ThisClass::RefreshChoices);
	}

	const FSSSurvivorState* Survivor = IsValid(RunSubsystem) ? RunSubsystem->FindRescuedSurvivor(SurvivorId) : nullptr;
	if (NameText && Survivor && IsValid(Survivor->Definition))
	{
		NameText->SetText(Survivor->Definition->DisplayName);
	}

	// 첫마디 (대사 표): 밀린 보고 > 보고 하나 > 오늘 밤 장소 정해 둠 > 평소 인사
	if (IsValid(RunSubsystem))
	{
		const USSCompanionState* Companions = RunSubsystem->GetCompanions();
		const FSSCompanionRecord* Record = Companions->FindRecord(SurvivorId);
		if (Record && Record->PendingReports.Num() > 1)
		{
			Say(Companions->GetLine(SurvivorId, ESSCompanionLine::Backlog));
		}
		else if (Record && Record->HasSomethingToSay())
		{
			Say(Companions->GetLine(SurvivorId, ESSCompanionLine::ReportReady));
		}
		else if (Record && Record->bHasOrder)
		{
			Say(Companions->GetLine(SurvivorId, ESSCompanionLine::OrderReminder, Record->OrderedSpot));
		}
		else
		{
			Say(Companions->GetLine(SurvivorId, ESSCompanionLine::Greet));
		}
	}

	RefreshChoices();
}

void USSCompanionTalkWidget::NativeDestruct()
{
	if (ReportButton) ReportButton->OnClicked.RemoveDynamic(this, &ThisClass::OnReportClicked);
	if (OrderButton) OrderButton->OnClicked.RemoveDynamic(this, &ThisClass::OnOrderClicked);
	if (CloseButton) CloseButton->OnClicked.RemoveDynamic(this, &ThisClass::OnCloseClicked);
	if (IsValid(RunSubsystem))
	{
		RunSubsystem->OnSurvivorsChanged.RemoveDynamic(this, &ThisClass::RefreshChoices);
		RunSubsystem->OnActionPointsChanged.RemoveDynamic(this, &ThisClass::RefreshChoices);
	}
	RunSubsystem = nullptr;
	Super::NativeDestruct();
}

void USSCompanionTalkWidget::RefreshChoices()
{
	if (!IsValid(RunSubsystem) || !ReportButton || !OrderButton || !SpotList || !HintText) return;

	const FSSSurvivorState* Survivor = RunSubsystem->FindRescuedSurvivor(SurvivorId);
	const bool bAlive = Survivor && Survivor->bAlive;
	const FSSCompanionRecord* Record = RunSubsystem->GetCompanions()->FindRecord(SurvivorId);

	// 보고: 안 들은 보고가 있을 때만. 밀린 개수를 버튼에 표시
	const int32 PendingCount = Record ? Record->PendingReports.Num() + (Record->bPendingTestimony ? 1 : 0) : 0;
	ReportButton->SetIsEnabled(bAlive && PendingCount > 0);
	if (UTextBlock* ReportLabel = Cast<UTextBlock>(ReportButton->GetContent()))
	{
		ReportLabel->SetText(PendingCount > 1
			? FText::Format(NSLOCTEXT("SSTalk", "ReportCount", "조사 보고 듣기 ({0})"), PendingCount)
			: NSLOCTEXT("SSTalk", "Report", "조사 보고 듣기"));
	}

	// 장소 정하기: 오늘 아직 안 정했고 행동력이 있을 때만
	const bool bOrdered = Record && Record->bHasOrder;
	const bool bEnoughPoints = RunSubsystem->GetActionPoints() >= USSCompanionState::InvestigationOrderCost;
	const bool bCanOrder = bAlive && !bOrdered && bEnoughPoints;
	OrderButton->SetIsEnabled(bCanOrder);
	if (!bCanOrder) SpotList->SetVisibility(ESlateVisibility::Collapsed);

	// 왜 못 누르는지 안내
	FText Hint = NSLOCTEXT("SSTalk", "HintFree", "장소를 정하지 않으면 동료가 알아서 고른다.");
	if (bOrdered)
	{
		Hint = FText::Format(NSLOCTEXT("SSTalk", "HintOrdered", "오늘 밤 조사 장소: {0} (정해 둠)"),
			FSSInvestigation::GetSpotName(Record->OrderedSpot));
	}
	else if (!bEnoughPoints)
	{
		Hint = NSLOCTEXT("SSTalk", "HintNoPoints", "행동력이 부족해서 장소를 정해 줄 수 없다.");
	}
	HintText->SetText(Hint);
}

void USSCompanionTalkWidget::OnReportClicked()
{
	if (!IsValid(RunSubsystem)) return;

	USSCompanionState* Companions = RunSubsystem->GetCompanions();

	// B2에서 돌아온 뒤 첫 대화: 증언 (숨은 진실)
	FText Testimony;
	if (Companions->HearTestimony(SurvivorId, Testimony))
	{
		Say(Testimony);
		ShowTruthCard();
		RunSubsystem->AddHiddenTruth(SSRescueIds::TestimonyTruth(), NSLOCTEXT("SSTalk", "TestimonyJournal",
			"서하린의 증언: 운영진은 스스로 배우는 아라를 두려워해 폐기를 정했다. 방법은 연구소 전체 정화. 안에 있던 사람은 대피 명단에 없었다."));
		RefreshChoices();
		return;
	}

	FSSInvestigationReport Report;
	if (!Companions->HearInvestigationReport(SurvivorId, Report)) return;

	// 동료 대사 (대사 표) + 단서가 있으면 단서 카드
	Say(Companions->GetReportLine(Report));
	ShowClueCard(Report);
}

void USSCompanionTalkWidget::ShowTruthCard()
{
	// 단서 카드 자리에 숨은 진실 카드
	ClueHeaderText->SetText(NSLOCTEXT("SSTalk", "TruthHeader", "숨은 진실 · 서하린의 증언"));
	ClueBodyText->SetText(NSLOCTEXT("SSTalk", "TruthBody",
		"폐기 회의\n운영진은 스스로 발전한 아라를 두려워해 폐기를 결정했다. 방법은 연구소 전체 정화(소각) 프로토콜. 직원 대피 계획은 없었다."));
	ClueCard->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void USSCompanionTalkWidget::ShowClueCard(const FSSInvestigationReport& Report)
{
	const USSCompanionState* Companions = RunSubsystem->GetCompanions();
	const FSSClueRow* Clue = Report.bFoundClue ? Companions->FindClue(Report.ClueId) : nullptr;
	if (!Clue) return;

	// 언제 조사한 건지: 어젯밤이면 "어젯밤", 더 전이면 "N일째 밤"
	// (보고의 Day는 조사한 날, 지금은 그다음 날 이후)
	const FText When = Report.Day == RunSubsystem->GetCurrentDay() - 1
		? NSLOCTEXT("SSTalk", "LastNight", "어젯밤")
		: FText::Format(NSLOCTEXT("SSTalk", "NightOfDay", "{0}일째 밤"), Report.Day);

	// 예: 어젯밤 · 단서 · 단말 로그 2/3
	ClueHeaderText->SetText(FText::Format(NSLOCTEXT("SSTalk", "ClueHeader", "{0} · 단서 · {1} {2}/{3}"),
		When, FSSInvestigation::GetSpotName(Report.Spot), Clue->Stage, Companions->CountClues(Report.Spot)));
	ClueBodyText->SetText(FText::Format(NSLOCTEXT("SSTalk", "ClueBody", "{0}\n{1}"), Clue->Title, Clue->Text));
	ClueCard->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void USSCompanionTalkWidget::OnOrderClicked()
{
	if (!SpotList || !OrderButton || !OrderButton->GetIsEnabled()) return;
	// 장소 버튼 줄을 펼치거나 접음
	const bool bOpen = SpotList->GetVisibility() != ESlateVisibility::Collapsed;
	SpotList->SetVisibility(bOpen ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
}

void USSCompanionTalkWidget::OnSpotChosen(ESSInvestigationSpot Spot)
{
	if (!IsValid(RunSubsystem)) return;

	// 성공하면 행동력 1이 빠지고, 오늘 밤 그 장소를 조사함
	if (RunSubsystem->GetCompanions()->OrderInvestigation(SurvivorId, Spot))
	{
		SpotList->SetVisibility(ESlateVisibility::Collapsed);
		Say(RunSubsystem->GetCompanions()->GetLine(SurvivorId, ESSCompanionLine::OrderAccept, Spot));
	}
}

void USSCompanionTalkWidget::OnCloseClicked()
{
	RemoveFromParent();
}

void USSCompanionTalkWidget::Say(const FText& Line)
{
	if (LineText) LineText->SetText(Line);
	if (ClueCard) ClueCard->SetVisibility(ESlateVisibility::Collapsed);
}
