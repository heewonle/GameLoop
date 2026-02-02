// Fill out your copyright notice in the Description page of Project Settings.


#include "HealingItem.h"
#include "GLCharacter.h"

AHealingItem::AHealingItem()
{
	HealAmount = 20.0f;
	ItemType = "Healing";
}

void AHealingItem::ActivateItem(AActor* Activator)
{
	Super::ActivateItem(Activator);
	// 플레이어 캐릭터의 체력을 20만큼 회복시키는 로직 등을 구현 가능
	if (Activator && Activator->ActorHasTag("Player"))
	{
		if (AGLCharacter* PlayerCharacter = Cast<AGLCharacter>(Activator))
		{
			// 캐릭터의 체력을 회복
			PlayerCharacter->AddHealth(HealAmount);
		}

		DestroyItem();
	}
}