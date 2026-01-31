// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FGRecipe.h"
#include "Unlocks/FGUnlock.h"
#include "Unlocks/FGUnlockInfoOnly.h"
#include "UKLUnlockCleanerItem.generated.h"

/**
 * 
 */
UCLASS(Blueprintable, EditInlineNew, abstract, DefaultToInstanced)
class KLIB_API UUKLUnlockCleanerItem : public UFGUnlockInfoOnly
{
	GENERATED_BODY()

	virtual void Unlock(AFGUnlockSubsystem* unlockSubssytem) override;
	virtual void Apply(AFGUnlockSubsystem* unlockSubssytem) override;
	void SendToSubsystem(AFGUnlockSubsystem* unlockSubssytem);

public:
	// for display in Codex (can be Null!)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UFGItemDescriptor> mItemDescriptor;
};
