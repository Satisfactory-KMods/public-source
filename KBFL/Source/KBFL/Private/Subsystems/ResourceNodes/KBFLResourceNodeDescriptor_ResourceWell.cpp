#include "Subsystems/ResourceNodes/KBFLResourceNodeDescriptor_ResourceWell.h"

#include "Buildables/FGBuildableFrackingActivator.h"
#include "Buildables/FGBuildableFrackingExtractor.h"
#include "EngineUtils.h"
#include "KBFLLogging.h"
#include "Resources/FGResourceNodeFrackingCore.h"
#include "Resources/FGResourceNodeFrackingSatellite.h"
#include "Subsystems/KBFLContentCDOHelperSubsystem.h"

uint32
UKBFLResourceNodeDescriptor_ResourceWell::GetCountOfSat(TMap<TEnumAsByte<EResourcePurity>, FKBFLTransformArray> SatMap)
{
	uint32 count = 0;
	for (auto Map : SatMap)
	{
		count += Map.Value.mTransforms.Num();
	}

	return count;
}

void UKBFLResourceNodeDescriptor_ResourceWell::ForeachLocations(TArray<AActor*>& ActorArray)
{
	// check form that is will valid (solid is not allowed on resource wells!)
	if (mResourceClass)
	{
		EResourceForm Form = UFGItemDescriptor::GetForm(mResourceClass);
		if (Form != EResourceForm::RF_GAS && Form != EResourceForm::RF_LIQUID)
		{
			UE_LOG(KBFLActorSpawnerLog, Warning, TEXT("Skipp well spawn (Liquid & Gas only!)"));
			return;
		}
	}

	for (FKBFLCoreInformations WellData : mLocations)
	{
		if (GetCountOfSat(WellData.mPurityLocations) == 0 || WellData.mCoreLocation.GetLocation() == FVector(0))
		{
			UE_LOG(KBFLActorSpawnerLog, Warning, TEXT("GetCountOfSat == 0 : Skip!"));
			continue;
		}

		bIsInSatelliteSpawning = false;

		UE_LOG(KBFLActorSpawnerLog, Warning, TEXT("Begin Spawn for Well Data"));
		mCoreTransforms.Add(WellData.mCoreLocation);
		AActor* OutActor;

		if (!CheckActorInRange(WellData.mCoreLocation, OutActor) && IsRangeFree(WellData.mCoreLocation))
		{
			UE_LOG(KBFLActorSpawnerLog, Warning, TEXT("Try Core spawn at: %s"), *WellData.mCoreLocation.ToString());
			OutActor = SpawnActorAtLocation(WellData.mCoreLocation, GetActorClass());
		}

		mLastCore = Cast<AFGResourceNodeFrackingCore>(OutActor);

		if (mLastCore)
		{
			ApplyMaterialData(mLastCore, WellData.mFrackingCoreMaterialInfo);
			ActorArray.Add(mLastCore);
		}

		if (mLastCore)
		{
			TArray<AFGResourceNodeFrackingSatellite*> Satellites = {};
			bIsInSatelliteSpawning = true;
			for (TTuple<TEnumAsByte<EResourcePurity>, FKBFLTransformArray> PurityMap : WellData.mPurityLocations)
			{
				mLastPur = PurityMap.Key;
				for (FTransform Location : PurityMap.Value.mTransforms)
				{
					if (Location.GetLocation() != FVector(0))
					{
						if (!CheckActorInRange(Location, OutActor) && IsRangeFree(Location))
						{
							OutActor = SpawnActorAtLocation(Location, mFrackingSatelliteClass);
						}

						if (OutActor)
						{
							ApplyMaterialData(OutActor, WellData.mFrackingSatelliteMaterialInfo);
							ActorArray.Add(OutActor);
						}

						if (AFGResourceNodeFrackingSatellite* Satellite =
								Cast<AFGResourceNodeFrackingSatellite>(OutActor))
						{
							Satellites.AddUnique(Satellite);
						}
					}
				}
			}

			Validate(Satellites);

			bIsInCrackSpawning = true;
			for (FTransform Location : WellData.mCrackLocations)
			{
				OutActor = SpawnActorAtLocation(Location, mCrackClass);
				if (OutActor)
				{
					ApplyMaterialData(OutActor, WellData.mCrackMaterialInfo);
					ActorArray.AddUnique(OutActor);
				}
			}
			bIsInCrackSpawning = false;
		}
		else
		{
			UE_LOG(KBFLActorSpawnerLog, Warning, TEXT("Core is invalid!!!"));
		}
	}

	bIsInSatelliteSpawning = false;
}

