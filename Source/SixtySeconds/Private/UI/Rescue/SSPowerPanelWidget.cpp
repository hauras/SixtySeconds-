#include "UI/Rescue/SSPowerPanelWidget.h"
#include "Rescue/SSRescueSession.h"
#include "Character/SSSurvivorDefinition.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Components/ButtonSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/SizeBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"

// ── 타일 하나 ──

void USSPowerTileWidget::Setup(USSPowerPanelWidget* InOwner, int32 InCell, float InSize)
{
	OwnerPanel = InOwner;
	Cell = InCell;
	Size = InSize;
}

TSharedRef<SWidget> USSPowerTileWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		// 칸 크기 고정 상자 → 버튼 → 그림
		USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>();
		Box->SetWidthOverride(Size);
		Box->SetHeightOverride(Size);

		Button = WidgetTree->ConstructWidget<UButton>();
		FButtonStyle TileStyle = Button->GetStyle();
		TileStyle.Normal.DrawAs = ESlateBrushDrawType::Image;
		TileStyle.Normal.TintColor = FSlateColor(FLinearColor(0.018f, 0.028f, 0.025f));
		TileStyle.Hovered.DrawAs = ESlateBrushDrawType::Image;
		TileStyle.Hovered.TintColor = FSlateColor(FLinearColor(0.045f, 0.13f, 0.12f));
		TileStyle.Pressed.DrawAs = ESlateBrushDrawType::Image;
		TileStyle.Pressed.TintColor = FSlateColor(FLinearColor(0.025f, 0.20f, 0.19f));
		Button->SetStyle(TileStyle);
		Button->SetToolTipText(NSLOCTEXT("SSPowerPanel", "RotateTile", "클릭 · 시계 방향으로 90° 회전"));
		Button->SetCursor(EMouseCursor::Hand);

		Image = WidgetTree->ConstructWidget<UImage>();
		Image->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));

		Button->SetContent(Image);
		if (UButtonSlot* ImageSlot = Cast<UButtonSlot>(Image->Slot))
		{
			ImageSlot->SetPadding(FMargin(0.f));
			ImageSlot->SetHorizontalAlignment(HAlign_Fill);
			ImageSlot->SetVerticalAlignment(VAlign_Fill);
		}
		// 칸 테두리를 얇게 남겨 클릭 범위가 보이게 한다.
		UBorder* Outline = WidgetTree->ConstructWidget<UBorder>();
		FSlateBrush OutlineBrush;
		OutlineBrush.DrawAs = ESlateBrushDrawType::Image;
		Outline->SetBrush(OutlineBrush);
		Outline->SetBrushColor(FLinearColor(0.10f, 0.13f, 0.12f));
		Outline->SetPadding(FMargin(1.f));
		Outline->SetContent(Button);
		Box->SetContent(Outline);
		WidgetTree->RootWidget = Box;
	}
	return Super::RebuildWidget();
}

void USSPowerTileWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (Button) Button->OnClicked.AddUniqueDynamic(this, &ThisClass::OnClicked);
}

void USSPowerTileWidget::NativeDestruct()
{
	if (Button) Button->OnClicked.RemoveDynamic(this, &ThisClass::OnClicked);
	Super::NativeDestruct();
}

void USSPowerTileWidget::Show(UTexture2D* Texture, float AngleDegrees, const FLinearColor& Tint)
{
	if (!Image) return;

	// 빈 칸(열린 면 없음)은 그림을 숨김
	if (!Texture)
	{
		Image->SetVisibility(ESlateVisibility::Hidden);
		return;
	}
	Image->SetVisibility(ESlateVisibility::HitTestInvisible);
	Image->SetBrushFromTexture(Texture);
	Image->SetColorAndOpacity(Tint);
	Image->SetRenderTransformAngle(AngleDegrees);
}

void USSPowerTileWidget::OnClicked()
{
	if (IsValid(OwnerPanel)) OwnerPanel->OnTileClicked(Cell);
}

// ── 패널 ──

