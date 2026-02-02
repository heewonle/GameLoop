// Fill out your copyright notice in the Description page of Project Settings.


#include "GLGameStateBase.h"

AGLGameStateBase::AGLGameStateBase()
{
    Score = 0;
}

int32 AGLGameStateBase::GetScore() const
{
    return Score;
}

void AGLGameStateBase::AddScore(int32 Amount)
{
    Score += Amount;
}