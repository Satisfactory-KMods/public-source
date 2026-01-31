#pragma once
#include "Subsystems/ResourceNodes/KBFLResourceNodeDescriptor_ResourceNode.h"

#include "Buildables/FGBuildableResourceExtractor.h"
#include "Subsystems/KBFLContentCDOHelperSubsystem.h"

void UKBFLResourceNodeDescriptor_ResourceNode::ForeachLocations(TArray<AActor*>& ActorArray)
{
	SetSphereCheckChannels(TArray<TEnumAsByte<EObjectTypeQuery>>{
		UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel3), ObjectTypeQuery9});

	if (mResourceClass)
	{
		for (TTuple<TEnumAsByte<EResourcePurity>, FKBFLTransformArray> PurityMap : mPurityLocations)
		{
			mLastPur = PurityMap.Key;
			for (FTransform Location : PurityMap.Value.mTransforms)
			{
				AActor* OutActor;
				if (!CheckActorInRange(Location, OutActor) && IsRangeFree(Location))
				{
					OutActor = SpawnActorAtLocation(Location, GetActorClass());
				}

				if (OutActor)
				{
					ApplyMaterialData(OutActor, mResourceNodeMaterialInfo);
					ActorArray.Add(OutActor);
				}
			}
		}
	}
}

void UKBFLResourceNodeDescriptor_ResourceNode::ModifyCheckActor(AActor*& InActor, FTransform FoundTransform)
{
	Super::ModifyCheckActor(InActor, FoundTransform);

	AFGResourceNode* ResourceNode = Cast<AFGResourceNode>(InActor);
	if (ResourceNode)
	{
		ResourceNode->InitResource(mResourceClass, mAmount, mLastPur);
	}
}

void UKBFLResourceNodeDescriptor_ResourceNode::ModifySpawnedActorPreSpawn(AActor*& InActor)
{
	Super::ModifySpawnedActorPreSpawn(InActor);

	AFGResourceNode* ResourceNode = Cast<AFGResourceNode>(InActor);
	if (ResourceNode)
	{
		ResourceNode->InitResource(mResourceClass, mAmount, mLastPur);
	}
}

void UKBFLResourceNodeDescriptor_ResourceNode::AfterSpawning()
{
	UKBFLContentCDOHelperSubsystem* Sub = UKBFLContentCDOHelperSubsystem::Get(GetWorld());

	if (mAddToOilPump && mResourceClass && Sub)
	{
		if (TSubclassOf<AFGBuildableResourceExtractor> BPOilPump = LoadClass<AFGBuildableResourceExtractor>(
				nullptr, TEXT("/Game/FactoryGame/Buildable/Factory/OilPump/Build_OilPump.Build_OilPump_C")))
		{
			if (AFGBuildableResourceExtractor* OilPumpDefault =
					Sub->GetAndStoreDefaultObject_Native<AFGBuildableResourceExtractor>(BPOilPump))
			{
				Sub->StoreClass(mResourceClass);
				OilPumpDefault->mAllowedResources.AddUnique(mResourceClass);
			}
		}
	}
}
