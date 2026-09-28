#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "SSEventTypes.generated.h"

// 사건·선택지가 나오기 위한 조건. Target·Amount의 의미는 종류마다 다르다.
UENUM(BlueprintType)
enum class ESSEventCondition : uint8
{
	None,
	HasItem,           // Target = ItemId, Amount = 최소 개수 (0이면 1)
	AnySurvivorAlive,  // 살아 있는 동료가 한 명 이상
	SurvivorAlive,     // Target = SurvivorId
	RobotIdle,         // 탐사 로봇이 은신처에서 대기 중
	RobotAway,         // 탐사 로봇이 탐사 중
};

// 선택지를 고르면 일어나는 일
UENUM(BlueprintType)
enum class ESSEventEffect : uint8
{
	None,
	Item,              // Target = ItemId, Amount = +획득 / -손실
	PlayerHealth,      // Amount = ± 체력
	PlayerSatiety,     // Amount = ± 포만감
	PlayerHydration,   // Amount = ± 수분
	SurvivorsHealth,   // Amount = ± 살아 있는 모든 동료 체력
	ActionPoints,      // Amount = ± 오늘 행동력
	Journal,           // Text = 저널에 남길 문장
	ScheduleEvent,     // Target = EventId, Amount = 며칠 뒤 (1 이상)
};

// Events 시트 한 줄. 행 이름(Name 열)이 EventId.
USTRUCT(BlueprintType)
struct SIXTYSECONDS_API FSSEventRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Event") FText Title;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Event") FText Body;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Event") int32 MinDay = 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Event") int32 MaxDay = 0;      // 0 = 제한 없음
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Event") int32 Weight = 10;     // 뽑힐 확률 가중치
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Event") bool bOnceOnly = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Event") int32 Cooldown = 0;    // 나온 뒤 며칠 동안 다시 안 나옴
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Event") bool bScheduledOnly = false;   // 다른 선택의 결과로만 등장 (보답 사건 등)

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Event") ESSEventCondition Condition = ESSEventCondition::None;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Event") FName ConditionTarget = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Event") int32 ConditionAmount = 0;
};

// Choices 시트 한 줄. 행 이름이 ChoiceId.
USTRUCT(BlueprintType)
struct SIXTYSECONDS_API FSSEventChoiceRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Event") FName EventId = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Event") int32 Order = 0;       // 화면에 나오는 순서
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Event") FText Text;

	// 이 조건이 안 맞으면 선택지가 비활성
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Event") ESSEventCondition Condition = ESSEventCondition::None;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Event") FName ConditionTarget = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Event") int32 ConditionAmount = 0;
};

// Effects 시트 한 줄. 행 이름은 아무거나 겹치지 않게 (예: Knock_Open_1).
USTRUCT(BlueprintType)
struct SIXTYSECONDS_API FSSEventEffectRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Event") FName ChoiceId = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Event") ESSEventEffect Type = ESSEventEffect::None;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Event") FName Target = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Event") int32 Amount = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Event") float Chance = 1.f;    // 0~1, 이 효과가 일어날 확률
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Event") FText Text;
};

// 화면에 보여줄 선택지 하나 (규칙 판단 결과 포함)
struct FSSEventChoiceView
{
	FName ChoiceId = NAME_None;
	FText Text;
	bool bAvailable = false;
};
