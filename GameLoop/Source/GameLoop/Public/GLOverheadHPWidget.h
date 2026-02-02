// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GLOverheadHPWidget.generated.h"

class UProgressBar;
class UWidgetAnimation;

UCLASS()
class GAMELOOP_API UGLOverheadHPWidget : public UUserWidget
{
	GENERATED_BODY()
public:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    // 캐릭터에서 "현재 HP/최대HP"를 넣어줌
    UFUNCTION(BlueprintCallable)
    void SetHP(float InHealth, float InMaxHealth);

    // 데미지 받았을 때 호출(깜빡임)
    UFUNCTION(BlueprintCallable)
    void PlayHitFlash();

protected:

    UPROPERTY(meta = (BindWidgetOptional))
    UProgressBar* HPBar = nullptr;


    // 표시값(부드럽게 따라오는 값)
    float TargetPercent = 1.f;
    float DisplayPercent = 1.f;

    // 보간 속도(값이 클수록 빨리 따라옴)
    UPROPERTY(EditAnywhere, Category = "HP")
    float InterpSpeed = 8.f;

    FTimerHandle InterpTimerHandle;

    void TickInterp();

    // 위젯 애니메이션(디자이너에서 만든 Anim_HitFlash)
    UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
    UWidgetAnimation* Anim_HitFlash = nullptr;
private:
    FTimerHandle HitFlashTimerHandle;
};
