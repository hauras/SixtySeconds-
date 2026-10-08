#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SSTraceConfig.generated.h"

class UTexture2D;
class UDataTable;

// ─────────────────────────────────────────────
// 외부 통신(역추적) 한 판의 규칙 숫자들
// 에디터에서 DataAsset으로 만들어 숫자를 조절한다
// ─────────────────────────────────────────────
UCLASS(BlueprintType)
class SIXTYSECONDS_API USSTraceConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	// ── 지도 ──

	// 지도 크기. 화면에 그릴 때 기준
	UPROPERTY(EditAnywhere, Category="SS|Trace|Map")
	FVector2D MapSize = FVector2D(600.0, 380.0);

	// 적 센서 위치 (3개 이상, 은신처를 둘러싸게 배치)
	UPROPERTY(EditAnywhere, Category="SS|Trace|Map")
	TArray<FVector2D> SensorPositions;

	// 은신처 위치
	UPROPERTY(EditAnywhere, Category="SS|Trace|Map")
	FVector2D ShelterPosition = FVector2D(360.0, 190.0);

	// 연구소에 원래 있는 통신 단자(중계기) 위치. 대피실 단말에서 원격으로 골라 경유함
	UPROPERTY(EditAnywhere, Category="SS|Trace|Map")
	TArray<FVector2D> RelayPositions;

	// 지도 이미지 (평면도). 없으면 격자만 그림
	UPROPERTY(EditAnywhere, Category="SS|Trace|Map")
	TObjectPtr<UTexture2D> MapImage;

	// ── 센서 ──

	// 직접 송신을 잴 때 센서 오차
	UPROPERTY(EditAnywhere, Category="SS|Trace|Sensor", meta=(ClampMin="0.1"))
	float NoiseSigma = 18.f;

	// 중계기 경유 때 은신처에서 새는 약한 신호의 오차 (클수록 흐릿함)
	UPROPERTY(EditAnywhere, Category="SS|Trace|Sensor", meta=(ClampMin="0.1"))
	float LeakNoiseSigma = 70.f;

	// ── 판 규칙 ──

	// 한 판에 누를 수 있는 송신 횟수
	UPROPERTY(EditAnywhere, Category="SS|Trace|Rules", meta=(ClampMin="1"))
	int32 MaxTurns = 6;

	// 메시지를 다 받으려면 필요한 송신 횟수 (이만큼 받으면 수신 100%)
	UPROPERTY(EditAnywhere, Category="SS|Trace|Rules", meta=(ClampMin="1"))
	int32 ReceiveGoal = 5;

	// 적이 확인한 통신 단자는 며칠 동안 차단됨
	UPROPERTY(EditAnywhere, Category="SS|Trace|Rules", meta=(ClampMin="1"))
	int32 RelayBlockDays = 2;

	// 지도에서 단자를 클릭했다고 보는 거리 (지도 단위)
	UPROPERTY(EditAnywhere, Category="SS|Trace|Rules", meta=(ClampMin="1"))
	float RelayPickRadius = 24.f;

	// 의심 장소 반경이 이보다 작아지면 적이 확인하러 옴
	// 기본 센서 배치·오차 18 기준 직접 송신 반경: 1번 34.6 → 2번 24.5 → 3번 20.0
	// 22면 "직접 2번까지 안전, 3번째에 들킴". 센서 배치나 오차를 바꾸면 이 값도 다시 맞출 것
	UPROPERTY(EditAnywhere, Category="SS|Trace|Rules", meta=(ClampMin="1"))
	float ConfirmRadius = 22.f;

	// ── 경계 ──

	// 경계 수준 최대치
	UPROPERTY(EditAnywhere, Category="SS|Trace|Alert", meta=(ClampMin="0"))
	int32 MaxAlertLevel = 3;

	// 경계 1단계마다 센서 오차에 곱하는 값 (작을수록 경계가 오를 때 센서가 정밀해짐)
	UPROPERTY(EditAnywhere, Category="SS|Trace|Alert", meta=(ClampMin="0.1", ClampMax="1.0"))
	float AlertNoiseScale = 0.8f;

	// ── 받은 메시지 ──

	// 메시지 표 (행: FSSTraceMessageRow, Design/Trace/SS_TraceMessages.csv에서 임포트)
	UPROPERTY(EditAnywhere, Category="SS|Trace|Message")
	TObjectPtr<UDataTable> MessageTable;

	// 몇 번째 메시지마다 진실 단서를 줄지 (2면 2·4·6번째, 0이면 진실은 실용 정보가 떨어졌을 때만)
	UPROPERTY(EditAnywhere, Category="SS|Trace|Message", meta=(ClampMin="0"))
	int32 TruthEvery = 2;

	// 실용 정보는 받은 날부터 며칠 안에 해독해야 하는지 (지나면 사라짐, 진실 단서는 기한 없음)
	UPROPERTY(EditAnywhere, Category="SS|Trace|Message", meta=(ClampMin="1"))
	int32 InfoValidDays = 3;

	// 해독 다이얼 개수 (2 쉬움 ~ 4 어려움). 많을수록 다이얼 하나가 맡는 글자가 줄어 신호가 흔들림
	UPROPERTY(EditAnywhere, Category="SS|Trace|Message", meta=(ClampMin="1", ClampMax="6"))
	int32 DialCount = 3;
};
