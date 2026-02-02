#include "GLGamceState.h"

#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"

#include "GLPlayerController.h"
#include "SpawnVolume.h"
#include "CoinItem.h"
#include "GLGameInstance.h"

#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"

namespace
{
	static ASpawnVolume* FindSpawnVolume(UWorld* World)
	{
		if (!World) return nullptr;

		TArray<AActor*> Found;
		UGameplayStatics::GetAllActorsOfClass(World, ASpawnVolume::StaticClass(), Found);

		for (AActor* A : Found)
		{
			if (ASpawnVolume* V = Cast<ASpawnVolume>(A))
			{
				if (IsValid(V)) return V;
			}
		}
		return nullptr;
	}
}

AGLGamceState::AGLGamceState()
{
	bIsGameOver = false;
	bIsTraveling = false;

	Score = 0;
	SpawnedCoinCount = 0;
	CollectedCoinCount = 0;

	LevelDuration = 30.0f;
	CurrentLevelIndex = 0;
	MaxLevels = 3;

	WavesPerLevel = 3;
	CurrentWaveIndex = 0;

	RequiredCoinCount = 0;
	DefaultRequiredCoins = 0;
	DefaultWaveDuration = 0.f;
}

void AGLGamceState::BeginPlay()
{
	Super::BeginPlay();

	bIsGameOver = false;
	bIsTraveling = false;

	UWorld* World = GetWorld();
	if (!World) return;

	// 메뉴맵이면 게임 로직 스킵
	const FString MapName = World->GetMapName();
	if (MapName.Contains(TEXT("L_Menu")))
	{
		return;
	}

	// HUD 업데이트 타이머 ON
	World->GetTimerManager().SetTimer(
		HUDUpdateTimerHandle,
		this,
		&AGLGamceState::UpdateHUD,
		0.1f,
		true
	);

	StartLevel();

	// HUD 위젯 생성 타이밍 방어(원하면 제거 가능)
	World->GetTimerManager().SetTimerForNextTick(
		FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				if (!IsValid(this)) return;
				if (bIsGameOver || bIsTraveling) return;
				UpdateHUD();
			})
	);
}

int32 AGLGamceState::GetScore() const
{
	return Score;
}

void AGLGamceState::AddScore(int32 Amount)
{
	if (bIsGameOver || bIsTraveling) return;

	Score += Amount;

	if (UGLGameInstance* GI = GetGameInstance<UGLGameInstance>())
	{
		GI->AddToScore(Amount);
	}

	UpdateHUD();
}

void AGLGamceState::StartLevel()
{
	if (bIsGameOver || bIsTraveling) return;

	UWorld* World = GetWorld();
	if (!World) return;

	SpawnedCoinCount = 0;
	CollectedCoinCount = 0;
	CurrentWaveIndex = 0;

	if (UGLGameInstance* GI = GetGameInstance<UGLGameInstance>())
	{
		CurrentLevelIndex = GI->CurrentLevelIndex;
	}

	// HUD 보여주기
	if (APlayerController* PC = World->GetFirstPlayerController())
	{
		if (AGLPlayerController* GLPC = Cast<AGLPlayerController>(PC))
		{
			GLPC->ShowGameHUD();
		}
	}

	StartWave();
	UpdateHUD();
}

