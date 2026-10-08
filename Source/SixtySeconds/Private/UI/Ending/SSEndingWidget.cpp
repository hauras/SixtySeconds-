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
			"서버실 문 앞에서 서하린이 멈춰 섰다. 눈동자 안쪽에서 초록 불이 켜졌다.\n{0}"), Base);
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
		EndingStatsText->SetText(FText::Format(NSLOCTEXT("SSEnding", "Stats", "버틴 날 {0}일 · 구한 사람 {1}명 · 패널 경보 {2}회"),
			Report.DaysSurvived,
			Report.RescuedCount,
			Report.AlarmCount));
	}
}

void USSEndingWidget::OnRestartClicked()
{
	// 새 판: 진행 상태를 지우고 스크램블 맵부터
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (USSRunSubsystem* Run = GameInstance->GetSubsystem<USSRunSubsystem>()) Run->ResetRun();
	}
	RemoveFromParent();
	UGameplayStatics::OpenLevel(this, RestartLevel);
}
