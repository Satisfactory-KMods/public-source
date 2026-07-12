//

#pragma once

#include "CoreMinimal.h"
#include "KBFLCDOOverwriteBase.h"
#include "Resources/FGItemDescriptor.h"

#include "KBFLCDOItemStackSize.generated.h"

USTRUCT(BlueprintType)
struct FKBFLItemArray
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TSubclassOf<UFGItemDescriptor>> mItems = {};
};

/**
 * Data asset that sets item stack sizes via the CDO overwrite system.
 */
UCLASS()
class KBFL_API UKBFLCDOItemStackSize : public UKBFLCDOOverwriteBase
{
	GENERATED_BODY()

public:
	/** Applies each configured stack size to its mapped item descriptors, updating mStackSize and the cached value. */
	virtual void ApplyToInstances() override;

	// Lazy-load path: re-apply to a single item class loaded after the initial CDO pass.
	virtual bool ShouldCallForInstance(UClass* NewClass) override;

	/** Lazy-load handler: sets the configured stack size on a single loaded item descriptor instance. */
	virtual void ApplyToInstance(UObject* Instance) override;

	// ===== Stack Size Settings =====
	/** Map of target stack size to the items that should receive it */
	UPROPERTY(EditAnywhere, Category = "Stack Size Settings")
	TMap<EStackSize, FKBFLItemArray> mItemStackSizeCDO;
};
