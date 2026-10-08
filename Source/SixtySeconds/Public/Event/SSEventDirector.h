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

	// 오늘 나올 사건. 마지막 밤이면 서버실, 그다음 예약된 사건, 없으면 확률·가중치로 뽑음. 없으면 NAME_None
	FName PickEventForToday(const USSRunSubsystem& Run);

	// 마지막 밤 서버실 사건 (엔딩을 정함)
	static FName GetFinalEventId() { return FName(TEXT("ServerRoom")); }

	const FSSEventRow* FindEvent(FName EventId) const;
	TArray<FSSEventChoiceView> GetChoices(FName EventId, const USSRunSubsystem& Run) const;

	// 선택지 적용. 선택지가 그 사건 것이 아니거나 조건이 안 맞으면 false
	bool ApplyChoice(FName EventId, FName ChoiceId, USSRunSubsystem& Run, FSSEventResult& OutResult);

	// 사건 밖(외부 통신 메시지 등)에서 효과 하나만 적용. 카탈로그가 없으면 아이템 효과는 건너뜀
	void ApplyStandaloneEffect(const FSSEventEffectRow& Effect, USSRunSubsystem& Run, FSSEventResult& OutResult);

	FText DescribeChanges(const FSSEventResult& Result) const;
	// 실제 변화 하나를 화면과 기록에서 같은 이름으로 보여준다.
	FText DescribeChange(const FSSEventChange& Change) const;
	void ResetRunState();                                 // 새 게임 시작 시
	void SetSeed(int32 Seed) { Random.Initialize(Seed); } // 테스트용

	static bool CheckCondition(ESSEventCondition Condition, FName Target, int32 Amount, const USSRunSubsystem& Run);

	// 며칠째 밤에 사건을 띄우도록 예약 (아라가 제안·강제 교체를 예약할 때)
	void ScheduleEvent(FName InEventId, int32 Day);

	// 예약된 그 사건을 모두 취소 (아라의 표적이 바뀌었을 때)
	void CancelScheduled(FName InEventId);

	// 그 사건이 며칠째 밤에 예약돼 있는지 (가장 이른 날. 없으면 -1)
	int32 FindScheduledDay(FName InEventId) const;

	// 사건 문장 안의 {Target}을 아라 표적 이름으로 채움 (표적이 없으면 "동료")
	static FFormatNamedArguments MakeTextArgs(const USSRunSubsystem& Run);
	static FText FillText(const FText& Text, const USSRunSubsystem& Run);

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