void UKBFLResourceNodeDescriptor_ResourceWell::Validate(TArray<AFGResourceNodeFrackingSatellite*> Satellites)
{
	// mLastCore->InitResource(mResourceClass);
	if (mLastCore->mActivator.IsValid())
	{
		mLastCore->mActivator->mSatelliteNodeCount = mLastCore->mSatellites.Num();
		mLastCore->mActivator->CalculateConnectedExtractorCount();
		mLastCore->mActivator->CalculateDefaultPotentialExtractionPerMinute();
	}

	/*
	TArray<AFGResourceNodeFrackingSatellite*> AllreadyFound = {};
	for (int i = 0; i < mLastCore->mSatellites.Num(); ++i)
	{
		if(mLastCore->mSatellites[i].IsValid() && !AllreadyFound.Contains(mLastCore->mSatellites[i]))
		{
			UE_LOG(KBFLActorSpawnerLog, Warning, TEXT("Remove Duplicated Satellite"));
			mLastCore->mSatellites.RemoveAt(i);
		}
		else
		{
			if(mLastCore->mSatellites[i].IsValid())
				AllreadyFound.AddUnique(mLastCore->mSatellites[i].Get());
		}
	}*/
}

void UKBFLResourceNodeDescriptor_ResourceWell::ModifyCheckActor(AActor*& InActor, FTransform FoundTransform)
{
	Super::ModifyCheckActor(InActor, FoundTransform);

	class AFGResourceNodeFrackingCore* Fracking = Cast<class AFGResourceNodeFrackingCore>(InActor);
	if (Fracking)
	{
		Fracking->mResourceClass = mResourceClass;
		Fracking->InitResource(mResourceClass);
		return;
	}

	class AFGResourceNodeFrackingSatellite* Satellite = Cast<class AFGResourceNodeFrackingSatellite>(InActor);
	if (Satellite)
	{
		Satellite->mCore = mLastCore;
		Satellite->InitResource(mResourceClass, mAmount, mLastPur);
	}
}

void UKBFLResourceNodeDescriptor_ResourceWell::ModifySpawnedActorPreSpawn(AActor*& InActor)
{
	Super::ModifySpawnedActorPreSpawn(InActor);

	class AFGResourceNodeFrackingCore* Fracking = Cast<class AFGResourceNodeFrackingCore>(InActor);
	if (Fracking)
	{
		Fracking->mResourceClass = mResourceClass;
		Fracking->InitResource(mResourceClass);
	}

	class AFGResourceNodeFrackingSatellite* Satellite = Cast<class AFGResourceNodeFrackingSatellite>(InActor);
	if (Satellite)
	{
		Satellite->mCore = mLastCore;
		Satellite->InitResource(mResourceClass, mAmount, mLastPur);
	}
}

void UKBFLResourceNodeDescriptor_ResourceWell::ModifySpawnedActorPostSpawn(AActor*& InActor)
{
	Super::ModifySpawnedActorPostSpawn(InActor);
	// ModifySpawnedActorPreSpawn(InActor);
}

TArray<TEnumAsByte<EObjectTypeQuery>> UKBFLResourceNodeDescriptor_ResourceWell::GetSphereCheckChannels()
{
	return Super::GetSphereCheckChannels();
	if (bIsInCrackSpawning)
	{
		return Super::GetSphereCheckChannels();
	}
	return TArray<TEnumAsByte<EObjectTypeQuery>>{UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel3),
												 ObjectTypeQuery9};
}

