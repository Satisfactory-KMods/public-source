// ILikeBanas

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
	//~ Begin UFGUnlockInfoOnly Interface
	virtual void Unlock(AFGUnlockSubsystem* unlockSubssytem) override;
	virtual void Apply(AFGUnlockSubsystem* unlockSubssytem) override;
	//~ End UFGUnlockInfoOnly Interface

	void SendToSubsystem(AFGUnlockSubsystem* unlockSubssytem);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UFGItemDescriptor> mItemDescriptor;
};
