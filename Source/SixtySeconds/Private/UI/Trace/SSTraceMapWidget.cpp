#include "UI/Trace/SSTraceMapWidget.h"
#include "Trace/SSTraceSession.h"
#include "Trace/SSTraceConfig.h"
#include "Rendering/DrawElements.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Engine/Texture2D.h"

namespace
{
	// 색 (단말 화면 톤)
	const FLinearColor BackColor   = FLinearColor::FromSRGBColor(FColor(7, 16, 12));
	const FLinearColor GridColor   = FLinearColor::FromSRGBColor(FColor(80, 160, 120, 22));
	const FLinearColor FrameColor  = FLinearColor::FromSRGBColor(FColor(120, 190, 150, 70));
	const FLinearColor SensorColor = FLinearColor::FromSRGBColor(FColor(255, 160, 60));
	const FLinearColor ShelterColor= FLinearColor::FromSRGBColor(FColor(143, 224, 160));
	const FLinearColor RelayColor  = FLinearColor::FromSRGBColor(FColor(235, 192, 133));
	const FLinearColor SpotColor   = FLinearColor::FromSRGBColor(FColor(226, 75, 74));
	const FLinearColor BlockedColor= FLinearColor::FromSRGBColor(FColor(110, 120, 115));

	// 선 긋기
	void DrawLines(FSlateWindowElementList& Out, int32 Layer, const FGeometry& Geo,
		const TArray<FVector2f>& Points, const FLinearColor& Color, float Thickness)
	{
		FSlateDrawElement::MakeLines(Out, Layer, Geo.ToPaintGeometry(), Points, ESlateDrawEffect::None, Color, true, Thickness);
	}

	// 원 긋기 (점 48개를 이은 선)
	void DrawCircle(FSlateWindowElementList& Out, int32 Layer, const FGeometry& Geo,
		const FVector2D& Center, float Radius, const FLinearColor& Color, float Thickness)
	{
		TArray<FVector2f> Points;
		constexpr int32 Segments = 48;
		for (int32 i = 0; i <= Segments; ++i)
		{
			const float A = 2.f * PI * i / Segments;
			Points.Add(FVector2f(float(Center.X + Radius * FMath::Cos(A)), float(Center.Y + Radius * FMath::Sin(A))));
		}
		DrawLines(Out, Layer, Geo, Points, Color, Thickness);
	}

	// 기울어진 타원 긋기 (점 48개를 이은 선)
	//   Radii : X = 긴 반지름, Y = 짧은 반지름 (위젯 단위)
	//   Angle : 긴 반지름이 향하는 각도 (라디안)
	void DrawOval(FSlateWindowElementList& Out, int32 Layer, const FGeometry& Geo,
		const FVector2D& Center, const FVector2D& Radii, float Angle, const FLinearColor& Color, float Thickness)
	{
		const double Cos = FMath::Cos(double(Angle));
		const double Sin = FMath::Sin(double(Angle));

		TArray<FVector2f> Points;
		constexpr int32 Segments = 48;
		for (int32 i = 0; i <= Segments; ++i)
		{
			const double T = 2.0 * PI * i / Segments;

			// 기울기 없는 타원 위의 점
			const double X = Radii.X * FMath::Cos(T);
			const double Y = Radii.Y * FMath::Sin(T);

			// 각도만큼 돌려서 중심으로 옮김
			Points.Add(FVector2f(float(Center.X + X * Cos - Y * Sin), float(Center.Y + X * Sin + Y * Cos)));
		}
		DrawLines(Out, Layer, Geo, Points, Color, Thickness);
	}

	// 점선 (Dash 길이만큼 긋고 같은 길이만큼 띄움)
	void DrawDashedLine(FSlateWindowElementList& Out, int32 Layer, const FGeometry& Geo,
		const FVector2D& From, const FVector2D& To, const FLinearColor& Color, float Dash)
	{
		const double Length = FVector2D::Distance(From, To);
		if (Length <= 0.0 || Dash <= 0.f) return;

		const FVector2D Dir = (To - From) / Length;
		for (double D = 0.0; D < Length; D += Dash * 2.0)
		{
			const FVector2D A = From + Dir * D;
			const FVector2D B = From + Dir * FMath::Min(D + Dash, Length);
			DrawLines(Out, Layer, Geo, { FVector2f(A), FVector2f(B) }, Color, 1.5f);
		}
	}

