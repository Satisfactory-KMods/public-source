#pragma once

#include "CoreMinimal.h"
#include "Unlocks/FGUnlock.h"

#include "KPCLFaxitUnlockBase.generated.h"

UCLASS(Abstract)
class KPRIVATECODELIB_API UKPCLFaxitUnlockBase : public UFGUnlock
{
	GENERATED_BODY()

public:
	virtual bool IsRepeatPurchasesAllowed_Implementation() const override;
	virtual void Unlock(AFGUnlockSubsystem* UnlockSubsystem) override;
	virtual void Apply(AFGUnlockSubsystem* UnlockSubsystem) override;

	virtual void ApplyToFaxit(class AKPCLFaxitSubsystem* Subsystem) const {}

private:
	void RecomputeFaxitValues(AFGUnlockSubsystem* UnlockSubsystem) const;
};
