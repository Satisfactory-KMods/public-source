// ILikeBanas

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "KPCLRepeatPurchaseUnlock.generated.h"

UINTERFACE(MinimalAPI)
class UKPCLRepeatPurchaseUnlock : public UInterface
{
	GENERATED_BODY()
};

/** Module-independent contract used by KPCL repeat-purchase save and replication handling. */
class KPRIVATECODELIB_API IKPCLRepeatPurchaseUnlock
{
	GENERATED_BODY()

public:
	/** Finds a repeat-purchase unlock implementation on the supplied schematic CDO. */
	static IKPCLRepeatPurchaseUnlock* FindOnSchematic(TSubclassOf<class UFGSchematic> SchematicClass);

	virtual int32 GetMaxPurchaseCount() const = 0;
	virtual float GetCostMultiplier(int32 CompletedPurchaseCount) const = 0;
	virtual float GetRewardMultiplier(int32 CompletedPurchaseCount) const = 0;
	virtual void ApplyLevelState(TSubclassOf<class UFGSchematic> SchematicClass, int32 CompletedPurchaseCount) = 0;
};
