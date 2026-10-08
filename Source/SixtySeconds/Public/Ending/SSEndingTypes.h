#pragma once

#include "CoreMinimal.h"
#include "SSEndingTypes.generated.h"

// 마지막 밤 서버실에서 정해지는 엔딩
UENUM(BlueprintType)
enum class ESSEnding : uint8
{
	None,       // 아직 안 끝남
	Resolve,    // ① 해결: 아라를 종료하고 격벽이 열림
	Dominion,   // ② 지배: 아무것도 못 바꾸고 계속 "보호"받음
	Reversal,   // ③ 반전: 아라를 믿음 (아라는 정화에서 우리를 지킨 것)
};

// 엔딩 카드에 보여줄 결과 (엔딩이 정해진 순간 고정)
struct FSSEndingReport
{
	ESSEnding Ending = ESSEnding::None;

	// 숨은 진실을 하나라도 알았나 (해결 엔딩에 한 줄 추가)
	bool bKnewTruth = false;

	// 종료하러 데려간 하린이 사실 안드로이드였나 (해결을 골랐지만 지배로 바뀜)
	bool bSabotaged = false;

	// 기록
	int32 DaysSurvived = 0;
	int32 RescuedCount = 0;
	int32 AlarmCount = 0;
};
