// Fill out your copyright notice in the Description page of Project Settings.


#include "GLCharacter.h"
#include "GLPlayerController.h"
#include "EnhancedInputComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Actor.h"
#include "GLGamceState.h"
#include "Kismet/GameplayStatics.h"
#include "Components/WidgetComponent.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "GLOverheadHPWidget.h"
// Sets default values
AGLCharacter::AGLCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

    // (1) 스프링 암 생성
    SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    // 스프링 암을 루트 컴포넌트 (CapsuleComponent)에 부착
    SpringArmComp->SetupAttachment(RootComponent);
    // 캐릭터와 카메라 사이의 거리 기본값 300으로 설정
    SpringArmComp->TargetArmLength = 300.0f;
    // 컨트롤러 회전에 따라 스프링 암도 회전하도록 설정
    SpringArmComp->bUsePawnControlRotation = true;

    // (2) 카메라 컴포넌트 생성
    CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    // 스프링 암의 소켓 위치에 카메라를 부착
    CameraComp->SetupAttachment(SpringArmComp, USpringArmComponent::SocketName);
    // 카메라는 스프링 암의 회전을 따르므로 PawnControlRotation은 꺼둠
    CameraComp->bUsePawnControlRotation = false;
    OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
    OverheadWidget->SetupAttachment(GetMesh());
    OverheadWidget->SetWidgetSpace(EWidgetSpace::Screen);
    NormalSpeed = 600.0f;
    SprintSpeedMultiplier = 1.5f;
    SprintSpeed = NormalSpeed * SprintSpeedMultiplier;

    GetCharacterMovement()->MaxWalkSpeed = NormalSpeed;

    // 초기 체력 설정
    MaxHealth = 100.0f;
    Health = MaxHealth;

}

// Called when the game starts or when spawned
void AGLCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AGLCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

    if (!bDead && GetActorLocation().Z < DeathZ)
    {
        bDead = true;
        Health = 0.f;
        OnDeath();
    }
}

// Called to bind functionality to input
void AGLCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
    // Enhanced InputComponent로 캐스팅
    if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        // IA를 가져오기 위해 현재 소유 중인 Controller를 ASpartaPlayerController로 캐스팅
        if (AGLPlayerController* PlayerController = Cast<AGLPlayerController>(GetController()))
        {
            if (PlayerController->MoveAction)
            {
                // IA_Move 액션 키를 "키를 누르고 있는 동안" Move() 호출
                EnhancedInput->BindAction(
                    PlayerController->MoveAction,
                    ETriggerEvent::Triggered,
                    this,
                    &AGLCharacter::Move
                );
            }

            if (PlayerController->JumpAction)
            {
                // IA_Jump 액션 키를 "키를 누르고 있는 동안" StartJump() 호출
                EnhancedInput->BindAction(
                    PlayerController->JumpAction,
                    ETriggerEvent::Triggered,
                    this,
                    &AGLCharacter::StartJump
                );

                // IA_Jump 액션 키에서 "손을 뗀 순간" StopJump() 호출
                EnhancedInput->BindAction(
                    PlayerController->JumpAction,
                    ETriggerEvent::Completed,
                    this,
                    &AGLCharacter::StopJump
                );
            }

            if (PlayerController->LookAction)
            {
                // IA_Look 액션 마우스가 "움직일 때" Look() 호출
                EnhancedInput->BindAction(
                    PlayerController->LookAction,
                    ETriggerEvent::Triggered,
                    this,
                    &AGLCharacter::Look
                );
            }

            if (PlayerController->SprintAction)
            {
                // IA_Sprint 액션 키를 "누르고 있는 동안" StartSprint() 호출
                EnhancedInput->BindAction(
                    PlayerController->SprintAction,
                    ETriggerEvent::Triggered,
                    this,
                    &AGLCharacter::StartSprint
                );
                // IA_Sprint 액션 키에서 "손을 뗀 순간" StopSprint() 호출
                EnhancedInput->BindAction(
                    PlayerController->SprintAction,
                    ETriggerEvent::Completed,
                    this,
                    &AGLCharacter::StopSprint
                );
            }
        }
    }
}