bool USSPowerPanelWidget::ResolveTileShape(uint8 Mask, ESSPowerTileShape& OutShape, int32& OutQuarterTurns)
{
	using namespace SSPowerSide;

	// 그림마다 기본 방향의 열린 면
	struct FBaseShape
	{
		ESSPowerTileShape Shape;
		uint8 Mask;
	};
	static const FBaseShape BaseShapes[] = {
		{ ESSPowerTileShape::End, North },
		{ ESSPowerTileShape::Straight, North | South },
		{ ESSPowerTileShape::Corner, North | East },
		{ ESSPowerTileShape::Tee, North | East | South },
		{ ESSPowerTileShape::Cross, North | East | South | West },
	};

	// 기본 방향을 0~3번 돌려서 같아지는 그림을 찾음
	for (const FBaseShape& Base : BaseShapes)
	{
		uint8 Turned = Base.Mask;
		for (int32 Turns = 0; Turns < 4; ++Turns)
		{
			if (Turned == Mask)
			{
				OutShape = Base.Shape;
				OutQuarterTurns = Turns;
				return true;
			}
			Turned = FSSPowerGrid::RotateClockwise(Turned);
		}
	}
	OutShape = ESSPowerTileShape::None;
	OutQuarterTurns = 0;
	return false;
}

void USSPowerPanelWidget::SetSession(USSRescueSession* InSession, const USSSurvivorDefinition* InTarget)
{
	if (IsValid(Session) && SessionChangedHandle.IsValid())
	{
		Session->OnChanged.Remove(SessionChangedHandle);
	}
	Session = InSession;
	bAbortArmed = false;
	bFinishNotified = false;
	bResultShown = false;
	if (ResultOverlay) ResultOverlay->SetVisibility(ESlateVisibility::Collapsed);
	if (IsValid(Session))
	{
		SessionChangedHandle = Session->OnChanged.AddUObject(this, &ThisClass::Refresh);
	}

	// 구출 대상 카드
	if (InTarget)
	{
		TargetName = InTarget->DisplayName;
		TargetTexture = InTarget->Portrait ? InTarget->Portrait.Get() : InTarget->ShelterImage.Get();
		if (TargetNameText) TargetNameText->SetText(InTarget->DisplayName);
		if (TargetPortrait)
		{
			UTexture2D* PortraitTexture = TargetTexture;
			TargetPortrait->SetVisibility(PortraitTexture ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
			if (PortraitTexture) TargetPortrait->SetBrushFromTexture(PortraitTexture);
		}
	}

	BuildBoard();
	Refresh();
}

void USSPowerPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (!TileGrid || !MovesText || !ActionButton)
	{
		UE_LOG(LogTemp, Error, TEXT("[PowerPanel] Required WBP widgets are missing (TileGrid, MovesText, ActionButton)."));
		return;
	}
	ActionButton->OnClicked.AddUniqueDynamic(this, &ThisClass::OnActionClicked);
	if (ResultButton) ResultButton->OnClicked.AddUniqueDynamic(this, &ThisClass::OnResultClicked);
	if (ResultOverlay && !bResultShown) ResultOverlay->SetVisibility(ESlateVisibility::Collapsed);
	if (PanelLogText) PanelLogText->SetText(PanelLogLine);

	// 타일 그림은 화면에 붙은 뒤에 생기므로 여기서 한 번 더 그림
	Refresh();
}

void USSPowerPanelWidget::NativeDestruct()
{
	if (ActionButton) ActionButton->OnClicked.RemoveDynamic(this, &ThisClass::OnActionClicked);
	if (ResultButton) ResultButton->OnClicked.RemoveDynamic(this, &ThisClass::OnResultClicked);
	if (UWorld* World = GetWorld()) World->GetTimerManager().ClearTimer(RevealTimer);
	if (IsValid(Session) && SessionChangedHandle.IsValid())
	{
		Session->OnChanged.Remove(SessionChangedHandle);
	}
	SessionChangedHandle.Reset();
	Super::NativeDestruct();
}