	// 채운 사각형
	void DrawBox(FSlateWindowElementList& Out, int32 Layer, const FGeometry& Geo,
		const FVector2D& Pos, const FVector2D& Size, const FLinearColor& Color)
	{
		FSlateDrawElement::MakeBox(Out, Layer,
			Geo.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(Pos))),
			FAppStyle::GetBrush("WhiteBrush"), ESlateDrawEffect::None, Color);
	}

	// 지도 표시는 위젯 본문과 같은 한글 명칭을 쓴다.
	void DrawLabel(FSlateWindowElementList& Out, int32 Layer, const FGeometry& Geo,
		const FVector2D& Pos, const FString& Text, const FLinearColor& Color)
	{
		FSlateDrawElement::MakeText(Out, Layer,
			Geo.ToPaintGeometry(FVector2f(160.f, 16.f), FSlateLayoutTransform(FVector2f(Pos))),
			Text, FCoreStyle::GetDefaultFontStyle("Mono", 9), ESlateDrawEffect::None, Color);
	}
}

void USSTraceMapWidget::SetSource(USSTraceSession* InSession, USSTraceConfig* InConfig)
{
	Session = InSession;
	Config = InConfig;
	Rings.Reset();
	ExposedAlpha = 0.f;
	FlashAlpha = 0.f;

	// 지도 그림 (없으면 격자만)
	MapBrush = FSlateBrush();
	if (IsValid(Config) && IsValid(Config->MapImage))
	{
		MapBrush.SetResourceObject(Config->MapImage);
		MapBrush.ImageSize = Config->MapSize;
	}

	// 센서마다 "듣는 중" 퍼짐 시점을 엇갈리게
	IdlePingTimers.Reset();
	if (IsValid(Config))
	{
		for (int32 i = 0; i < Config->SensorPositions.Num(); ++i)
		{
			IdlePingTimers.Add(0.8f + i * 1.1f);
		}
	}
}

void USSTraceMapWidget::PlayBurst(const FVector2D& Source, bool bFaint)
{
	if (!IsValid(Config)) return;

	FlashPosition = Source;
	FlashAlpha = bFaint ? 0.4f : 1.f;

	// 센서마다 신호까지의 거리만큼 원이 퍼짐 (차례로)
	for (int32 i = 0; i < Config->SensorPositions.Num(); ++i)
	{
		FRing& Ring = Rings.AddDefaulted_GetRef();
		Ring.Center = Config->SensorPositions[i];
		Ring.Target = float(FVector2D::Distance(Source, Config->SensorPositions[i]));
		Ring.Delay = 0.12f * i;
		Ring.Color = SensorColor.CopyWithNewOpacity(bFaint ? 0.35f : 0.8f);
	}
}

void USSTraceMapWidget::PlayExposed()
{
	ExposedAlpha = 1.f;
}

void USSTraceMapWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::Visible);   // 빈 위젯이라도 클릭을 받게
}

void USSTraceMapWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Time += InDeltaTime;

	// 퍼지는 원: 목표까지 퍼진 뒤 흐려짐
	for (FRing& Ring : Rings)
	{
		if (Ring.Delay > 0.f)
		{
			Ring.Delay -= InDeltaTime;
			continue;
		}
		if (Ring.Radius < Ring.Target)
		{
			Ring.Radius = FMath::Min(Ring.Target, Ring.Radius + 600.f * InDeltaTime);
		}
		else
		{
			Ring.Life -= 0.6f * InDeltaTime;
		}
	}
	Rings.RemoveAll([](const FRing& Ring) { return Ring.Life <= 0.f; });

	// 센서가 가끔 작게 퍼짐 ("듣고 있음")
	if (IsValid(Config))
	{
		for (int32 i = 0; i < IdlePingTimers.Num() && i < Config->SensorPositions.Num(); ++i)
		{
			IdlePingTimers[i] -= InDeltaTime;
			if (IdlePingTimers[i] <= 0.f)
			{
				IdlePingTimers[i] = 3.2f;
				FRing& Ring = Rings.AddDefaulted_GetRef();
				Ring.Center = Config->SensorPositions[i];
				Ring.Target = 26.f;
				Ring.Color = SensorColor.CopyWithNewOpacity(0.35f);
			}
		}
	}

	FlashAlpha = FMath::Max(0.f, FlashAlpha - 1.5f * InDeltaTime);
	ExposedAlpha = FMath::Max(0.f, ExposedAlpha - 0.6f * InDeltaTime);
}

FReply USSTraceMapWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton || !IsValid(Config))
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}

	const FVector2D Local = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
	OnMapClicked.Broadcast(LocalToMap(Local, InGeometry.GetLocalSize()));
	return FReply::Handled();
}

float USSTraceMapWidget::MapScale(const FVector2D& LocalSize) const
{
	if (!IsValid(Config) || Config->MapSize.X <= 0.0 || Config->MapSize.Y <= 0.0) return 1.f;
	return float(FMath::Min(LocalSize.X / Config->MapSize.X, LocalSize.Y / Config->MapSize.Y));
}