void UKBFLResourceNodeDescriptor_ResourceWell::AfterSpawning()
{
	UKBFLContentCDOHelperSubsystem* Sub = UKBFLContentCDOHelperSubsystem::Get(GetWorld());
	if (Sub && IsValid(mResourceClass))
	{
		Sub->StoreClass(mResourceClass);
		// Do CDO to Activator & Extractor for new resource classes to enable the placement
		if (TSubclassOf<AFGBuildableFrackingActivator> BPBuildableFrackingActivator =
				LoadClass<AFGBuildableFrackingActivator>(nullptr,
														 TEXT("/Game/FactoryGame/Buildable/Factory/FrackingSmasher/"
															  "Build_FrackingSmasher.Build_FrackingSmasher_C")))
		{
			if (AFGBuildableFrackingActivator* FrackingActivatorDefault =
					Sub->GetAndStoreDefaultObject_Native<AFGBuildableFrackingActivator>(BPBuildableFrackingActivator))
			{
				FrackingActivatorDefault->mAllowedResources.AddUnique(mResourceClass);
			}
		}

		if (TSubclassOf<AFGBuildableFrackingExtractor> BPBuildableFrackingExtractor =
				LoadClass<AFGBuildableFrackingExtractor>(nullptr,
														 TEXT("/Game/FactoryGame/Buildable/Factory/FrackingExtractor/"
															  "Build_FrackingExtractor.Build_FrackingExtractor_C")))
		{
			if (AFGBuildableFrackingExtractor* FrackingExtractorDefault =
					Sub->GetAndStoreDefaultObject_Native<AFGBuildableFrackingExtractor>(BPBuildableFrackingExtractor))
			{
				FrackingExtractorDefault->mAllowedResources.AddUnique(mResourceClass);
			}
		}
	}
}

bool UKBFLResourceNodeDescriptor_ResourceWell::IsAllowedToRemoveActor(AActor* InActor)
{
	if (class AFGResourceNodeFrackingSatellite* Satellite = Cast<AFGResourceNodeFrackingSatellite>(InActor))
	{
		if (Satellite->mCore)
		{
			return false;
		}
		return true;
	}

	return Super::IsAllowedToRemoveActor(InActor);
}

void UKBFLResourceNodeDescriptor_ResourceWell::RemoveWrongActors(TArray<AActor*>& ActorArray)
{
	for (TActorIterator<AFGResourceNodeFrackingCore> CoreActor(GetWorld(), mFrackingCoreClass); CoreActor; ++CoreActor)
	{
		const bool IsActorCorrect = ActorArray.Contains(*CoreActor);

		if (!IsActorCorrect)
		{
			UE_LOG(KBFLActorSpawnerLog, Warning, TEXT("Core pending to remove! > %s "), *CoreActor->GetName());
			if (IsAllowedToRemoveActor(*CoreActor))
			{
				TArray<AFGResourceNodeFrackingSatellite*> Satellites;
				CoreActor->GetSatellites(Satellites);
				for (AFGResourceNodeFrackingSatellite* Sat : Satellites)
				{
					if (Sat)
					{
						Sat->K2_DestroyActor();
					}
				}

				UE_LOG(KBFLActorSpawnerLog, Log, TEXT("Remove Core > %s at %s"), *CoreActor->GetName(),
					   *CoreActor->GetActorLocation().ToString());
				CoreActor->K2_DestroyActor();
			}
		}
	}

	for (TActorIterator<AFGResourceNodeFrackingSatellite> SatelliteActor(GetWorld(), mFrackingSatelliteClass);
		 SatelliteActor; ++SatelliteActor)
	{
		const bool IsActorCorrect = ActorArray.Contains(*SatelliteActor);

		if (!IsActorCorrect)
		{
			if (IsAllowedToRemoveActor(*SatelliteActor))
			{
				if (SatelliteActor->mCore)
				{
					if (SatelliteActor->mCore->mSatellites.Contains(*SatelliteActor))
					{
						SatelliteActor->mCore->mSatellites.Remove(*SatelliteActor);
					}
				}
				SatelliteActor->Destroy();
			}
		}
		else if (!SatelliteActor->mCore)
		{
			UE_LOG(KBFLActorSpawnerLog, Log, TEXT("Remove Actors > InValidCore!!! > %s"), *SatelliteActor->GetName());
			SatelliteActor->Destroy();
		}
	}
}
