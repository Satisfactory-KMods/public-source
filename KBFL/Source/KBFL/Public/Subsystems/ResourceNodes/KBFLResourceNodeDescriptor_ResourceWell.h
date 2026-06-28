#pragma once

#include "CoreMinimal.h"
#include "KBFLResourceNodeDescriptor.h"
#include "KBFLResourceNodeDescriptor_ResourceNode.h"
#include "Resources/FGResourceNodeFrackingCore.h"
#include "Resources/FGResourceNodeFrackingSatellite.h"

#include "KBFLResourceNodeDescriptor_ResourceWell.generated.h"

class AFGResourceNodeFrackingCore;

USTRUCT(BlueprintType)
struct FKBFLCoreInformations
{
	GENERATED_BODY()

	FKBFLCoreInformations()
	{
		mPurityLocations.Add(RP_Inpure, FKBFLTransformArray());
		mPurityLocations.Add(RP_Normal, FKBFLTransformArray());
		mPurityLocations.Add(RP_Pure, FKBFLTransformArray());
	}

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locations")
	FTransform mCoreLocation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locations")
	TArray<FTransform> mCrackLocations;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = " Locations")
	TMap<TEnumAsByte<EResourcePurity>, FKBFLTransformArray> mPurityLocations;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Materials")
	TMap<uint8, TObjectPtr<UMaterialInterface>> mFrackingCoreMaterialInfo;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Materials")
	TMap<uint8, TObjectPtr<UMaterialInterface>> mFrackingSatelliteMaterialInfo;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Materials")
	TMap<uint8, TObjectPtr<UMaterialInterface>> mCrackMaterialInfo;
};

/**
 *
 */
UCLASS()
class KBFL_API UKBFLResourceNodeDescriptor_ResourceWell : public UKBFLResourceNodeDescriptor
{
	GENERATED_BODY()

public:
	virtual void ForeachLocations(TArray<AActor*>& ActorArray) override;
	virtual void ModifyCheckActor(AActor*& InActor, FTransform FoundTransform) override;
	virtual void ModifySpawnedActorPreSpawn(AActor*& InActor) override;
	virtual void ModifySpawnedActorPostSpawn(AActor*& InActor) override;

	virtual TArray<TSubclassOf<AActor>> GetSearchingActorClasses() override
	{
		return {mFrackingCoreClass, mFrackingSatelliteClass};
	};

	virtual TArray<TEnumAsByte<EObjectTypeQuery>> GetSphereCheckChannels() override;

	virtual void AfterSpawning() override;

	void Validate(TArray<AFGResourceNodeFrackingSatellite*> Satellites);

	virtual bool IsAllowedToRemoveActor(AActor* InActor) override;

	virtual void RemoveWrongActors(TArray<AActor*>& ActorArray) override;

	uint32 GetCountOfSat(TMap<TEnumAsByte<EResourcePurity>, FKBFLTransformArray> SatMap);
	/*virtual void ModifySpawnedActorPostSpawn(AActor*& InActor) override {
	Super::ModifySpawnedActorPostSpawn(InActor);
	ModifySpawnedActorPreSpawn(InActor);
	}*/

	FORCEINLINE virtual TSubclassOf<AActor> GetActorClass() override
	{
		if (!bIsInSatelliteSpawning)
		{
			return mFrackingCoreClass;
		}
		return mFrackingSatelliteClass;
	}

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Resource Node Locations")
	TArray<FKBFLCoreInformations> mLocations;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Resource Node")
	TSubclassOf<AFGResourceNodeFrackingCore> mFrackingCoreClass =
		LoadClass<AActor>(nullptr, TEXT("/KBFL/ResourceNodeActors/BaseNode_FrackingCore.BaseNode_FrackingCore_C"));

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Resource Node")
	TSubclassOf<AFGResourceNodeFrackingSatellite> mFrackingSatelliteClass =
		LoadClass<AActor>(nullptr, TEXT("/KBFL/ResourceNodeActors/BaseNode_FrackingSat.BaseNode_FrackingSat_C"));

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Resource Node")
	TSubclassOf<AActor> mCrackClass =
		LoadClass<AActor>(nullptr, TEXT("/KBFL/ResourceNodeActors/SMA_Crack.SMA_Crack_C"));
	;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<AFGResourceNodeFrackingCore> mLastCore;

	UPROPERTY(BlueprintReadWrite)
	TArray<FTransform> mCoreTransforms;
	bool bIsInSatelliteSpawning = false;
	bool bIsInCrackSpawning = false;
};
