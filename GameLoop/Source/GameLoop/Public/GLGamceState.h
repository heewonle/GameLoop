// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "GLGamceState.generated.h"

/**
 * 
 */
UCLASS()
class GAMELOOP_API AGLGamceState : public AGameState
{
	GENERATED_BODY()
public:
	AGLGamceState();
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnyWhere, BlueprintReadWrite, Category = "Score")
	int32 Score;
	// 현재 레벨에서 스폰된 코인 개수
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Coin")
	int32 SpawnedCoinCount;
	// 플레이어가 수집한 코인 개수
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Coin")
	int32 CollectedCoinCount;
	// 각 레벨이 유지되는 시간 (초 단위)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Level")
	float LevelDuration;
	// 현재 진행 중인 레벨 인덱스
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Level")
	int32 CurrentLevelIndex;
	// 전체 레벨의 개수
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Level")
	int32 MaxLevels;
	// 실제 레벨 맵 이름 배열. 여기 있는 인덱스를 차례대로 연동
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
	TArray<FName> LevelMapNames;

	// 매 레벨이 끝나기 전까지 시간이 흐르도록 관리하는 타이머
	FTimerHandle LevelTimerHandle;

	UFUNCTION(BlueprintPure, Category = "Score")
	int32 GetScore() const;
	UFUNCTION(BlueprintCallable, Category = "Score")
	void AddScore(int32 Amount);
	// 게임이 완전히 끝났을 때 (모든 레벨 종료) 실행되는 함수
	UFUNCTION(BlueprintCallable, Category = "Level")
	void OnGameOver();

	// ===== 웨이브 설정(추가) =====
	UPROPERTY(EditAnywhere, Category = "Wave")
	int32 WavesPerLevel = 3;

	UPROPERTY(VisibleAnywhere, Category = "Wave")
	int32 CurrentWaveIndex = 0; // 0부터 시작

	// 웨이브별 스폰 개수(선택) - 없으면 기본값 사용
	UPROPERTY(EditAnywhere, Category = "Wave")
	TArray<int32> ItemsToSpawnPerWave; // 예: [15, 20, 25]

	// 웨이브별 제한시간(예: [10, 12, 15])
	UPROPERTY(EditAnywhere, Category = "Wave")
	TArray<float> WaveDurations;

	// 기본 웨이브 제한시간(배열 없거나 인덱스 범위 밖이면 사용)
	UPROPERTY(EditAnywhere, Category = "Wave")
	float DefaultWaveDuration = 10.f;

	// 웨이브별 클리어에 필요한 코인 수
	UPROPERTY(EditAnywhere, Category = "Wave")
	TArray<int32> RequiredCoinsPerWave;

	// 기본 요구 코인 수 (배열 범위 밖일 때 사용)
	UPROPERTY(EditAnywhere, Category = "Wave")
	int32 DefaultRequiredCoins = 10;
		
	void OnWaveTimeUp();
	void StartWave();
	void EndWave();
	void ClearWaveItems();

	void StopHUDUpdateTimer();

	// 레벨을 시작할 때, 아이템 스폰 및 타이머 설정
	void StartLevel();
	// 레벨 제한 시간이 만료되었을 때 호출

	// 코인을 주웠을 때 호출
	void OnCoinCollected();
	// 레벨을 강제 종료하고 다음 레벨로 이동
	void EndLevel();
	// HUD 갱신
	void UpdateHUD();
	// 웨이브 타이머
	FTimerHandle WaveTimerHandle;
	// 현재 웨이브의 클리어 목표
	int32 RequiredCoinCount;

	FTimerHandle HUDUpdateTimerHandle;

	void EndPlay(const EEndPlayReason::Type EndPlayReason);

	UPROPERTY()
	bool bIsTraveling = false;
private:
	bool bIsGameOver = false;
};
