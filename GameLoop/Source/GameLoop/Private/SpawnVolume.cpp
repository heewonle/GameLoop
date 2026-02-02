#include "SpawnVolume.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"

ASpawnVolume::ASpawnVolume()
{
    PrimaryActorTick.bCanEverTick = false;

    Scene = CreateDefaultSubobject<USceneComponent>(TEXT("Scene"));
    SetRootComponent(Scene);

    SpawningBox = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawningBox"));
    SpawningBox->SetupAttachment(Scene);

    ItemDataTable = nullptr;
}

AActor* ASpawnVolume::SpawnRandomItem()
{
    if (FItemSpawnRow* SelectedRow = GetRandomItem())
    {
        if (UClass* ActualClass = SelectedRow->ItemClass.Get())
        {
            // 여기서 SpawnItem()을 호출하고, 스폰된 AActor 포인터를 리턴
            return SpawnItem(ActualClass);
        }
    }

    return nullptr;
}

FVector ASpawnVolume::GetRandomPointInVolume() const
{
    const FVector BoxExtent = SpawningBox->GetScaledBoxExtent();
    const FVector BoxOrigin = SpawningBox->GetComponentLocation();

    // 1) 박스 안 랜덤 위치 (XY 기준)
    FVector P = BoxOrigin + FVector(
        FMath::FRandRange(-BoxExtent.X, BoxExtent.X),
        FMath::FRandRange(-BoxExtent.Y, BoxExtent.Y),
        FMath::FRandRange(-BoxExtent.Z, BoxExtent.Z)
    );

    UWorld* World = GetWorld();
    if (!World) return P;

    const float TraceUp = 100000.f;
    const float TraceDown = 200000.f;

    const float SphereRadius = 40.f; // 아이템 크기에 맞게 조절

    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(SpawnGroundSweep), false);
    Params.AddIgnoredActor(this);

    const FVector Start = FVector(P.X, P.Y, P.Z + TraceUp);
    const FVector End = FVector(P.X, P.Y, P.Z - TraceDown);

    const FCollisionShape Sphere = FCollisionShape::MakeSphere(SphereRadius);

    const bool bHit = World->SweepSingleByChannel(
        Hit,
        Start,
        End,
        FQuat::Identity,
        ECC_Visibility,
        Sphere,
        Params
    );

    // 3) 맞으면 → 구 중심 위치를 그대로 사용
    if (bHit)
    {
        P = Hit.Location; // ⭐ 이미 바닥 위에 얹힌 위치
    }

    return P;
}

FItemSpawnRow* ASpawnVolume::GetRandomItem() const
{
    if (!ItemDataTable) return nullptr;

    // 1) 모든 Row(행) 가져오기
    TArray<FItemSpawnRow*> AllRows;
    static const FString ContextString(TEXT("ItemSpawnContext"));
    ItemDataTable->GetAllRows(ContextString, AllRows);

    if (AllRows.IsEmpty()) return nullptr;

    // 2) 전체 확률 합 구하기
    float TotalChance = 0.0f; // 초기화
    for (const FItemSpawnRow* Row : AllRows) // AllRows 배열의 각 Row를 순회
    {
        if (Row) // Row가 유효한지 확인
        {
            TotalChance += Row->SpawnChance; // SpawnChance 값을 TotalChance에 더하기
        }
    }

    // 3) 0 ~ TotalChance 사이 랜덤 값
    const float RandValue = FMath::FRandRange(0.0f, TotalChance);
    float AccumulateChance = 0.0f;

    // 4) 누적 확률로 아이템 선택
    for (FItemSpawnRow* Row : AllRows)
    {
        AccumulateChance += Row->SpawnChance;
        if (RandValue <= AccumulateChance)
        {
            return Row;
        }
    }

    return nullptr;
}

AActor* ASpawnVolume::SpawnItem(TSubclassOf<AActor> ItemClass)
{
    if (!ItemClass) return nullptr;

    // SpawnActor가 성공하면 스폰된 액터의 포인터가 반환됨
    AActor* SpawnedActor = GetWorld()->SpawnActor<AActor>(
        ItemClass,
        GetRandomPointInVolume(),
        FRotator::ZeroRotator
    );

    return SpawnedActor;
}

void ASpawnVolume::BeginPlay() {
    UE_LOG(LogTemp, Warning, TEXT("[Actor] %s BeginPlay"), *GetName());
}