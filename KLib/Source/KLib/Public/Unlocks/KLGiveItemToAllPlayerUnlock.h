// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Unlocks/FGUnlock.h"
#include "FGUnlockSubsystem.h"

#include "KLGiveItemToAllPlayerUnlock.generated.h"

UCLASS(Blueprintable, EditInlineNew, abstract, DefaultToInstanced)
class KLIB_API UKLGiveItemToAllPlayerUnlock : public UFGUnlock
{
	GENERATED_BODY()

public:
	virtual bool IsRepeatPurchasesAllowed_Implementation() const override;

	virtual void Unlock(AFGUnlockSubsystem* unlockSubssytem) override;

private:
	void GiveToPlayer(AFGCharacterPlayer* Player);

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool mCanRepeat = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FItemAmount> mAmounts;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool mToAllPlayer = false;
};
