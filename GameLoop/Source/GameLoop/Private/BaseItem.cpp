// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseItem.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"

// Sets default values
ABaseItem::ABaseItem()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
    // 루트 컴포넌트 생성 및 설정
    Scene = CreateDefaultSubobject<USceneComponent>(TEXT("Scene"));
    SetRootComponent(Scene);

    // 충돌 컴포넌트 생성 및 설정
    Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
    // 겹침만 감지하는 프로파일 설정
    Collision->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    // 루트 컴포넌트로 설정
    Collision->SetupAttachment(Scene);

    // 스태틱 메시 컴포넌트 생성 및 설정
    StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
    StaticMesh->SetupAttachment(Collision);
    // 메시가 불필요하게 충돌을 막지 않도록 하기 위해, 별도로 NoCollision 등으로 설정할 수 있음.

// Overlap 이벤트 바인딩
    Collision->OnComponentBeginOverlap.AddDynamic(this, &ABaseItem::OnItemOverlap);
    Collision->OnComponentEndOverlap.AddDynamic(this, &ABaseItem::OnItemEndOverlap);

}
// 플레이어가 아이템 범위에 들어왔을 때 동작
void ABaseItem::OnItemOverlap(
    UPrimitiveComponent* OverlappedComp,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{
    // OtherActor가 플레이어인지 확인 ("Player" 태그 활용)
    if (OtherActor && OtherActor->ActorHasTag("Player"))
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, FString::Printf(TEXT("Overlap!!!")));
        // 아이템 사용 (획득) 로직 호출
        ActivateItem(OtherActor);
    }
}

// 플레이어가 아이템 범위를 벗어났을 때 동작
void ABaseItem::OnItemEndOverlap(
    UPrimitiveComponent* OverlappedComp,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex)
{
}

// 아이템이 사용(Activate)되었을 때 동작

void ABaseItem::ActivateItem(AActor* Activator)
{
    UParticleSystemComponent* PSC = nullptr;

    if (PickupParticle)
    {
        PSC = UGameplayStatics::SpawnEmitterAtLocation(
            GetWorld(),
            PickupParticle,
            GetActorLocation(),
            GetActorRotation(),
            true
        );

        // SetAutoDestroy가 없으므로, bAutoDestroy 플래그를 직접 설정해야 합니다.
        if (PSC)
        {
            PSC->OnSystemFinished.Clear();
            PSC->OnSystemFinished.AddDynamic(this, &ABaseItem::OnPickupFxFinished);

            // 2초 후 강제로 종료
            FTimerHandle LocalHandle;
            GetWorld()->GetTimerManager().SetTimer(
                LocalHandle,
                [WeakPSC = TWeakObjectPtr<UParticleSystemComponent>(PSC)]()
                {
                    if (WeakPSC.IsValid())
                    {
                        WeakPSC->DeactivateSystem();
                    }
                },
                2.0f,
                false
            );
        }
    }

    if (PickupSound)
    {
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), PickupSound, GetActorLocation());
    }
}

void ABaseItem::CleanupPickupParticle(UParticleSystemComponent* PSC)
{
    if (IsValid(PSC))
    {
        PSC->DeactivateSystem();     // 일단 끄고
        PSC->DestroyComponent();     // 제거
    }
}

void ABaseItem::OnPickupFxFinished(UParticleSystemComponent* FinishedPSC)
{
    if (IsValid(FinishedPSC))
    {
        FinishedPSC->DestroyComponent();
    }
}
void ABaseItem::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearAllTimersForObject(this);
    }
    Super::EndPlay(EndPlayReason);
}

// 아이템 유형을 반환
FName ABaseItem::GetItemType() const
{
	return ItemType;
}

// 아이템을 파괴(제거)하는 함수
void ABaseItem::DestroyItem()
{
	// AActor에서 제공하는 Destroy() 함수로 객체 제거
	Destroy();
}


// Called when the game starts or when spawned
void ABaseItem::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ABaseItem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

