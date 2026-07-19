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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KMods|Unlocks|Repeat Purchases")
	bool bCanRepeat = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KMods|Unlocks|Items")
	TArray<FItemAmount> mAmounts;
};