void AGLCharacter::Move(const FInputActionValue& value)
{
    // 컨트롤러가 있어야 방향 계산이 가능
    if (!Controller) return;

    // Value는 Axis2D로 설정된 IA_Move의 입력값 (WASD)을 담고 있음
// 예) (X=1, Y=0) → 전진 / (X=-1, Y=0) → 후진 / (X=0, Y=1) → 오른쪽 / (X=0, Y=-1) → 왼쪽
    const FVector2D MoveInput = value.Get<FVector2D>();

    if (!FMath::IsNearlyZero(MoveInput.X))
    {
        // 캐릭터가 바라보는 방향(정면)으로 X축 이동
        AddMovementInput(GetActorForwardVector(), MoveInput.X);
    }

    if (!FMath::IsNearlyZero(MoveInput.Y))
    {
        // 캐릭터의 오른쪽 방향으로 Y축 이동
        AddMovementInput(GetActorRightVector(), MoveInput.Y);
    }
}

void AGLCharacter::StartJump(const FInputActionValue& value)
{
    // Jump 함수는 Character가 기본 제공
    if (value.Get<bool>())
    {
        Jump();
    }
}

void AGLCharacter::StopJump(const FInputActionValue& value)
{
    // StopJumping 함수도 Character가 기본 제공
    if (!value.Get<bool>())
    {
        StopJumping();
    }
}

void AGLCharacter::Look(const FInputActionValue& value)
{
    // 마우스의 X, Y 움직임을 2D 축으로 가져옴
    FVector2D LookInput = value.Get<FVector2D>();

    // X는 좌우 회전 (Yaw), Y는 상하 회전 (Pitch)
    // 좌우 회전
    AddControllerYawInput(LookInput.X);
    // 상하 회전
    AddControllerPitchInput(LookInput.Y);
}

void AGLCharacter::StartSprint(const FInputActionValue& value)
{
    // Shift 키를 누른 순간 이 함수가 호출된다고 가정
// 스프린트 속도를 적용
    if (GetCharacterMovement())
    {
        GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
    }
}

void AGLCharacter::StopSprint(const FInputActionValue& value)
{
    // Shift 키를 뗀 순간 이 함수가 호출
// 평상시 속도로 복귀
    if (GetCharacterMovement())
    {
        GetCharacterMovement()->MaxWalkSpeed = NormalSpeed;
    }
}

// 체력 회복 함수
void AGLCharacter::AddHealth(float Amount)
{
    // 체력을 회복시킴. 최대 체력을 초과하지 않도록 제한함
    Health = FMath::Clamp(Health + Amount, 0.0f, MaxHealth);
    UpdateOverheadHP(false);
    UE_LOG(LogTemp, Log, TEXT("Health increased to: %f"), Health);
}

int32 AGLCharacter::GetHealth() const
{
    return Health;
};

// 데미지 처리 함수
float AGLCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    // 기본 데미지 처리 로직 호출 (필수는 아님)
    float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

    // 체력을 데미지만큼 감소시키고, 0 이하로 떨어지지 않도록 Clamp
    Health = FMath::Clamp(Health - DamageAmount, 0.0f, MaxHealth);
    UpdateOverheadHP(true);
    UE_LOG(LogTemp, Warning, TEXT("Health decreased to: %f"), Health);

    // 체력이 0 이하가 되면 사망 처리
    if (Health <= 0.0f)
    {
        OnDeath();
    }

    // 실제 적용된 데미지를 반환
    return ActualDamage;
}

// 사망 처리 함수
void AGLCharacter::OnDeath()
{
    // GameOver 호출
    if (UWorld* World = GetWorld())
    {
        if (AGLGamceState* GS = World->GetGameState<AGLGamceState>())
        {
            GS->OnGameOver();
        }
    }

    // (더 이상 입력/이동 못 하게
    if (AController* C = GetController())
    {
        C->UnPossess();
    }
}


void AGLCharacter::FellOutOfWorld(const UDamageType& dmgType)
{
    // 이미 죽은 상태면 중복 방지(원하면 bIsDead 같은 플래그 추가)
    // 사망 처리로 통일
    Health = 0.f;
    UpdateOverheadHP(false);
    OnDeath();
}

void AGLCharacter::UpdateOverheadHP(bool bHitFlash)
{
    if (!OverheadWidget) return;

    UUserWidget* W = OverheadWidget->GetUserWidgetObject();
    if (!W) return;

    if (UGLOverheadHPWidget* HPW = Cast<UGLOverheadHPWidget>(W))
    {
        HPW->SetHP(Health, MaxHealth);
        if (bHitFlash)
        {
            HPW->PlayHitFlash();
        }
    }
}