#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Math/RandomStream.h"
#include "Event/SSEventTypes.h"
#include "SSEventDirector.generated.h"

class USSEventCatalog;
class USSRunSubsystem;

// 하루 사건의 규칙: 오늘 어떤 사건이 나올지 고르고, 고른 선택지의 효과를 RunSubsystem에 적용한다.
// 런이 끝날 때까지 유지되는 상태(1회성·쿨다운·예약)를 가진다. RunSubsystem이 소유.
UCLASS()
class SIXTYSECONDS_API USSEventDirector : public UObject
{
	GENERATED_BODY()

public:
	// 카탈로그를 연결하고 표를 색인. 검사 실패하면 false (사건이 나오지 않음)
	bool SetCatalog(USSEventCatalog* InCatalog);
	const USSEventCatalog* GetCatalog() const { return Catalog; }

	// 오늘 나올 사건. 예약된 사건이 먼저, 없으면 확률·가중치로 뽑음. 없으면 NAME_None
	FName PickEventForToday(const USSRunSubsystem& Run);

	const FSSEventRow* FindEvent(FName EventId) const;
	TArray<FSSEventChoiceView> GetChoices(FName EventId, const USSRunSubsystem& Run) const;

	// 선택지 적용. 선택지가 그 사건 것이 아니거나 조건이 안 맞으면 false
	bool ApplyChoice(FName EventId, FName ChoiceId, USSRunSubsystem& Run, FSSEventResult& OutResult);

	FText DescribeChanges(const FSSEventResult& Result) const;
	void ResetRunState();            // 새 게임 시작 시
	void SetSeed(int32 Seed) { Random.Initialize(Seed); }   // 테스트용

	static bool CheckCondition(ESSEventCondition Condition, FName Target, int32 Amount, const USSRunSubsystem& Run);

private:
	bool IsEligible(FName EventId, const FSSEventRow& Row, const USSRunSubsystem& Run) const;
	bool HasAvailableChoice(FName EventId, const USSRunSubsystem& Run) const;
	void ApplyEffect(const FSSEventEffectRow& Effect, USSRunSubsystem& Run, FSSEventResult& OutResult);

	UPROPERTY()
	TObjectPtr<USSEventCatalog> Catalog;

	// 색인: 사건 → 선택지(Order 순), 선택지 → 효과
	TMap<FName, TArray<FName>> ChoicesByEvent;
	TMap<FName, TArray<FSSEventEffectRow>> EffectsByChoice;

	// 런 상태
	struct FScheduled
	{
		FName EventId;
		int32 Day = 0;
	};
	TArray<FScheduled> Scheduled;
	TSet<FName> FiredOnce;
	TMap<FName, int32> LastFiredDay;

	FRandomStream Random = FRandomStream(FPlatformTime::Cycles());
};