FVector2D USSTraceMapWidget::MapToLocal(const FVector2D& MapPos, const FVector2D& LocalSize) const
{
	const float Scale = MapScale(LocalSize);
	const FVector2D Offset = IsValid(Config) ? (LocalSize - Config->MapSize * Scale) * 0.5 : FVector2D::ZeroVector;
	return Offset + MapPos * Scale;
}

FVector2D USSTraceMapWidget::LocalToMap(const FVector2D& LocalPos, const FVector2D& LocalSize) const
{
	const float Scale = MapScale(LocalSize);
	const FVector2D Offset = IsValid(Config) ? (LocalSize - Config->MapSize * Scale) * 0.5 : FVector2D::ZeroVector;
	return (LocalPos - Offset) / Scale;
}

int32 USSTraceMapWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const int32 Layer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	const FVector2D Size = AllottedGeometry.GetLocalSize();

	// 배경
	DrawBox(OutDrawElements, Layer, AllottedGeometry, FVector2D::ZeroVector, Size, BackColor);
	if (!IsValid(Config)) return Layer + 1;

	const float Scale = MapScale(Size);
	const auto ToLocal = [&](const FVector2D& P) { return MapToLocal(P, Size); };

	const FVector2D TopLeft = ToLocal(FVector2D::ZeroVector);
	const FVector2D BottomRight = ToLocal(Config->MapSize);

	// 지도 그림
	if (MapBrush.GetResourceObject())
	{
		FSlateDrawElement::MakeBox(OutDrawElements, Layer,
			AllottedGeometry.ToPaintGeometry(FVector2f(BottomRight - TopLeft), FSlateLayoutTransform(FVector2f(TopLeft))),
			&MapBrush, ESlateDrawEffect::None, FLinearColor::White);
	}

	// 격자와 테두리
	for (double X = 0.0; X <= Config->MapSize.X; X += 40.0)
	{
		DrawLines(OutDrawElements, Layer + 1, AllottedGeometry,
			{ FVector2f(ToLocal(FVector2D(X, 0.0))), FVector2f(ToLocal(FVector2D(X, Config->MapSize.Y))) }, GridColor, 1.f);
	}
	for (double Y = 0.0; Y <= Config->MapSize.Y; Y += 40.0)
	{
		DrawLines(OutDrawElements, Layer + 1, AllottedGeometry,
			{ FVector2f(ToLocal(FVector2D(0.0, Y))), FVector2f(ToLocal(FVector2D(Config->MapSize.X, Y))) }, GridColor, 1.f);
	}
	DrawLines(OutDrawElements, Layer + 1, AllottedGeometry,
		{ FVector2f(TopLeft), FVector2f(float(BottomRight.X), float(TopLeft.Y)), FVector2f(BottomRight), FVector2f(float(TopLeft.X), float(BottomRight.Y)), FVector2f(TopLeft) },
		FrameColor, 2.f);

	// 퍼지는 원들 (탐지기가 없으면 흐리게)
	const float RingOpacity = bShowEnemyInfo ? 1.f : 0.4f;
	for (const FRing& Ring : Rings)
	{
		if (Ring.Delay > 0.f || Ring.Radius <= 0.f) continue;
		DrawCircle(OutDrawElements, Layer + 2, AllottedGeometry, ToLocal(Ring.Center), Ring.Radius * Scale,
			Ring.Color.CopyWithNewOpacity(Ring.Color.A * Ring.Life * RingOpacity), 2.f);
	}

	// 적의 의심 범위(타원): 확신할수록 빠르고 밝게 깜빡임
	bool bShelterInRange = false;   // 은신처가 어떤 타원 안에 들어갔나 (위험 표시용)
	if (bShowEnemyInfo && IsValid(Session))
	{
		for (const FSSSuspectSpot& Spot : Session->GetSpots())
		{
			const float Confidence = FMath::Clamp(Config->ConfirmRadius / FMath::Max(Spot.Radius, 1.f), 0.f, 1.f);
			const float Pulse = 0.5f + 0.5f * FMath::Sin(Time * (2.f + 8.f * Confidence));
			const FLinearColor Color = SpotColor.CopyWithNewOpacity(0.35f + 0.65f * Pulse * Confidence);
			const FVector2D Center = ToLocal(Spot.Position);
			DrawOval(OutDrawElements, Layer + 3, AllottedGeometry, Center, Spot.OvalSize * Scale, Spot.OvalAngle, Color, 2.f);

			// 적이 생각하는 위치 (작은 +)
			DrawLines(OutDrawElements, Layer + 3, AllottedGeometry, { FVector2f(Center + FVector2D(-4.0, 0.0)), FVector2f(Center + FVector2D(4.0, 0.0)) }, Color, 1.5f);
			DrawLines(OutDrawElements, Layer + 3, AllottedGeometry, { FVector2f(Center + FVector2D(0.0, -4.0)), FVector2f(Center + FVector2D(0.0, 4.0)) }, Color, 1.5f);

			if (FSSSuspectMemory::IsInsideOval(Spot, Config->ShelterPosition)) bShelterInRange = true;

			DrawLabel(OutDrawElements, Layer + 3, AllottedGeometry, Center + FVector2D(Spot.Radius * Scale + 4.f, -6.f),
				FString::Printf(TEXT("확신 %d%%"), FMath::RoundToInt(Confidence * 100.f)), SpotColor);
		}
	}

	// 센서 (탐지기가 있을 때만 위치가 보임)
	if (bShowEnemyInfo)
	{
		for (int32 i = 0; i < Config->SensorPositions.Num(); ++i)
		{
			const FVector2D P = ToLocal(Config->SensorPositions[i]);
			DrawBox(OutDrawElements, Layer + 4, AllottedGeometry, P - FVector2D(6.0, 6.0), FVector2D(12.0, 12.0), SensorColor);
			DrawLabel(OutDrawElements, Layer + 4, AllottedGeometry, P + FVector2D(10.0, -7.0), FString::Printf(TEXT("센서 %d"), i + 1), SensorColor);
		}
	}

	// 은신처
	const FVector2D Shelter = ToLocal(Config->ShelterPosition);
	// 의심 범위 안에 들어가면 붉게 + "위험" (적이 확신하는 순간 들킴)
	const FLinearColor ShelterNow = bShelterInRange ? SpotColor : ShelterColor;
	DrawBox(OutDrawElements, Layer + 4, AllottedGeometry, Shelter - FVector2D(6.0, 6.0), FVector2D(12.0, 12.0), ShelterNow);
	DrawLabel(OutDrawElements, Layer + 4, AllottedGeometry, Shelter + FVector2D(10.0, 4.0),
		bShelterInRange ? TEXT("은신처 · 위험") : TEXT("은신처"), ShelterNow);

	// 통신 단자 (삼각형): 쓸 수 있음 = 흐린 호박색, 고름 = 밝게 + 은신처에서 점선, 차단 = 회색 + X
	for (int32 i = 0; i < Config->RelayPositions.Num(); ++i)
	{
		const bool bBlocked = IsValid(Session) && Session->IsRelayBlocked(i);
		const bool bSelected = IsValid(Session) && Session->GetSelectedRelay() == i;
		const FVector2D R = ToLocal(Config->RelayPositions[i]);

		FLinearColor Color = RelayColor.CopyWithNewOpacity(0.55f);
		if (bBlocked) Color = BlockedColor;
		if (bSelected) Color = RelayColor;

		// 고른 단자: 은신처에서 원격으로 잇는 점선
		if (bSelected)
		{
			DrawDashedLine(OutDrawElements, Layer + 3, AllottedGeometry, Shelter, R, RelayColor.CopyWithNewOpacity(0.6f), 5.f);
		}

		DrawLines(OutDrawElements, Layer + 4, AllottedGeometry,
			{ FVector2f(R + FVector2D(0.0, -10.0)), FVector2f(R + FVector2D(9.0, 7.0)), FVector2f(R + FVector2D(-9.0, 7.0)), FVector2f(R + FVector2D(0.0, -10.0)) },
			Color, bSelected ? 2.5f : 1.5f);

		// 차단된 단자: X 표시
		if (bBlocked)
		{
			DrawLines(OutDrawElements, Layer + 4, AllottedGeometry, { FVector2f(R + FVector2D(-7.0, -7.0)), FVector2f(R + FVector2D(7.0, 7.0)) }, Color, 2.f);
			DrawLines(OutDrawElements, Layer + 4, AllottedGeometry, { FVector2f(R + FVector2D(7.0, -7.0)), FVector2f(R + FVector2D(-7.0, 7.0)) }, Color, 2.f);
		}

		const TCHAR* State = bBlocked ? TEXT(" 차단") : (bSelected ? TEXT(" 선택") : TEXT(""));
		DrawLabel(OutDrawElements, Layer + 4, AllottedGeometry, R + FVector2D(12.0, -6.0),
			FString::Printf(TEXT("단자 %d%s"), i + 1, State), Color);
	}

	// 송신 위치 번쩍임
	if (FlashAlpha > 0.f)
	{
		DrawCircle(OutDrawElements, Layer + 5, AllottedGeometry, ToLocal(FlashPosition), 14.f + 10.f * (1.f - FlashAlpha),
			FLinearColor::White.CopyWithNewOpacity(FlashAlpha), 3.f);
	}

	// 들킴: 화면 전체가 붉게 번쩍
	if (ExposedAlpha > 0.f)
	{
		DrawBox(OutDrawElements, Layer + 6, AllottedGeometry, FVector2D::ZeroVector, Size, SpotColor.CopyWithNewOpacity(0.35f * ExposedAlpha));
	}

	return Layer + 6;
}
