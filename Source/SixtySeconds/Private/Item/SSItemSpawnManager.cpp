#include "Item/SSItemSpawnManager.h"
#include "Item/SSItemSpawnPoint.h"
#include "Item/SSItemSpawnTypes.h"
#include "Item/SSPickupActor.h"
#include "Item/SSItemDefinition.h"
#include "Character/SSSurvivorPickup.h"
#include "Components/MeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"

ASSItemSpawnManager::ASSItemSpawnManager()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	PickupClass = ASSPickupActor::StaticClass();
}

void ASSItemSpawnManager::BeginPlay()
{
	Super::BeginPlay();

	// 시드가 0이면 판마다 다른 배치 (동료는 시드를 조금 바꿔 아이템과 따로 섞음)
	const int32 Seed = FixedSeed != 0 ? FixedSeed : FMath::Rand();
	SpawnItems(Seed);
	if (bPlaceSurvivors) PlaceSurvivors(Seed + 1);
}

TArray<ASSPickupActor*> ASSItemSpawnManager::SpawnItems(int32 Seed)
{
	TArray<ASSPickupActor*> Spawned;
	UWorld* World = GetWorld();
	if (!World || !IsValid(SpawnTable) || !PickupClass) return Spawned;

	// 1) 맵의 아이템 자리 모으기
	const TArray<ASSItemSpawnPoint*> Points = GatherPoints(ESSSpawnPointKind::Item);

	TArray<FName> Rooms;
	for (const ASSItemSpawnPoint* Point : Points) Rooms.Add(Point->RoomId);

	// 2) 어느 자리에 무엇을 (계산만)
	FRandomStream Random(Seed);
	const TArray<FSSItemPlacement> Plan = FSSItemSpawnPlanner::Plan(Rooms, *SpawnTable, Random);

	// 3) 실제로 놓기: 생성을 미뤄서 아이템을 먼저 정한 뒤 완성 (BeginPlay에서 메시가 정해지도록)
	for (const FSSItemPlacement& Placement : Plan)
	{
		const FTransform PointTransform = Points[Placement.PointIndex]->GetActorTransform();
		ASSPickupActor* Pickup = World->SpawnActorDeferred<ASSPickupActor>(PickupClass, PointTransform, this, nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Pickup) continue;

		FSSItemStack Stack;
		Stack.Item = SpawnTable->Entries[Placement.EntryIndex].Item;
		Stack.Quantity = Placement.Quantity;
		Pickup->SetItemStack(Stack);
		Pickup->FinishSpawning(PointTransform);

		PlaceOnSurface(Pickup, PointTransform);
		Spawned.Add(Pickup);
	}
	return Spawned;
}

TArray<ASSItemSpawnPoint*> ASSItemSpawnManager::GatherPoints(ESSSpawnPointKind Kind) const
{
	TArray<ASSItemSpawnPoint*> Points;
	for (TActorIterator<ASSItemSpawnPoint> It(GetWorld()); It; ++It)
	{
		if (It->Kind == Kind) Points.Add(*It);
	}

	// 이름순으로 정렬해서 같은 시드면 같은 결과
	Points.Sort([](const ASSItemSpawnPoint& A, const ASSItemSpawnPoint& B) { return A.GetName() < B.GetName(); });
	return Points;
}

int32 ASSItemSpawnManager::PlaceSurvivors(int32 Seed)
{
	UWorld* World = GetWorld();
	if (!World) return 0;

	// 맵의 동료들 (이름순)
	TArray<ASSSurvivorPickup*> Survivors;
	for (TActorIterator<ASSSurvivorPickup> It(World); It; ++It) Survivors.Add(*It);
	Survivors.Sort([](const ASSSurvivorPickup& A, const ASSSurvivorPickup& B) { return A.GetName() < B.GetName(); });

	const TArray<ASSItemSpawnPoint*> Points = GatherPoints(ESSSpawnPointKind::Survivor);
	if (Survivors.IsEmpty() || Points.IsEmpty()) return 0;   // 동료 자리가 없으면 맵에 놓인 그대로

	TArray<FName> Rooms;
	for (const ASSItemSpawnPoint* Point : Points) Rooms.Add(Point->RoomId);

	FRandomStream Random(Seed);
	const TArray<int32> Chosen = FSSItemSpawnPlanner::PlanSurvivors(Rooms, Survivors.Num(), Random);

	for (int32 Index = 0; Index < Chosen.Num(); ++Index)
	{
		ASSSurvivorPickup* Survivor = Survivors[Index];
		const FTransform PointTransform = Points[Chosen[Index]]->GetActorTransform();

		// 바닥 찾기: 자리에서 아래로 쏴서 닿는 곳에 세움 (동료 액터의 기준점은 발밑)
		FVector Location = PointTransform.GetLocation();
		FCollisionQueryParams Params(SCENE_QUERY_STAT(SSSurvivorSpawnFloor), false, Survivor);
		FHitResult Hit;
		if (World->LineTraceSingleByChannel(Hit, Location, Location - FVector(0.f, 0.f, SurfaceSearchDistance), ECC_Visibility, Params))
		{
			Location = Hit.ImpactPoint + FVector(0.f, 0.f, 2.f);
		}
		Survivor->SetActorLocationAndRotation(Location, PointTransform.Rotator());
	}
	return Chosen.Num();
}

void ASSItemSpawnManager::PlaceOnSurface(ASSPickupActor* Pickup, const FTransform& PointTransform) const
{
	// 아래로 쏴서 처음 닿는 표면 (자기 자신과 다른 아이템은 무시)
	const FVector Start = PointTransform.GetLocation();
	const FVector End = Start - FVector(0.f, 0.f, SurfaceSearchDistance);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SSItemSpawnSurface), false, Pickup);
	FHitResult Hit;
	if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params)) return;   // 표면이 없으면 자리 그대로

	// 메시의 맨 아래가 표면에 닿도록: 지금 바닥 높이와 표면 높이 차이만큼 내림
	FBox MeshBox(ForceInit);
	TArray<UMeshComponent*> Meshes;
	Pickup->GetComponents(Meshes);
	for (const UMeshComponent* Mesh : Meshes) MeshBox += Mesh->Bounds.GetBox();
	if (!MeshBox.IsValid) return;

	const float Drop = MeshBox.Min.Z - Hit.ImpactPoint.Z;
	Pickup->AddActorWorldOffset(FVector(0.f, 0.f, -Drop));
}