UImage* USSPowerPanelWidget::AddMarkerSlot(UPanelWidget* Parent, UTexture2D* Texture)
{
	USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>();
	Box->SetWidthOverride(TileSize);
	// 경보 줄은 낮게 만들어 배선판이 화면을 더 크게 쓰게 한다.
	const bool bAlarmRow = Parent == TopAlarmRow.Get() || Parent == BottomAlarmRow.Get();
	Box->SetHeightOverride(bAlarmRow ? TileSize * 0.5f : TileSize);
	Parent->AddChild(Box);
	if (!Texture) return nullptr;

	UImage* Marker = WidgetTree->ConstructWidget<UImage>();
	Marker->SetBrushFromTexture(Texture);
	if (bAlarmRow)
	{
		Marker->SetDesiredSizeOverride(FVector2D(TileSize * 0.5f));
	}
	Box->SetContent(Marker);
	if (bAlarmRow)
	{
		if (USizeBoxSlot* MarkerSlot = Cast<USizeBoxSlot>(Marker->Slot))
		{
			MarkerSlot->SetHorizontalAlignment(HAlign_Center);
			MarkerSlot->SetVerticalAlignment(VAlign_Center);
		}
	}
	return Marker;
}

void USSPowerPanelWidget::BuildBoard()
{
	if (!IsValid(Session) || !TileGrid) return;
	const FSSPowerGrid& Grid = Session->GetGrid();

	// 1) 타일: 칸 번호 = 행 × 너비 + 열
	TileGrid->ClearChildren();
	Tiles.Reset();
	for (int32 Cell = 0; Cell < Grid.Tiles.Num(); ++Cell)
	{
		USSPowerTileWidget* Tile = CreateWidget<USSPowerTileWidget>(this);
		if (!Tile) continue;
		Tile->Setup(this, Cell, TileSize);
		TileGrid->AddChildToUniformGrid(Tile, Cell / Grid.Width, Cell % Grid.Width);
		Tiles.Add(Tile);
	}

	// 2) 경보 램프: 위·아래 줄의 열마다 자리, 단자가 있는 열에만 램프
	AlarmLamps.Init(nullptr, Grid.Alarms.Num());
	for (UHorizontalBox* Row : { TopAlarmRow.Get(), BottomAlarmRow.Get() })
	{
		if (!Row) continue;
		Row->ClearChildren();
		const uint8 RowSide = Row == TopAlarmRow ? SSPowerSide::North : SSPowerSide::South;
		for (int32 Col = 0; Col < Grid.Width; ++Col)
		{
			const int32 AlarmIndex = Grid.Alarms.IndexOfByPredicate([&Grid, Col, RowSide](const FSSPowerAlarm& Alarm)
			{
				return Alarm.Side == RowSide && Alarm.Cell % Grid.Width == Col;
			});
			UImage* Lamp = AddMarkerSlot(Row, AlarmIndex != INDEX_NONE ? AlarmLampIdle.Get() : nullptr);
			if (AlarmIndex != INDEX_NONE) AlarmLamps[AlarmIndex] = Lamp;
		}
	}

	// 3) 전원·잠금 표시: 판 양옆 줄의 해당 행에만 아이콘
	LockImage = nullptr;
	for (UVerticalBox* Column : { SourceColumn.Get(), LockColumn.Get() })
	{
		if (!Column) continue;
		Column->ClearChildren();
		const bool bSource = Column == SourceColumn;
		const int32 MarkerRow = (bSource ? Grid.SourceCell : Grid.LockCell) / Grid.Width;
		for (int32 Row = 0; Row < Grid.Height; ++Row)
		{
			UImage* Marker = AddMarkerSlot(Column, Row == MarkerRow ? (bSource ? SourceIcon.Get() : LockIcon.Get()) : nullptr);
			if (!bSource && Row == MarkerRow) LockImage = Marker;
		}
	}
}

UTexture2D* USSPowerPanelWidget::GetTileTexture(ESSPowerTileShape Shape) const
{
	switch (Shape)
	{
	case ESSPowerTileShape::End: return TileEnd;
	case ESSPowerTileShape::Straight: return TileStraight;
	case ESSPowerTileShape::Corner: return TileCorner;
	case ESSPowerTileShape::Tee: return TileTee;
	case ESSPowerTileShape::Cross: return TileCross;
	default: return nullptr;
	}
}

