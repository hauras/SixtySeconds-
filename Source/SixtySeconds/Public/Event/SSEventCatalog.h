#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SSEventCatalog.generated.h"

class UDataTable;
class USSItemDefinition;

// 사건 데이터 묶음. 엑셀 3시트(사건·선택지·효과)를 CSV로 저장해 DataTable로 임포트하고 여기에 연결한다.
UCLASS(BlueprintType)
class SIXTYSECONDS_API USSEventCatalog : public UDataAsset
{
	GENERATED_BODY()

public:
	// 행 구조가 맞는지는 Validate가 확인
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SS|Event")
	TObjectPtr<UDataTable> EventTable;    // 행: FSSEventRow

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SS|Event")
	TObjectPtr<UDataTable> ChoiceTable;   // 행: FSSEventChoiceRow

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SS|Event")
	TObjectPtr<UDataTable> EffectTable;   // 행: FSSEventEffectRow

	// 표에서 ItemId로 적은 아이템의 실제 에셋. 획득 효과가 이 목록에서 에셋을 찾는다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SS|Event")
	TArray<TObjectPtr<USSItemDefinition>> Items;

	// 하루가 시작될 때 사건이 일어날 확률 (예약된 사건은 확률과 상관없이 나옴)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SS|Event", meta=(ClampMin="0.0", ClampMax="1.0"))
	float DailyEventChance = 0.7f;

	USSItemDefinition* FindItem(FName ItemId) const;

	// 표 형식, 없는 사건·선택지·아이템 참조, 선택지 없는 사건, 잘못된 수치 검사
	bool Validate(TArray<FText>& OutErrors) const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
