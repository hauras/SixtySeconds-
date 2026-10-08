#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SSExplorationLootCard.generated.h"

class UButton;
class UImage;
class UTextBlock;
struct FSSItemStack;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSSOnLootCardClicked, int32, LootIndex); // 방 위젯의 OnRoomClicked와 같은 역할

// 물품 카드 한 장. 챙기기 창이 물자 개수만큼 코드로 만들어서 넣는다.
UCLASS(Abstract)
class SIXTYSECONDS_API USSExplorationLootCard : public UUserWidget
{
	GENERATED_BODY()

public:
	// 창이 카드를 만든 직후 부름: 몇 번째 물자인지 + 표시할 내용
	void Setup(int32 InLootIndex, const FSSItemStack& Stack);

	// 운반함에 들어가지 않으면 카드를 어둡게 (창 안에서 못 담는 이유는 공간 부족뿐)
	void SetAvailable(bool bCanTake);

	FSSOnLootCardClicked OnCardClicked; // C++에서만 구독

protected:
	virtual void NativeConstruct() override; // 버튼 클릭 연결
	virtual void NativeDestruct() override;  // 연결 해제

	UPROPERTY(meta=(BindWidget)) // 카드 전체가 버튼
	TObjectPtr<UButton> CardButton;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UImage> ItemIcon; // 아이템 DataAsset의 Icon

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> NameText; // DisplayName

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> QuantityText; // "×2"

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> CostText; // "1칸"

private:
	UFUNCTION()
	void HandleClicked(); // OnCardClicked.Broadcast(LootIndex)

	int32 LootIndex = INDEX_NONE;
};
