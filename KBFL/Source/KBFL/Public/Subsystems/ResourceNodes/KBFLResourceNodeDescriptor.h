#pragma once

#include "CoreMinimal.h"
#include "KBFLActorSpawnDescriptorBase.h"
#include "Resources/FGResourceDescriptor.h"
#include "Resources/FGResourceNode.h"

#include "KBFLResourceNodeDescriptor.generated.h"

/**
 *
 */
UCLASS(Blueprintable, EditInlineNew, abstract, DefaultToInstanced)
class KBFL_API UKBFLResourceNodeDescriptor : public UKBFLActorSpawnDescriptorBase
{
	GENERATED_BODY()

public:
	UKBFLResourceNodeDescriptor() : mLastPur() { mActorFreeClass = AFGResourceNodeBase::StaticClass(); }

	virtual bool IsAllowedToRemoveActor(AActor* InActor) override;

	virtual TSubclassOf<AActor> GetActorFreeClass() override
	{
		return mActorFreeClass ? mActorFreeClass : TSubclassOf<AActor>{AFGResourceNodeBase::StaticClass()};
	}

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Resource Node")
	TSubclassOf<UFGResourceDescriptor> mResourceClass = nullptr;

	// Bool
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config", meta = (editcondition = "mRemoveOld"))
	bool mRemoveOccupied = false;

	// Floats
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	TEnumAsByte<EResourceAmount> mAmount = RA_Infinite;

	EResourcePurity mLastPur;
};