void USSPowerPanelWidget::Refresh()
{
	if (!IsValid(Session)) return;
	const FSSPowerGrid& Grid = Session->GetGrid();

	// 타일: 그림·회전·전력 색
	const TSet<int32> Powered = Grid.ComputePowered();
	for (int32 Cell = 0; Cell < Tiles.Num() && Cell < Grid.Tiles.Num(); ++Cell)
	{
		ESSPowerTileShape Shape;
		int32 QuarterTurns;
		ResolveTileShape(Grid.Tiles[Cell], Shape, QuarterTurns);
		Tiles[Cell]->Show(GetTileTexture(Shape), 90.f * QuarterTurns, Powered.Contains(Cell) ? PoweredTint : IdleTint);
	}

	// 경보 램프: 지금 전력이 닿은 단자만 켜짐
	const TArray<int32> Hot = Grid.GetHotAlarms();
	for (int32 Index = 0; Index < AlarmLamps.Num(); ++Index)
	{
		UTexture2D* LampTexture = Hot.Contains(Index) ? AlarmLampHot.Get() : AlarmLampIdle.Get();
		if (AlarmLamps[Index] && LampTexture)
		{
			AlarmLamps[Index]->SetBrushFromTexture(LampTexture);
			AlarmLamps[Index]->SetColorAndOpacity(Hot.Contains(Index) ? FLinearColor::White : FLinearColor(0.22f, 0.22f, 0.22f));
		}
	}

	const bool bUnlocked = Session->GetOutcome() == ESSRescueOutcome::Unlocked;
	if (LockImage) LockImage->SetColorAndOpacity(bUnlocked ? PoweredTint : FLinearColor::White);

	// 남은 회전
	MovesText->SetText(FText::Format(NSLOCTEXT("SSPowerPanel", "Moves", "{0} / {1}"),
		Session->GetRemainingMoves(),
		Session->GetMoveBudget()));
	if (bUnlocked)
	{
		MovesText->SetText(NSLOCTEXT("SSPowerPanel", "UnlockedBadge", "잠금 해제"));
	}
	if (MovesBar)
	{
		MovesBar->SetVisibility(Session->IsFinished() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
		const int32 Budget = FMath::Max(1, Session->GetMoveBudget());
		MovesBar->SetPercent(static_cast<float>(Session->GetRemainingMoves()) / Budget);
	}

	// 경보 카드
	if (AlarmPanel)
	{
		AlarmPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	if (AlarmText)
	{
		AlarmText->SetText(FText::Format(NSLOCTEXT("SSPowerPanel", "Alarm", "경보 전송 {0}회 · 아라가 패널 접근을 기록했습니다."),
			Session->GetAlarmCount()));
		if (Session->GetAlarmCount() == 0)
		{
			AlarmText->SetText(NSLOCTEXT("SSPowerPanel", "AlarmClear", "정상 · 접근 기록 없음\n붉은 경보 단자에는 전력을 연결하지 마세요."));
		}
	}

	// 상태 안내와 버튼 글자
	FText Status;
	switch (Session->GetOutcome())
	{
	case ESSRescueOutcome::Unlocked:
		Status = NSLOCTEXT("SSPowerPanel", "StatusUnlocked", "격리실 잠금이 풀렸다.");
		break;
	case ESSRescueOutcome::OutOfMoves:
		Status = NSLOCTEXT("SSPowerPanel", "StatusOut", "더 손댈 시간이 없다. 패널을 닫는다.");
		break;
	case ESSRescueOutcome::Aborted:
		Status = NSLOCTEXT("SSPowerPanel", "StatusAborted", "작업을 멈추고 덕트로 물러났다.");
		break;
	default:
		Status = bAbortArmed
			? NSLOCTEXT("SSPowerPanel", "StatusConfirmAbort", "한 번 더 누르면 철수한다. 오늘은 다시 열 수 없다.")
			: NSLOCTEXT("SSPowerPanel", "StatusHint", "전원과 격리실 잠금을 연결하세요. 경보 단자는 피하세요.");
		break;
	}
	if (StatusText)
	{
		StatusText->SetText(Status);
		StatusText->SetColorAndOpacity(FSlateColor(bUnlocked ? PoweredTint : FLinearColor(0.8f, 0.7f, 0.5f)));
	}
	if (ActionButtonText)
	{
		ActionButtonText->SetText(bUnlocked
			? NSLOCTEXT("SSPowerPanel", "RescueReturn", "구출 완료 · 돌아가기")
			: Session->IsFinished()
			? NSLOCTEXT("SSPowerPanel", "Return", "돌아가기")
			: NSLOCTEXT("SSPowerPanel", "Abort", "작업 중단"));
	}

	// 끝나면 배선판을 잠그고, 결과가 확정될 때까지 기존 버튼도 잠금
	// (결과 카드 버튼이 없으면 결과 확정 뒤 기존 버튼으로 닫음)
	const bool bFinished = Session->IsFinished();
	TileGrid->SetIsEnabled(!bFinished);
	ActionButton->SetIsEnabled(!bFinished || (bResultShown && !ResultButton));
	if (bFinished && !bFinishNotified) BeginFinishReveal();
}

void USSPowerPanelWidget::BeginFinishReveal()
{
	bFinishNotified = true;

	// 성공은 잠금이 켜지는 모습을 보여준 뒤, 실패·중단은 짧게
	const float Delay = Session->GetOutcome() == ESSRescueOutcome::Unlocked ? UnlockRevealSeconds : FailRevealSeconds;
	UWorld* World = GetWorld();
	if (!World)
	{
		NotifyFinished();
		return;
	}
	World->GetTimerManager().SetTimer(RevealTimer, this, &ThisClass::NotifyFinished, Delay, false);
}

void USSPowerPanelWidget::NotifyFinished()
{
	OnFinished.Broadcast();
}

void USSPowerPanelWidget::ShowResult(const FSSRescueReport& Report)
{
	bResultShown = true;
	const bool bUnlocked = Report.Outcome == ESSRescueOutcome::Unlocked;
	const bool bOutOfMoves = Report.Outcome == ESSRescueOutcome::OutOfMoves;
	bResultSuccess = bUnlocked;
	ResultRevealElapsed = 0.f;

	// ── 결과마다 글자와 색을 한 번에 정함 ──
	// 큰 글자 / 작은 글자 / 이름 줄 / 설명 / 이동 / 위치
	FText Heading;
	FText Subtitle;
	FText NameLine;
	FText Description;
	FText Transfer;
	FText Location;
	FLinearColor HeadingColor = FLinearColor(0.6f, 0.6f, 0.6f);
	if (bUnlocked)
	{
		Heading = NSLOCTEXT("SSPowerPanel", "SuccessHeading", "구출 성공");
		Subtitle = NSLOCTEXT("SSPowerPanel", "ResultUnlockedTitle", "격리실 잠금 해제");
		NameLine = FText::Format(NSLOCTEXT("SSPowerPanel", "ResultUnlockedBody", "{0} 귀환"), TargetName);
		Description = NSLOCTEXT("SSPowerPanel", "ReturnedDescription", "동료가 은신처로 돌아왔습니다.");
		Transfer = NSLOCTEXT("SSPowerPanel", "TransferSuccess", "B2 격리  →  B1 은신처");
		Location = NSLOCTEXT("SSPowerPanel", "ShelterReturned", "B1 은신처 복귀");
		HeadingColor = PoweredTint;
	}
	else
	{
		Heading = bOutOfMoves
			? NSLOCTEXT("SSPowerPanel", "FailureHeading", "구출 실패")
			: NSLOCTEXT("SSPowerPanel", "ResultAbortTitle", "작업 중단");
		Subtitle = bOutOfMoves
			? NSLOCTEXT("SSPowerPanel", "ResultOutTitle", "배선 연결 실패")
			: NSLOCTEXT("SSPowerPanel", "ResultAbortSubtitle", "덕트로 철수");
		NameLine = FText::Format(NSLOCTEXT("SSPowerPanel", "StillCapturedName", "{0} · B2 격리"), TargetName);
		Description = bOutOfMoves
			? NSLOCTEXT("SSPowerPanel", "ResultOutBody", "더 손댈 시간이 없었다.")
			: NSLOCTEXT("SSPowerPanel", "ResultAbortBody", "오늘은 다시 열 수 없다.");
		Transfer = NSLOCTEXT("SSPowerPanel", "TransferFailed", "B2 격리  ×  B1 은신처");
		Location = NSLOCTEXT("SSPowerPanel", "CapturedRemains", "B2 격리 유지");
		if (bOutOfMoves) HeadingColor = FailTint;
	}

	// 바꿔치기: 실제 구출 처리에서 교체가 확인된 경우에만
	FText Notice;
	if (bUnlocked && Report.bReplacedAndroid)
	{
		Notice = FText::Format(NSLOCTEXT("SSPowerPanel", "ResultReplaced", "은신처에 있던 같은 얼굴의 {0}: 작동 정지."), TargetName);
	}

	// 대가 (작게): 실제로 쓴 행동력, 경보
	const FText Cost = FText::Format(NSLOCTEXT("SSPowerPanel", "CostBadge", "행동력 {0} 소모"), Report.ActionPointsSpent);
	const FText Alarm = FText::Format(NSLOCTEXT("SSPowerPanel", "AlarmBadge", "경보 {0}회"), Report.AlarmCount);
	const FText AlarmRecorded = NSLOCTEXT("SSPowerPanel", "AlarmRecorded", "아라가 패널 접근을 기록했습니다.");

	// 배지가 둘 다 있으면 상세 줄은 경보 기록만, 배지가 없으면 상세 줄 하나에 전부
	const bool bHasBadges = ResultCostText && ResultAlarmCountText;
	FText Detail = bHasBadges
		? AlarmRecorded
		: FText::Format(NSLOCTEXT("SSPowerPanel", "ResultCost", "{0} · {1}"), Cost, Alarm);
	if (!bHasBadges && Report.AlarmCount > 0)
	{
		Detail = FText::Format(NSLOCTEXT("SSPowerPanel", "ResultCostAlarm", "{0} · {1}"), Detail, AlarmRecorded);
	}

	// ── 화면에 채우기 ──
	if (ResultOverlay) ResultOverlay->SetVisibility(ESlateVisibility::Visible);
	if (ResultTitleText)
	{
		ResultTitleText->SetText(Heading);
		ResultTitleText->SetColorAndOpacity(FSlateColor(HeadingColor));
	}
	if (ResultSubtitleText) ResultSubtitleText->SetText(Subtitle);
	if (ResultBodyText) ResultBodyText->SetText(NameLine);
	if (ResultDescriptionText) ResultDescriptionText->SetText(Description);
	if (ResultNoticeText)
	{
		ResultNoticeText->SetText(Notice);
		ResultNoticeText->SetColorAndOpacity(FSlateColor(NoticeTint));
		ResultNoticeText->SetVisibility(Notice.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	if (ResultTransferText)
	{
		ResultTransferText->SetText(Transfer);
		ResultTransferText->SetColorAndOpacity(FSlateColor(bUnlocked ? PoweredTint : FLinearColor(0.4f, 0.4f, 0.4f)));
	}
	if (ResultLocationText)
	{
		ResultLocationText->SetText(Location);
		ResultLocationText->SetColorAndOpacity(FSlateColor(bUnlocked ? PoweredTint : NoticeTint));
	}
	if (ResultCostText) ResultCostText->SetText(Cost);
	if (ResultAlarmCountText)
	{
		ResultAlarmCountText->SetText(Alarm);
		ResultAlarmCountText->SetColorAndOpacity(FSlateColor(Report.AlarmCount > 0 ? NoticeTint : FLinearColor(0.5f, 0.5f, 0.5f)));
	}
	if (ResultDetailText)
	{
		ResultDetailText->SetText(Detail);
		const bool bShowDetail = !bHasBadges || Report.AlarmCount > 0;
		ResultDetailText->SetVisibility(bShowDetail ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	// 얼굴 대신 인원과 격리 상태 그림
	if (ResultPortrait)
	{
		UTexture2D* StateTexture = bUnlocked ? ResultSuccessTexture.Get() : ResultFailureTexture.Get();
		ResultPortrait->SetVisibility(StateTexture ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (StateTexture) ResultPortrait->SetBrushFromTexture(StateTexture);
		ResultPortrait->SetColorAndOpacity(FLinearColor::White);
	}

	// 버튼 색: 성공 청록 / 실패 주황
	if (ResultButton)
	{
		FButtonStyle Style = ResultButton->GetStyle();
		Style.Normal.TintColor = FSlateColor(bUnlocked ? FLinearColor(0.025f, 0.12f, 0.11f) : FLinearColor(0.14f, 0.09f, 0.035f));
		Style.Hovered.TintColor = FSlateColor(bUnlocked ? FLinearColor(0.05f, 0.23f, 0.20f) : FLinearColor(0.25f, 0.15f, 0.06f));
		Style.Pressed.TintColor = Style.Normal.TintColor;
		ResultButton->SetStyle(Style);
	}

	// 등장 연출 시작값 (NativeTick이 키움). 파동은 성공일 때만
	if (ResultPulse) ResultPulse->SetVisibility(bUnlocked ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (ResultCardFrame)
	{
		ResultCardFrame->SetRenderOpacity(0.f);
		ResultCardFrame->SetRenderScale(FVector2D(0.94f));
	}

	// 배선판·중단 버튼 잠금 (카드 버튼이 없으면 기존 버튼으로 닫기)
	Refresh();

	// 카드가 없으면 아래 상태 글자에 요약을 남김 (Refresh가 덮어쓰지 않게 그 뒤에)
	if (!ResultOverlay && StatusText)
	{
		StatusText->SetText(FText::Format(NSLOCTEXT("SSPowerPanel", "ResultFallback", "{0} · {1}\n{2}"), Heading, NameLine, Detail));
	}
}

void USSPowerPanelWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!bResultShown || ResultRevealElapsed >= 1.2f) return;
	ResultRevealElapsed = FMath::Min(1.2f, ResultRevealElapsed + InDeltaTime);
	// 카드가 부드럽게 나타나고 성공 파동은 한 번 퍼진 뒤 사라진다.
	const float CardAlpha = FMath::Clamp(ResultRevealElapsed / 0.35f, 0.f, 1.f);
	if (ResultCardFrame)
	{
		ResultCardFrame->SetRenderOpacity(CardAlpha);
		ResultCardFrame->SetRenderScale(FVector2D(FMath::InterpEaseOut(0.94f, 1.f, CardAlpha, 3.f)));
	}
	if (ResultPulse && bResultSuccess)
	{
		const float PulseAlpha = ResultRevealElapsed / 1.2f;
		ResultPulse->SetRenderScale(FVector2D(1.f + PulseAlpha * 0.25f));
		ResultPulse->SetRenderOpacity(1.f - PulseAlpha);
	}
}

void USSPowerPanelWidget::OnResultClicked()
{
	if (bResultShown) OnClosed.Broadcast();
}

void USSPowerPanelWidget::OnTileClicked(int32 Cell)
{
	if (!IsValid(Session)) return;

	// 타일을 만지면 중단 확인은 풀림
	bAbortArmed = false;
	Session->Rotate(Cell);   // 바뀌면 세션 알림으로 Refresh
}

void USSPowerPanelWidget::OnActionClicked()
{
	if (!IsValid(Session)) return;

	// 진행 중: 첫 번째는 확인, 두 번째에 중단
	if (!Session->IsFinished())
	{
		if (!bAbortArmed)
		{
			bAbortArmed = true;
			Refresh();
			return;
		}
		Session->Abort();
		return;   // 결과 카드가 뜬 뒤에 닫을 수 있음
	}

	// 결과가 확정된 뒤에만 닫힘 (연출 중에 눌러도 무시)
	if (bResultShown) OnClosed.Broadcast();
}

