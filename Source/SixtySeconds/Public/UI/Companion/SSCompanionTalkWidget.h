#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Companion/SSInvestigation.h"
#include "SSCompanionTalkWidget.generated.h"

class USSRunSubsystem;
class USSCompanionTalkWidget;
class UButton;
class UTextBlock;
class UHorizontalBox;
class UBorder;
struct FSSInvestigationReport;

// 조사 장소 버튼 하나 (어느 장소인지 기억해서 대화창에 알려줌)
UCLASS()
class SIXTYSECONDS_API USSTalkSpotButtonWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 버튼을 만들기 전에 장소와 주인 창을 정해줌
	void Setup(USSCompanionTalkWidget* InOwner, ESSInvestigationSpot InSpot);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void OnClicked();

	// 이 버튼이 고르는 장소
	ESSInvestigationSpot Spot = ESSInvestigationSpot::Storage;

	// 눌렸을 때 알려줄 대화창
	UPROPERTY(Transient)
	TObjectPtr<USSCompanionTalkWidget> OwnerTalk;

	UPROPERTY(Transient)
	TObjectPtr<UButton> Button;
};

// ─────────────────────────────────────────────
// 동료와 대화하는 창 (정보창의 [대화하기]로 열림)
// - 조사 보고 듣기: 들어야 기록창에 남음. 단서 보고는 들을 때까지 보존
// - 오늘 밤 조사 장소 정해주기: 행동력 1, 하루 한 번
// 대사는 CSV에서 가져오고, 화면은 WBP_CompanionTalk에서 편집
// ─────────────────────────────────────────────
UCLASS(Abstract)
class SIXTYSECONDS_API USSCompanionTalkWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 누구와 대화하는지 (AddToViewport 전에 부름)
	void SetSurvivor(FName InSurvivorId) { SurvivorId = InSurvivorId; }

	// 장소 버튼이 눌렸을 때 (USSTalkSpotButtonWidget이 부름)
	void OnSpotChosen(ESSInvestigationSpot Spot);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	// 버튼 켜기/끄기와 안내 문구를 지금 상태에 맞춤
	UFUNCTION()
	void RefreshChoices();

	UFUNCTION()
	void OnReportClicked();

	UFUNCTION()
	void OnOrderClicked();

	UFUNCTION()
	void OnCloseClicked();

	// 대사 칸에 한 줄 보여줌 (단서 카드는 숨김)
	void Say(const FText& Line);

	// 보고에 단서가 있으면 대사 아래 단서 카드를 보여줌
	void ShowClueCard(const FSSInvestigationReport& Report);

	// 단서 카드 자리에 숨은 진실 카드 (구출된 하린의 증언)
	void ShowTruthCard();

	// 대화 상대
	FName SurvivorId = NAME_None;

	UPROPERTY(Transient)
	TObjectPtr<USSRunSubsystem> RunSubsystem;

	// 동료 이름
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> NameText;

	// 동료가 하는 말
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> LineText;

	// 단서 카드 (보고에 단서가 있을 때만 보임)
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UBorder> ClueCard;

	// 카드 윗줄: "어젯밤 · 단서 · 단말 로그 2/3"
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> ClueHeaderText;

	// 카드 본문: 단서 제목과 내용
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> ClueBodyText;

	// [어젯밤 조사 보고 듣기]
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> ReportButton;

	// [오늘 밤 조사 장소 정하기]
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> OrderButton;

	// 장소 버튼 5개가 들어가는 줄 (주문 버튼을 누르면 펼쳐짐)
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UHorizontalBox> SpotList;

	// 행동력·이미 정했는지 안내
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> HintText;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> CloseButton;
};