void AGLGamceState::StartWave()
{
	if (bIsGameOver || bIsTraveling) return;

	UWorld* World = GetWorld();
	if (!World) return;

	SpawnedCoinCount = 0;
	CollectedCoinCount = 0;

	// 목표 코인
	RequiredCoinCount = DefaultRequiredCoins;
	if (RequiredCoinsPerWave.IsValidIndex(CurrentWaveIndex))
	{
		RequiredCoinCount = RequiredCoinsPerWave[CurrentWaveIndex];
	}

	// 웨이브 시간
	float WaveTime = DefaultWaveDuration;
	if (WaveDurations.IsValidIndex(CurrentWaveIndex))
	{
		WaveTime = WaveDurations[CurrentWaveIndex];
	}
	WaveTime = FMath::Max(0.1f, WaveTime);

	// 웨이브 타이머
	FTimerManager& TM = World->GetTimerManager();
	TM.ClearTimer(WaveTimerHandle);
	TM.SetTimer(
		WaveTimerHandle,
		this,
		&AGLGamceState::OnWaveTimeUp,
		WaveTime,
		false
	);

	// 스폰 개수
	int32 ItemToSpawn = 40;
	if (ItemsToSpawnPerWave.IsValidIndex(CurrentWaveIndex))
	{
		ItemToSpawn = ItemsToSpawnPerWave[CurrentWaveIndex];
	}
	ItemToSpawn = FMath::Max(0, ItemToSpawn);

	ASpawnVolume* SpawnVolume = FindSpawnVolume(World);
	if (!IsValid(SpawnVolume))
	{
		// 스폰 볼륨이 없으면 이번 웨이브는 바로 종료 처리
		EndWave();
		return;
	}

	for (int32 i = 0; i < ItemToSpawn; ++i)
	{
		if (bIsGameOver || bIsTraveling) break;
		if (!IsValid(SpawnVolume)) break;

		AActor* SpawnedActor = SpawnVolume->SpawnRandomItem();
		if (IsValid(SpawnedActor) && SpawnedActor->IsA(ACoinItem::StaticClass()))
		{
			SpawnedCoinCount++;
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("Wave %d Start! Coins: %d, Goal: %d, TimeLimit: %.1f sec"), CurrentWaveIndex + 1, SpawnedCoinCount, RequiredCoinCount, WaveTime);
	// 목표가 스폰보다 크면 보정
	RequiredCoinCount = FMath::Clamp(RequiredCoinCount, 0, SpawnedCoinCount);

	UpdateHUD();

	// 코인이 0개면 바로 다음 처리
	if (SpawnedCoinCount <= 0)
	{
		EndWave();
	}
}

void AGLGamceState::OnWaveTimeUp()
{
	if (bIsGameOver || bIsTraveling) return;

	OnGameOver();
}

void AGLGamceState::OnCoinCollected()
{
	if (bIsGameOver || bIsTraveling) return;

	CollectedCoinCount++;
	UpdateHUD();

	if (CollectedCoinCount >= RequiredCoinCount)
	{
		EndWave();
	}
}

void AGLGamceState::ClearWaveItems()
{
	UWorld* World = GetWorld();
	if (!World) return;

	TArray<AActor*> Coins;
	UGameplayStatics::GetAllActorsOfClass(World, ACoinItem::StaticClass(), Coins);

	for (AActor* A : Coins)
	{
		if (IsValid(A))
		{
			A->Destroy();
		}
	}
}

void AGLGamceState::EndWave()
{
	if (bIsGameOver || bIsTraveling) return;

	UWorld* World = GetWorld();
	if (!World) return;

	FTimerManager& TM = World->GetTimerManager();
	TM.ClearTimer(WaveTimerHandle);

	ClearWaveItems();

	CurrentWaveIndex++;
	UpdateHUD();

	if (CurrentWaveIndex >= WavesPerLevel)
	{
		EndLevel();
		return;
	}

	StartWave();
}

void AGLGamceState::EndLevel()
{
	if (bIsGameOver || bIsTraveling) return;

	UWorld* World = GetWorld();
	if (!World) return;

	bIsTraveling = true;

	// 타이머 정리
	World->GetTimerManager().ClearTimer(HUDUpdateTimerHandle);
	World->GetTimerManager().ClearTimer(WaveTimerHandle);
	World->GetTimerManager().ClearAllTimersForObject(this);

	UGLGameInstance* GI = GetGameInstance<UGLGameInstance>();
	if (!GI)
	{
		bIsGameOver = true;
		return;
	}

	GI->CurrentLevelIndex++;

	// 마지막 레벨이면 메뉴로
	if (GI->CurrentLevelIndex >= MaxLevels || !LevelMapNames.IsValidIndex(GI->CurrentLevelIndex))
	{
		bIsGameOver = true;

		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			if (AGLPlayerController* GLPC = Cast<AGLPlayerController>(PC))
			{
				GLPC->ShowMainMenu(true);
			}
		}
		return;
	}

	const FName NextMap = LevelMapNames[GI->CurrentLevelIndex];

	// UI 정리
	if (APlayerController* PC = World->GetFirstPlayerController())
	{
		if (AGLPlayerController* GLPC = Cast<AGLPlayerController>(PC))
		{
			GLPC->CleanupUIForTravel();
		}
	}

	UGameplayStatics::OpenLevel(this, NextMap);
}

