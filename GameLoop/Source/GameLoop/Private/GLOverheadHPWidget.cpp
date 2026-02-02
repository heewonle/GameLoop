#include "GLOverheadHPWidget.h"
#include "Components/ProgressBar.h"
#include "TimerManager.h"

void UGLOverheadHPWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 초기값 반영
    if (HPBar)      HPBar->SetPercent(DisplayPercent);
}

void UGLOverheadHPWidget::NativeDestruct()
{
    // 위젯이 화면에서 제거/파괴될 때 타이머 정리 (크래시 방지 핵심)
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(InterpTimerHandle);
        World->GetTimerManager().ClearTimer(HitFlashTimerHandle);
        // 필요하면 더 강하게:
        // World->GetTimerManager().ClearAllTimersForObject(this);
    }

    Super::NativeDestruct();
}

void UGLOverheadHPWidget::SetHP(float InHealth, float InMaxHealth)
{
    const float NewTarget =
        (InMaxHealth > 0.f) ? FMath::Clamp(InHealth / InMaxHealth, 0.f, 1.f) : 0.f;

    TargetPercent = NewTarget;

    // 처음 호출 시(또는 즉시 반영하고 싶으면) DisplayPercent를 같이 맞춰도 됨
    // DisplayPercent = TargetPercent;

    // 보간 타이머가 꺼져 있으면 켜기
    if (UWorld* World = GetWorld())
    {
        if (!World->GetTimerManager().IsTimerActive(InterpTimerHandle))
        {
            World->GetTimerManager().SetTimer(
                InterpTimerHandle,
                this,
                &UGLOverheadHPWidget::TickInterp,
                0.016f, // ~60fps
                true
            );
        }
    }
}

void UGLOverheadHPWidget::TickInterp()
{
    // 프레임 보간
    const float DT = 0.016f; // 타이머 주기와 맞춰서
    DisplayPercent = FMath::FInterpTo(DisplayPercent, TargetPercent, DT, InterpSpeed);

    if (HPBar) HPBar->SetPercent(DisplayPercent);

    // 충분히 근접하면 타이머 끄기(최적화)
    if (FMath::Abs(DisplayPercent - TargetPercent) < 0.001f)
    {
        DisplayPercent = TargetPercent;
        if (HPBar) HPBar->SetPercent(DisplayPercent);

        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().ClearTimer(InterpTimerHandle);
        }
    }
}

void UGLOverheadHPWidget::PlayHitFlash()
{
    // 애니 있으면 애니 재생
    if (Anim_HitFlash)
    {
        PlayAnimation(Anim_HitFlash, 0.f, 1);
        return;
    }

    // 애니가 없으면 최소 깜빡임(간단 버전)
    if (!HPBar) return;

    HPBar->SetRenderOpacity(0.2f);

    if (UWorld* World = GetWorld())
    {
        // 기존에 남아있던 깜빡임 타이머가 있으면 갱신 (중복 실행 방지)
        World->GetTimerManager().ClearTimer(HitFlashTimerHandle);

        // 핵심: raw this 캡처 람다 대신 WeakLambda 사용 (this 죽으면 실행 안 함)
        FTimerDelegate D = FTimerDelegate::CreateWeakLambda(this, [this]()
            {
                if (HPBar) HPBar->SetRenderOpacity(1.f);
            });

        World->GetTimerManager().SetTimer(HitFlashTimerHandle, D, 0.08f, false);
    }
}
