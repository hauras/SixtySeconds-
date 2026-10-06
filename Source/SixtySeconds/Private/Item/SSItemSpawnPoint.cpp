#include "Item/SSItemSpawnPoint.h"
#include "Components/ArrowComponent.h"
#include "Components/BillboardComponent.h"
#include "Components/SceneComponent.h"

ASSItemSpawnPoint::ASSItemSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// 둘 다 에디터 전용 표시 (게임에선 숨김)
	Icon = CreateDefaultSubobject<UBillboardComponent>(TEXT("Icon"));
	Icon->SetupAttachment(RootComponent);
	Icon->SetHiddenInGame(true);

	Arrow = CreateDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
	Arrow->SetupAttachment(RootComponent);
	Arrow->SetHiddenInGame(true);
	Arrow->ArrowSize = 0.5f;
}
