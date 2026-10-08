
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Character/SSSurvivorTypes.h"
#include "Companion/SSInvestigation.h"
#include "SSSurvivorDefinition.generated.h"

class UTexture2D;

/**
 * 
 */
UCLASS(BlueprintType)
class SIXTYSECONDS_API USSSurvivorDefinition : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SS|Survivor")
	FName SurvivorId;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SS|Survivor")
	FText DisplayName;

	// 정보창에서 보여줄 초상화
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SS|Survivor")
	TObjectPtr<UTexture2D> Portrait;

	// 은신처 화면에 배치할 인물 이미지
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SS|Survivor")
	TObjectPtr<UTexture2D> ShelterImage;

	// 합류 시 사용할 기본 스탯
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SS|Survivor")
	FSSSurvivorStats InitialStats;

	// ── 조사 성향 (거의 고정된 내부 값. 플레이어에게 숫자로 보여주지 않음, 임시값) ──

	// 꼼꼼함 0~1: 단서를 찾는 능력
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SS|Survivor|Investigation", meta=(ClampMin="0", ClampMax="1"))
	float Thoroughness = 0.5f;

	// 대담함 0~1: 위험한 장소를 덜 피하고 더 깊이 뒤짐 (단서↑, 아라에게 들킬 위험↑)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SS|Survivor|Investigation", meta=(ClampMin="0", ClampMax="1"))
	float Boldness = 0.5f;

	// 장소별 선호 (클수록 자주 고름, 정하지 않은 장소는 0). 예: 연구원은 단말 로그 3
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SS|Survivor|Investigation")
	TMap<ESSInvestigationSpot, float> SpotPreference;

	// 아라에게 얼마나 위협적인가 (위협도 = 의심 × 이 값). 연구원은 아라를 잘 알아서 높음 (임시값)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SS|Survivor|Investigation", meta=(ClampMin="0"))
	float AraInfluence = 1.f;
};
