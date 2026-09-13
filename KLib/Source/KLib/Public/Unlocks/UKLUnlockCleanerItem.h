// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "FGRecipe.h"
#include "Unlocks/FGUnlock.h"
#include "Unlocks/FGUnlockInfoOnly.h"

#include "UKLUnlockCleanerItem.generated.h"

UCLASS(Blueprintable, EditInlineNew, DefaultToInstanced)
class KLIB_API UUKLUnlockCleanerItem : public UFGUnlockInfoOnly
{
	GENERATED_BODY()

public:

	virtual void Unlock(AFGUnlockSubsystem* unlockSubssytem) override;
	virtual void Apply(AFGUnlockSubsystem* unlockSubssytem) override;

	void SendToSubsystem(AFGUnlockSubsystem* unlockSubssytem);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UFGItemDescriptor> mItemDescriptor;
};