void AGLGamceState::OnGameOver()
{
	if (bIsGameOver || bIsTraveling) return;

	UWorld* World = GetWorld();
	if (!World) return;

	bIsGameOver = true;

	// 타이머 정리
	World->GetTimerManager().ClearTimer(HUDUpdateTimerHandle);
	World->GetTimerManager().ClearTimer(WaveTimerHandle);
	World->GetTimerManager().ClearAllTimersForObject(this);

	if (APlayerController* PC = World->GetFirstPlayerController())
	{
		if (AGLPlayerController* GLPC = Cast<AGLPlayerController>(PC))
		{
			GLPC->ShowMainMenu(true);
		}
	}
}

void AGLGamceState::UpdateHUD()
{
	if (bIsGameOver || bIsTraveling) return;

	UWorld* World = GetWorld();
	if (!World || World->bIsTearingDown) return;

	APlayerController* PC = World->GetFirstPlayerController();
	AGLPlayerController* GLPC = Cast<AGLPlayerController>(PC);
	if (!GLPC) return;

	if (GLPC->IsMainMenuVisible()) return;

	UUserWidget* HUDWidget = GLPC->GetHUDWidget();
	if (!IsValid(HUDWidget)) return;

	// Level 표시(항상 GI 기준 우선)
	int32 LevelToShow = CurrentLevelIndex + 1;
	if (UGLGameInstance* GI = GetGameInstance<UGLGameInstance>())
	{
		LevelToShow = GI->CurrentLevelIndex + 1;
	}

	const int32 WaveToShow = CurrentWaveIndex + 1;
	const int32 TotalWaves = WavesPerLevel;

	const int32 GoalToShow = RequiredCoinCount;
	const int32 CollectedToShow = CollectedCoinCount;

	// 남은 시간
	float RemainingTime = 0.f;
	FTimerManager& TM = World->GetTimerManager();
	if (TM.IsTimerActive(WaveTimerHandle))
	{
		RemainingTime = FMath::Max(0.f, TM.GetTimerRemaining(WaveTimerHandle));
	}

	if (UTextBlock* T = Cast<UTextBlock>(HUDWidget->GetWidgetFromName(TEXT("Wave"))))
	{
		T->SetText(FText::FromString(FString::Printf(TEXT("Wave: %d / %d"), WaveToShow, TotalWaves)));
	}

	if (UTextBlock* T = Cast<UTextBlock>(HUDWidget->GetWidgetFromName(TEXT("Level"))))
	{
		T->SetText(FText::FromString(FString::Printf(TEXT("Level: %d"), LevelToShow)));
	}

	if (UTextBlock* T = Cast<UTextBlock>(HUDWidget->GetWidgetFromName(TEXT("Goal"))))
	{
		T->SetText(FText::FromString(FString::Printf(TEXT("Goal: %d"), GoalToShow)));
	}

	if (UTextBlock* T = Cast<UTextBlock>(HUDWidget->GetWidgetFromName(TEXT("Collected"))))
	{
		T->SetText(FText::FromString(FString::Printf(TEXT("Collected: %d"), CollectedToShow)));
	}

	if (UTextBlock* T = Cast<UTextBlock>(HUDWidget->GetWidgetFromName(TEXT("Time"))))
	{
		T->SetText(FText::FromString(FString::Printf(TEXT("Time: %.1f"), RemainingTime)));
	}
}

void AGLGamceState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearAllTimersForObject(this);
	}
	Super::EndPlay(EndPlayReason);
}
