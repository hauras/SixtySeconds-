#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Math/RandomStream.h"
#include "Event/SSEventTypes.h"
#include "SSTraceMessage.generated.h"

// 외부 통신으로 받은 메시지의 종류
UENUM(BlueprintType)
enum class ESSTraceMessageKind : uint8
{
	Info,  // 실용 정보: 조건이 맞는 것 중 무작위, 효과가 바로 쓸모 있음
	Truth, // 진실 단서: Order 순서대로 하나씩, 아라가 말하지 않는 바깥 사정
};

// ─────────────────────────────────────────────
// TraceMessages 시트 한 줄. 행 이름(Name 열)이 MessageId
// 효과는 사건 효과(ESSEventEffect)를 한 개 그대로 씀
// ─────────────────────────────────────────────
USTRUCT(BlueprintType)
struct SIXTYSECONDS_API FSSTraceMessageRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Trace")
	ESSTraceMessageKind Kind = ESSTraceMessageKind::Info;

	// 진실 단서의 순서 (작은 것부터). 실용 정보는 안 씀
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Trace")
	int32 Order = 0;

	// 이 날부터 나올 수 있음
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Trace")
	int32 MinDay = 1;

	// 실용 정보끼리 뽑힐 확률 가중치
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Trace")
	int32 Weight = 10;

	// 짧은 제목 (예: "순찰 교대 기록")
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Trace")
	FText Title;

	// 한글 해석 (해독에 성공하면 영어 원문 아래에 보여줌)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Trace")
	FText Text;

	// 영어 원문 (로봇 기계 통신). 이 문장을 암호로 바꿔서 해독 화면에 보여줌
	// 대문자·띄어쓰기·마침표만 사용, 숫자는 영어로 풀어 씀 (빈도 분석이 잘 먹히게)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Trace")
	FString CipherSource;

	// 받으면 일어나는 일 (없으면 None)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Trace")
	ESSEventEffect EffectType = ESSEventEffect::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Trace")
	FName EffectTarget = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SS|Trace")
	int32 EffectAmount = 0;
};

USTRUCT()
struct SIXTYSECONDS_API FSSPendingMessage
{
	GENERATED_BODY()

	UPROPERTY()
	FName MessageId = NAME_None;

	UPROPERTY()
	FSSTraceMessageRow Row;

	UPROPERTY()
	int32 ExpireDay = 0;

	// 정답 다이얼 값 (메시지를 받을 때 무작위로 정함, 1~25)
	UPROPERTY()
	TArray<int32> Key;

	// 플레이어가 지금 맞춰둔 다이얼 위치 (처음엔 전부 0, 창을 닫아도 남음)
	UPROPERTY()
	TArray<int32> Dials;

	// ── 진실 통신 (치환 암호) ──

	// 치환표: SubKey[원래 글자] = 암호 글자 (진실 단서만, 26칸)
	UPROPERTY()
	TArray<int32> SubKey;

	// 지금 추측표: Guess[암호 글자] = 원래 글자, 모르면 INDEX_NONE (창을 닫아도 남음)
	UPROPERTY()
	TArray<int32> Guess;

	// 자동 해독기를 이미 돌렸는지 (다시 열면 연출 없이 빈칸 채우기부터)
	UPROPERTY()
	bool bSolverDone = false;
};

// ─────────────────────────────────────────────
// 다음에 받을 메시지 고르기
// 부모 없음. 받은 기록은 부르는 쪽(RunSubsystem)이 들고 있음
// ─────────────────────────────────────────────
class SIXTYSECONDS_API FSSTraceMessagePicker
{
public:
	// 다음 메시지 하나를 고름. 고를 게 없으면 NAME_None
	//   Table          : 메시지 표 (행: FSSTraceMessageRow)
	//   Day            : 오늘 날짜 (MinDay 확인)
	//   Received       : 이미 받은 메시지들 (다시 안 나옴)
	//   CompletedCount : 이번 메시지까지 포함한 완료 횟수 (1부터)
	//   TruthEvery     : 몇 번째마다 진실 단서를 줄지 (2면 2·4·6번째)
	//   Random         : 실용 정보를 뽑을 난수
	// 규칙: 진실 차례면 아직 안 받은 진실 중 Order가 가장 작은 것.
	//       진실 차례가 아니거나 진실이 없으면 실용 정보를 가중치로 뽑고, 실용 정보도 없으면 남은 진실
	static FName Pick(
		const UDataTable* Table,
		int32 Day,
		const TSet<FName>& Received,
		int32 CompletedCount,
		int32 TruthEvery,
		FRandomStream& Random);

private:
	static FName PickTruth(const UDataTable* Table, int32 Day, const TSet<FName>& Received);
	static FName PickInfo(const UDataTable* Table, int32 Day, const TSet<FName>& Received, FRandomStream& Random);
};
