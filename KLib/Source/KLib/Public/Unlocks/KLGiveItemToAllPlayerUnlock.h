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

	virtual bool IsRepeatPurchasesAllowed_Implementation() const override;
	virtual void Unlock(AFGUnlockSubsystem* unlockSubssytem) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KMods|Unlocks|Repeat Purchases")
	bool bCanRepeat = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KMods|Unlocks|Items")
	TArray<FItemAmount> mAmounts;
};
