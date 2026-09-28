#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SSExplorationRoomWidget.generated.h"

class UButton;
class UTextBlock;
class UBorder;
class UImage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSSOnRoomClicked, int32, RoomIndex);   // 눌린 방 번호를 실어 보냄

// 탐사 지도 위 방 하나. WBP에 직접 배치하고 인스턴스마다 RoomId를 입력한다.
UCLASS(Abstract)   // 이 클래스 자체는 못 쓰고, WBP_ExplorationRoom으로만 쓰게 막음
class SIXTYSECONDS_API USSExplorationRoomWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 탐사 위젯이 시작할 때 부름: 방 번호와 이름을 받아 저장
	void Setup(int32 InRoomIndex, const FText& InDisplayName);

	// 탐사 위젯이 Refresh 때 부름: 모양만 바꿈 (판단은 안 함)
	void SetState(bool bIsCurrent, bool bCanMove, bool bGuardHere, bool bGuardNext);

	FName GetRoomId() const { return RoomId; }
	int32 GetRoomIndex() const { return RoomIndex; }
	// 추가
	FSSOnRoomClicked OnRoomClicked;   // C++에서만 구독하니까 UPROPERTY 없이도 됨

protected:
	virtual void NativeConstruct() override;   // 버튼 클릭 연결
	virtual void NativeDestruct() override;    // 연결 해제

	// EditAnywhere → 배치한 인스턴스마다 디자이너에서 다른 값을 입력할 수 있음
	UPROPERTY(EditAnywhere, Category="SS|Exploration")
	FName RoomId;

	// 상태별 색. 코드 수정 없이 디자이너에서 조정
	UPROPERTY(EditAnywhere, Category="SS|Exploration|Style")
	FLinearColor NormalColor = FLinearColor(0.2f, 0.2f, 0.2f);

	UPROPERTY(EditAnywhere, Category="SS|Exploration|Style")
	FLinearColor MovableColor = FLinearColor(0.2f, 0.6f, 1.f);

	UPROPERTY(EditAnywhere, Category="SS|Exploration|Style")
	FLinearColor CurrentColor = FLinearColor(0.1f, 0.8f, 0.5f);

	UPROPERTY(EditAnywhere, Category="SS|Exploration|Style")
	FLinearColor DangerColor = FLinearColor(0.9f, 0.25f, 0.2f);   // 경비가 다음 턴에 올 방

	UPROPERTY(meta=(BindWidget))           // 없으면 클릭을 못 받으니까 필수
	TObjectPtr<UButton> RoomButton;

	UPROPERTY(meta=(BindWidgetOptional))   // 나머지는 모양이라 없어도 동작
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UBorder> HighlightBorder;   // 상태 색을 입힐 테두리

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UImage> PlayerIcon;         // 현재 방일 때만 보임

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UImage> GuardIcon;          // 경비가 있는 방일 때만 보임

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> RoomStateText;  // 색상과 함께 현재 상태·순찰 예고 표시

private:
	UFUNCTION()               // AddDynamic으로 연결하려면 UFUNCTION 필수
	void HandleClicked();     // OnRoomClicked.Broadcast(RoomIndex)

	int32 RoomIndex = INDEX_NONE;   // Setup 전에는 -1
};
