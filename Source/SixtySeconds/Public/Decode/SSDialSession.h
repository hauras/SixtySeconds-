
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SSDialSession.generated.h"

class USSRunSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSSOnDialChanged);
/**
 * 
 */
UCLASS()
class SIXTYSECONDS_API USSDialSession : public UObject
{
	GENERATED_BODY()
public:
	// 해독 시작. 대기함 번호가 잘못됐으면 false
	bool Initialize(USSRunSubsystem* InRun, int32 InPendingIndex);

	// 다이얼 돌리기 (Step: +1 오른쪽, -1 왼쪽). 잠금이 풀렸으면 아무 일 없음
	void TurnDial(int32 DialIndex, int32 Step);

	// 힌트: 행동력 1을 쓰고 틀린 다이얼 하나를 정답 바로 옆으로. 못 쓰면 false
	bool UseHint();

	// ── 화면이 읽는 값 ──
	int32 GetDialCount() const { return Dials.Num(); }
	int32 GetDial(int32 DialIndex) const;
	FString GetShownText() const;                 // 지금 다이얼로 되돌린 문장
	float GetDialStrength(int32 DialIndex) const; // 이 다이얼의 신호 세기 0~1
	bool IsUnlocked() const { return bUnlocked; }

	// ── 시간제한 (일반 통신만) ──

	// 일반 통신 해독 제한 시간 (초)
	static constexpr float InfoTimeLimit = 40.f;

	// 시간을 흘려보냄 (창이 매 프레임 부름). 0이 되면 실패
	void Tick(float DeltaSeconds);

	// 포기 (창을 닫을 때). 시간제한 있는 해독만 실패 처리, 진실 통신은 진행 저장
	void GiveUp();

	bool HasTimeLimit() const { return bTimed; }
	float GetTimeLeft() const { return TimeLeft; }
	bool IsFailed() const { return bFailed; }

	UPROPERTY(BlueprintAssignable, Category="SS|Decode")
	FSSOnDialChanged OnDialChanged;

private:
	// 다이얼이 전부 정답이면 잠금 해제하고 해독 성공 처리
	void CheckUnlock();

	// 시간 초과·포기: 메시지를 버리고 실패 표시
	void Fail();

	UPROPERTY()
	TObjectPtr<USSRunSubsystem> Run;

	// 대기함에서 몇 번째 메시지인지
	int32 PendingIndex = INDEX_NONE;

	// 암호문 (원문을 정답 열쇠로 암호화한 것)
	FString Cipher;

	// 정답 열쇠, 지금 다이얼 위치
	TArray<int32> Key;
	TArray<int32> Dials;

	// 다이얼마다 26칸 카이제곱 (Initialize에서 한 번 계산) → 신호 세기에 씀
	TArray<TArray<float>> ChiTable;

	bool bUnlocked = false;

	// 시간제한이 있는지 (실용 정보), 남은 시간, 실패했는지
	bool bTimed = false;
	float TimeLeft = 0.f;
	bool bFailed = false;
};
