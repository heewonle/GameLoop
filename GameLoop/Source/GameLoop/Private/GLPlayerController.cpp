// Fill out your copyright notice in the Description page of Project Settings.


#include "GLPlayerController.h"
#include "GLGamceState.h"
#include "GLGameInstance.h"
#include "EnhancedInputSubsystems.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Framework/Application/SlateApplication.h"
#include "Components/Button.h"
#include "Kismet/KismetSystemLibrary.h"


AGLPlayerController::AGLPlayerController()
    : InputMappingContext(nullptr),
    MoveAction(nullptr),
    JumpAction(nullptr),
    LookAction(nullptr),
    SprintAction(nullptr),
    HUDWidgetClass(nullptr),
    HUDWidgetInstance(nullptr),
    MainMenuWidgetClass(nullptr),
    MainMenuWidgetInstance(nullptr)
{
}

void AGLPlayerController::BeginPlay()
{
    Super::BeginPlay();

    // 현재 PlayerController에 연결된 Local Player 객체를 가져옴    
    if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
    {

        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
        {

            if (InputMappingContext)
            {
                Subsystem->AddMappingContext(InputMappingContext, 0);
            }
        }
    }

    const FString CurrentMapName = GetWorld() ? GetWorld()->GetMapName() : FString("NO_WORLD");

    if (CurrentMapName.Contains("L_Menu"))
    {
        ShowMainMenu(false);
                return;
    }

    AGLGamceState* GLGameState = GetWorld() ? GetWorld()->GetGameState<AGLGamceState>() : nullptr;

    if (GLGameState)
    {
        GLGameState->UpdateHUD();
    }
}

UUserWidget* AGLPlayerController::GetHUDWidget() const
{
    if (!IsValid(HUDWidgetInstance)) return nullptr;

    // ★ 월드가 다르면 무조건 버린다 (댕글링/크래시 예방)
    if (HUDWidgetInstance->GetWorld() != GetWorld())
    {
        UE_LOG(LogTemp, Warning, TEXT("HUDWidgetInstance world mismatch -> null it"));
        return nullptr;
    }
    return HUDWidgetInstance;
}
// 메뉴 UI 표시
void AGLPlayerController::ShowMainMenu(bool bIsRestart)
{
    bMainMenuVisible = true;

    UWidgetLayoutLibrary::RemoveAllWidgets(this);

    if (HUDWidgetInstance)
    {
        HUDWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
        HUDWidgetInstance->RemoveFromParent();
        HUDWidgetInstance = nullptr;
    }

    if (MainMenuWidgetInstance)
    {
        MainMenuWidgetInstance->RemoveFromParent();
        MainMenuWidgetInstance = nullptr;
    }

    if (!MainMenuWidgetClass)
    {
        return;
    }

    MainMenuWidgetInstance = CreateWidget<UUserWidget>(this, MainMenuWidgetClass);
    if (!MainMenuWidgetInstance) return;

    MainMenuWidgetInstance->AddToViewport(999);

    bShowMouseCursor = true;
    bEnableClickEvents = true;
    bEnableMouseOverEvents = true;

    FInputModeUIOnly Mode;
    Mode.SetWidgetToFocus(MainMenuWidgetInstance->TakeWidget());
    Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    SetInputMode(Mode);

    if (UTextBlock* ButtonText = Cast<UTextBlock>(MainMenuWidgetInstance->GetWidgetFromName(TEXT("StartButtonText"))))
    {
        ButtonText->SetText(FText::FromString(bIsRestart ? TEXT("Restart") : TEXT("Start")));
    }
    if (UTextBlock* ExitText = Cast<UTextBlock>(MainMenuWidgetInstance->GetWidgetFromName(TEXT("ExitButtonText"))))
    {
        ExitText->SetText(FText::FromString(bIsRestart ? TEXT("Return To Main Menu") : TEXT("Exit")));;
    }

    if (bIsRestart)
    {
        UFunction* PlayAnimFunc = MainMenuWidgetInstance->FindFunction(FName("PlayGameOverAnim"));
        if (PlayAnimFunc)
        {
            MainMenuWidgetInstance->ProcessEvent(PlayAnimFunc, nullptr);
        }
        if (UFunction* SetFlagFunc = MainMenuWidgetInstance->FindFunction(FName("SetIsRestartMenu")))
        {
            struct FSetIsRestartMenuParams
            {
                bool bRestart;
            };

            FSetIsRestartMenuParams Params;
            Params.bRestart = bIsRestart;

            MainMenuWidgetInstance->ProcessEvent(SetFlagFunc, &Params);
        }

        if (UTextBlock* TotalScoreText = Cast<UTextBlock>(MainMenuWidgetInstance->GetWidgetFromName(TEXT("TotalScoreText"))))
        {
            if (UGLGameInstance* GLGameInstance = Cast<UGLGameInstance>(UGameplayStatics::GetGameInstance(this)))
            {
                TotalScoreText->SetText(FText::FromString(FString::Printf(TEXT("Total Score: %d"), GLGameInstance->TotalScore)));
            }
        }
    }
}


