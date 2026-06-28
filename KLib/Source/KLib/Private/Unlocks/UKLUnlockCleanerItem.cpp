// Fill out your copyright notice in the Description page of Project Settings.

#include "Unlocks/UKLUnlockCleanerItem.h"

#include "FGRecipeManager.h"
#include "FGUnlockSubsystem.h"
#include "Subsystem/KLUnlockSubsystem.h"


void UUKLUnlockCleanerItem::Unlock(AFGUnlockSubsystem* unlockSubssytem) { Super::Unlock(unlockSubssytem); }

void UUKLUnlockCleanerItem::Apply(AFGUnlockSubsystem* unlockSubssytem)
{
	Super::Apply(unlockSubssytem);
	SendToSubsystem(unlockSubssytem);
}

void UUKLUnlockCleanerItem::SendToSubsystem(AFGUnlockSubsystem* unlockSubssytem)
{
	fgcheckf(mItemDescriptor, TEXT("UnlockCleanerItem %s has no ItemDescriptor set!"), *GetName());
	if (unlockSubssytem)
	{
		if (AKLUnlockSubsystem* Subsystem = AKLUnlockSubsystem::Get(unlockSubssytem))
		{
			Subsystem->UnlockCleanerDesc(mItemDescriptor);
		}
	}
}
