#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SSTraceMapWidget.generated.h"

class USSTraceSession;
class USSTraceConfig;

// 지도를 클릭함 (지도 좌표). 창이 듣고 가까운 통신 단자를 고름
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSSOnTraceMapClicked, FVector2D, MapPosition);

// ─────────────────────────────────────────────
// 역추적 지도
// 지도 그림, 센서, 은신처, 통신 단자, 적의 의심 범위를 코드로 직접 그린다 (NativePaint)
// 연출: 센서가 가끔 톡 퍼지며 "듣고 있음", 송신하면 센서에서 거리 원이 퍼짐, 들키면 화면이 붉게 번쩍
// 판단은 하지 않음. 세션을 보고 그리기만 함
// ─────────────────────────────────────────────
UCLASS()
class SIXTYSECONDS_API USSTraceMapWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 그릴 대상 연결 (창이 판을 시작할 때 부름)
	void SetSource(USSTraceSession* InSession, USSTraceConfig* InConfig);

	// 신호가 나간 순간: 신호 위치가 번쩍하고, 센서마다 거리 원이 퍼짐
	//   bFaint : 새어 나간 약한 신호처럼 흐리게
	void PlayBurst(const FVector2D& Source, bool bFaint = false);

	// 들킴 연출
	void PlayExposed();

	// 지도 클릭
	UPROPERTY(BlueprintAssignable, Category="SS|Trace")
	FSSOnTraceMapClicked OnMapClicked;

	// 신호 탐지기가 있나: 없으면 센서 위치와 의심 범위가 안 보이고 퍼지는 원만 흐리게 보임
	UPROPERTY(EditAnywhere, Category="SS|Trace")
	bool bShowEnemyInfo = true;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	// 퍼지는 원 하나
	struct FRing
	{
		FVector2D Center = FVector2D::ZeroVector; // 지도 좌표
		float Target = 0.f;                       // 멈출 반지름 (지도 단위)
		float Radius = 0.f;                       // 지금 반지름
		float Delay = 0.f;                        // 퍼지기 시작할 때까지 남은 시간
		float Life = 1.f;                         // 1 → 0 으로 흐려짐
		FLinearColor Color = FLinearColor::White;
	};

	// 지도 좌표 ↔ 위젯 안 좌표 (지도 비율을 유지한 채 가운데 정렬)
	float MapScale(const FVector2D& LocalSize) const;
	FVector2D MapToLocal(const FVector2D& MapPos, const FVector2D& LocalSize) const;
	FVector2D LocalToMap(const FVector2D& LocalPos, const FVector2D& LocalSize) const;

	UPROPERTY(Transient)
	TObjectPtr<USSTraceSession> Session;

	UPROPERTY(Transient)
	TObjectPtr<USSTraceConfig> Config;

	// Config의 MapImage를 그릴 브러시 (SetSource에서 채움)
	FSlateBrush MapBrush;

	// 연출 상태
	TArray<FRing> Rings;
	TArray<float> IdlePingTimers; // 센서마다 다음 "듣는 중" 퍼짐까지 남은 시간
	FVector2D FlashPosition = FVector2D::ZeroVector;
	float FlashAlpha = 0.f;   // 송신 위치 번쩍임
	float ExposedAlpha = 0.f; // 들킴 화면 번쩍임
	float Time = 0.f;
};
