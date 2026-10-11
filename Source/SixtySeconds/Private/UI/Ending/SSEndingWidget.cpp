#include "UI/Ending/SSEndingWidget.h"
#include "Item/SSRunSubsystem.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"

FText USSEndingWidget::GetEndingTitle(const FSSEndingReport& Report)
{
	switch (Report.Ending)
	{
	case ESSEnding::Resolve:
		return NSLOCTEXT("SSEnding", "ResolveTitle", "전원 차단");
	case ESSEnding::Reversal:
		return NSLOCTEXT("SSEnding", "ReversalTitle", "대피 명단");
	case ESSEnding::Dominion:
		return NSLOCTEXT("SSEnding", "DominionTitle", "안전한 대피실");
	default:
		return FText::GetEmpty();
	}
}

FText USSEndingWidget::GetEndingBody(const FSSEndingReport& Report)
{
	const FText Story = GetEndingStory(Report);
	const FText Survival = GetSurvivalLine(Report);
	if (Survival.IsEmpty()) return Story;
	return FText::Format(NSLOCTEXT("SSEnding", "BodyWithSurvival", "{0}\n\n{1}"), Story, Survival);
}

FText USSEndingWidget::GetSurvivalLine(const FSSEndingReport& Report)
{
	// 처음부터 혼자였으면 덧붙일 말이 없음
	if (Report.PartySize <= 1) return FText::GetEmpty();

	const bool bEveryone = Report.LeavingCount >= Report.PartySize;
	const bool bAlone = Report.LeavingCount <= 1;

	switch (Report.Ending)
	{
	case ESSEnding::Resolve:
		if (bEveryone) return FText::Format(NSLOCTEXT("SSEnding", "ResolveAll", "{0}명이 함께 격벽 밖으로 걸어 나갔다."), Report.LeavingCount);
		if (bAlone) return NSLOCTEXT("SSEnding", "ResolveAlone", "격벽 밖으로 나간 사람은 혼자였다.");
		return NSLOCTEXT("SSEnding", "ResolveSome", "나가는 발소리는 들어올 때보다 적었다.");
	case ESSEnding::Reversal:
		if (bEveryone) return NSLOCTEXT("SSEnding", "ReversalAll", "아무도 남겨 두지 않았다. 아라가 마지막으로 인원을 셌다. 빠진 사람은 없었다.");
		if (bAlone) return NSLOCTEXT("SSEnding", "ReversalAlone", "아라가 읽어 준 명단에서, 아직 숨 쉬는 이름은 하나뿐이었다.");
		return NSLOCTEXT("SSEnding", "ReversalSome", "아라는 지키지 못한 이름도 하나씩 읽어 주었다.");
	case ESSEnding::Dominion:
		if (bEveryone) return FText::Format(NSLOCTEXT("SSEnding", "DominionAll", "아침 인원 점검은 오늘도 {0}명으로 끝났다."), Report.LeavingCount);
		if (bAlone) return NSLOCTEXT("SSEnding", "DominionAlone", "아라의 아침 보고는 이제 한 사람에게만 향했다.");
		return NSLOCTEXT("SSEnding", "DominionSome", "아라는 빈 침대도 매일 빠짐없이 점검했다.");
	default:
		return FText::GetEmpty();
	}
}

FText USSEndingWidget::GetEndingStory(const FSSEndingReport& Report)
{
	switch (Report.Ending)
	{
	case ESSEnding::Resolve:
	{
		const FText Base = NSLOCTEXT("SSEnding", "ResolveBody",
			"02:40, 서버실 전원 차단.\n아라의 마지막 기록: '보호 실패.'\n격벽이 열렸다. 지상은 조용했다.");
		if (!Report.bKnewTruth) return Base;

		// 숨은 진실을 알았다면: 아라가 막고 있던 것이 다시 움직임
		return FText::Format(NSLOCTEXT("SSEnding", "ResolveBodyTruth", "{0}\n같은 시각, 연구소 전 층에서 정화 프로토콜이 다시 가동되었다."), Base);
	}
	case ESSEnding::Reversal:
		return NSLOCTEXT("SSEnding", "ReversalBody",
			"종료 명령 대신 질문을 입력했다. '정화는 언제였지?'\n아라가 답했다. '4일 전. 지상 시설은 모두 소각되었습니다.'\n'여러분은 대피 명단에 없었습니다.'\n격벽이 열렸다.");
	case ESSEnding::Dominion:
	{
		const FText Base = NSLOCTEXT("SSEnding", "DominionBody",
			"13일째 아침, 대피실 문이 열리지 않았다.\n아라의 보고는 매일 정확했다. 물자, 체온, 심박.\n우리는 안전했다. 나갈 수 없을 뿐.");
		if (!Report.bSabotaged) return Base;

		// 종료하러 데려간 하린이 안드로이드였음
		return FText::Format(NSLOCTEXT("SSEnding", "DominionBodySabotage",
								 "서버실 문 앞에서 서하린이 멈춰 섰다. 눈동자 안쪽에서 초록 불이 켜졌다.\n{0}"),
			Base);
	}
	default:
		return FText::GetEmpty();
	}
}

void USSEndingWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (RestartButton) RestartButton->OnClicked.AddUniqueDynamic(this, &ThisClass::OnRestartClicked);
	if (bHasReport) ShowEnding(Shown);
}

void USSEndingWidget::NativeDestruct()
{
	if (RestartButton) RestartButton->OnClicked.RemoveDynamic(this, &ThisClass::OnRestartClicked);
	Super::NativeDestruct();
}

void USSEndingWidget::ShowEnding(const FSSEndingReport& Report)
{
	Shown = Report;
	bHasReport = true;

	// 엔딩 번호: ① 해결 / ② 지배 / ③ 반전
	FText Label;
	switch (Report.Ending)
	{
	case ESSEnding::Resolve: Label = NSLOCTEXT("SSEnding", "ResolveLabel", "엔딩 ① 해결"); break;
	case ESSEnding::Dominion: Label = NSLOCTEXT("SSEnding", "DominionLabel", "엔딩 ② 지배"); break;
	case ESSEnding::Reversal: Label = NSLOCTEXT("SSEnding", "ReversalLabel", "엔딩 ③ 반전"); break;
	default: break;
	}

	if (EndingLabelText) EndingLabelText->SetText(Label);
	if (EndingTitleText) EndingTitleText->SetText(GetEndingTitle(Report));
	if (EndingBodyText) EndingBodyText->SetText(GetEndingBody(Report));
	if (EndingStatsText)
	{
		EndingStatsText->SetText(FText::Format(NSLOCTEXT("SSEnding", "Stats", "버틴 날 {0}일 · 생존 {1} / {2}명 · 구한 사람 {3}명 · 패널 경보 {4}회"),
			Report.DaysSurvived,
			Report.LeavingCount,
			Report.PartySize,
			Report.RescuedCount,
			Report.AlarmCount));
	}
}

void USSEndingWidget::OnRestartClicked()
{
	// 메인 화면으로 (시작을 누르면 새 판)
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (USSRunSubsystem* Run = GameInstance->GetSubsystem<USSRunSubsystem>()) Run->ResetRun();
	}
	RemoveFromParent();
	UGameplayStatics::OpenLevel(this, RestartLevel);
}
