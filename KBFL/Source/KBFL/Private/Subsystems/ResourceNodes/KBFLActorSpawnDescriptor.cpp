﻿#pragma once
#include "Subsystems/ResourceNodes/KBFLActorSpawnDescriptor.h"

#include "KBFLLogging.h"

void UKBFLActorSpawnDescriptor::ForeachLocations(TArray<AActor*>& ActorArray)
{
	UE_LOGFMT(KBFLActorSpawnerLog, Log, "ForeachLocations: Processing {0} location(s) for descriptor '{1}'", 
		mLocations.Num(), GetName());

	int32 SpawnedCount = 0;
	int32 SkippedZeroCount = 0;
	int32 ExistingCount = 0;
	
	for (FTransform Location : mLocations)
	{
		if (Location.GetLocation() == FVector(0))
		{
			SkippedZeroCount++;
			continue;
		}

		UE_LOGFMT(KBFLActorSpawnerLog, Log, "Attempting to process location: {0}", Location.ToString());
		AActor* OutActor = nullptr;
		
		if (!CheckActorInRange(Location, OutActor) && IsRangeFree(Location))
		{
			UE_LOGFMT(KBFLActorSpawnerLog, Log, "Spawning new actor at location {0}", Location.GetLocation().ToString());
			AActor* NewActor = SpawnActorAtLocation(Location, GetActorClass());
			if (NewActor)
			{
				OutActor = NewActor;
				SpawnedCount++;
			}
		}
		else
		{
			if (OutActor)
			{
				UE_LOGFMT(KBFLActorSpawnerLog, Log, "Found existing actor '{0}' at location", OutActor->GetName());
				ExistingCount++;
			}
			else
			{
				UE_LOGFMT(KBFLActorSpawnerLog, Warning, "Location not free or actor exists but not found");
			}
		}

		if (OutActor)
		{
			ActorArray.AddUnique(OutActor);
		}
	}
	
	UE_LOGFMT(KBFLActorSpawnerLog, Log, 
		"ForeachLocations completed: Total: {0}, Spawned: {1}, Existing: {2}, Skipped (Zero): {3}, Final Array: {4}",
		mLocations.Num(), SpawnedCount, ExistingCount, SkippedZeroCount, ActorArray.Num());
}
