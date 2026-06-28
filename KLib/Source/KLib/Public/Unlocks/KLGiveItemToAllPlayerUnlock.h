// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "FGUnlockSubsystem.h"
#include "Unlocks/FGUnlock.h"

#include "KLGiveItemToAllPlayerUnlock.generated.h"

UCLASS(Blueprintable, EditInlineNew, DefaultToInstanced)
class KLIB_API UKLGiveItemToAllPlayerUnlock : public UFGUnlock
{
	GENERATED_BODY()

public:
	//~ Begin UFGUnlock Interface
	virtual bool IsRepeatPurchasesAllowed_Implementation() const override;
	virtual void Unlock(AFGUnlockSubsystem* unlockSubssytem) override;
	//~ End UFGUnlock Interface

	void GiveToPlayer(AFGCharacterPlayer* Player);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool mCanRepeat = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool mToAllPlayer = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FItemAmount> mAmounts;
};