// 게임 HUD 표시
void AGLPlayerController::ShowGameHUD()
{
    bMainMenuVisible = false;

    if (HUDWidgetInstance)
    {
        HUDWidgetInstance->RemoveFromParent();
        HUDWidgetInstance = nullptr;
    }

    if (MainMenuWidgetInstance)
    {
        MainMenuWidgetInstance->RemoveFromParent();
        MainMenuWidgetInstance = nullptr;
    }

    if (!HUDWidgetClass)
    {
        return;
    }

    HUDWidgetInstance = CreateWidget<UUserWidget>(this, HUDWidgetClass);
    if (!HUDWidgetInstance) return;

    HUDWidgetInstance->AddToViewport();

    bShowMouseCursor = false;
    SetInputMode(FInputModeGameOnly());

    AGLGamceState* GLGameState = GetWorld() ? GetWorld()->GetGameState<AGLGamceState>() : nullptr;

    if (GLGameState)
    {
        GLGameState->UpdateHUD();
    }
}


// 게임 시작 - BasicLevel 오픈, GameInstance 데이터 리셋
void AGLPlayerController::StartGame()
{
	if (UGLGameInstance* GLGameInstance = Cast<UGLGameInstance>(UGameplayStatics::GetGameInstance(this)))
	{
		GLGameInstance->CurrentLevelIndex = 0;
		GLGameInstance->TotalScore = 0;
	}

	UGameplayStatics::OpenLevel(GetWorld(), FName("L_StartMap"));
}

void AGLPlayerController::CleanupUIForTravel()
{
    // 1) 입력을 먼저 GameOnly로 돌려서 Slate Focus 경합 차단
    FInputModeGameOnly GameOnly;
    SetInputMode(GameOnly);
    bShowMouseCursor = false;

    // 2) 키보드 포커스/마우스 캡처도 안전하게 날리기
    if (FSlateApplication::IsInitialized())
    {
        FSlateApplication::Get().ClearKeyboardFocus(EFocusCause::SetDirectly);
        FSlateApplication::Get().ClearUserFocus(0, EFocusCause::SetDirectly);
    }

    // 3) 위젯 제거 + 포인터 끊기
    if (IsValid(HUDWidgetInstance)) HUDWidgetInstance->RemoveFromParent();
    HUDWidgetInstance = nullptr;

    if (IsValid(MainMenuWidgetInstance)) MainMenuWidgetInstance->RemoveFromParent();
    MainMenuWidgetInstance = nullptr;

    // (옵션) 혹시 남아있는 위젯 전체 제거
    UWidgetLayoutLibrary::RemoveAllWidgets(this);
}

void AGLPlayerController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}