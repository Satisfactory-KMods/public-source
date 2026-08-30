#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "KPCLRepeatPurchaseUnlock.generated.h"

UINTERFACE(MinimalAPI)
class UKPCLRepeatPurchaseUnlock : public UInterface
{
	GENERATED_BODY()
};

class KPRIVATECODELIB_API IKPCLRepeatPurchaseUnlock
{
	GENERATED_BODY()

public:

	static IKPCLRepeatPurchaseUnlock* FindOnSchematic(TSubclassOf<class UFGSchematic> SchematicClass);

	virtual int32 GetMaxPurchaseCount() const = 0;
	virtual float GetCostMultiplier(int32 CompletedPurchaseCount) const = 0;
	virtual float GetRewardMultiplier(int32 CompletedPurchaseCount) const = 0;
	virtual void ApplyLevelState(TSubclassOf<class UFGSchematic> SchematicClass, int32 CompletedPurchaseCount) = 0;
};
