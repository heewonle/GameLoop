// Fill out your copyright notice in the Description page of Project Settings.


#include "GLGameMode.h"

#include "GLCharacter.h"

#include "GLGamceState.h"

#include "GLPlayerController.h" // PlayerController 클래스를 사용

AGLGameMode::AGLGameMode()
{
    DefaultPawnClass = AGLCharacter::StaticClass();
    PlayerControllerClass = AGLPlayerController::StaticClass();
    GameStateClass = AGLGamceState::StaticClass();
}

void AGLGameMode::BeginPlay()
{
    Super::BeginPlay();
    UE_LOG(LogTemp, Warning, TEXT("[GLGM] BeginPlay Map=%s"), *GetWorld()->GetMapName());
}