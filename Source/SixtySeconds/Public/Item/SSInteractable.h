#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SSInteractable.generated.h"

UINTERFACE(MinimalAPI)
class USSInteractable : public UInterface
{
	GENERATED_BODY()
};

// ─────────────────────────────────────────────
// 스크램블에서 E키로 상호작용하는 것 (아이템 줍기, 동료 데려가기)
// 캐릭터는 종류를 몰라도 이 셋만 부름 → E키와 화면 안내가 늘 같은 대상·같은 문장을 씀
// ─────────────────────────────────────────────
class SIXTYSECONDS_API ISSInteractable
{
	GENERATED_BODY()

public:
	// 지금 이 사람이 상호작용할 수 있나 (가방이 꽉 찼으면 false 등)
	virtual bool CanInteract(const APawn* Interactor) const = 0;

	// 화면에 띄울 안내 ("식량 · [E] 줍기", 못 하면 그 이유)
	virtual FText GetInteractPrompt(const APawn* Interactor) const = 0;

	// 상호작용 실행. 성공하면 true
	virtual bool TryInteract(APawn* Interactor) = 0;
};
