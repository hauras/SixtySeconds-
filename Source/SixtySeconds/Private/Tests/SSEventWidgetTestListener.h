#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SSEventWidgetTestListener.generated.h"

// 사건 종료 알림 횟수만 세는 테스트용 객체다.
UCLASS()
class USSEventWidgetTestListener : public UObject
{
	GENERATED_BODY()
public:
	UFUNCTION()
	void HandleFinished() { ++FinishedCount; }

	int32 FinishedCount = 0;
};
