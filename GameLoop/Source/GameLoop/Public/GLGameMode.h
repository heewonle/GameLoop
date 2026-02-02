// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "GLGameMode.generated.h"

/**
 * 
 */
UCLASS()
class GAMELOOP_API AGLGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	AGLGameMode();
	void BeginPlay();

	
};
