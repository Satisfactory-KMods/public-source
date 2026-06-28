#pragma once

#include "CoreMinimal.h"

#include "Equipment/FGBuildGun.h"
#include "KBFLResourceNodeDescriptor.h"

#include "KBFLResourceNodeDescriptor_ResourceNode.generated.h"


USTRUCT(BlueprintType)
struct FKBFLTransformArray
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FTransform> mTransforms;
};

/**
 *
 */
UCLASS()
class KBFL_API UKBFLResourceNodeDescriptor_ResourceNode : public UKBFLResourceNodeDescriptor
{
	GENERATED_BODY()

public:
	virtual void ForeachLocations(TArray<AActor*>& ActorArray) override;
	virtual void ModifyCheckActor(AActor*& InActor, FTransform FoundTransform) override;
	virtual void ModifySpawnedActorPreSpawn(AActor*& InActor) override;
	virtual void AfterSpawning() override;

	virtual TArray<TSubclassOf<AActor>> GetSearchingActorClasses() override { return {mResourceNodeClass}; };

	FORCEINLINE virtual TSubclassOf<AActor> GetActorClass() override { return mResourceNodeClass; }

	UKBFLResourceNodeDescriptor_ResourceNode()
	{
		mPurityLocations.Add(RP_Inpure, FKBFLTransformArray());
		mPurityLocations.Add(RP_Normal, FKBFLTransformArray());
		mPurityLocations.Add(RP_Pure, FKBFLTransformArray());
	}

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Resource Node Locations")
	TMap<TEnumAsByte<EResourcePurity>, FKBFLTransformArray> mPurityLocations;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Resource Node")
	TSubclassOf<AFGResourceNodeBase> mResourceNodeClass = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Resource Node")
	TMap<uint8, TObjectPtr<UMaterialInterface>> mResourceNodeMaterialInfo;

	/** Only add this to OilPump if the Allowed form is valid! */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Extractor")
	bool mAddToOilPump;
};
