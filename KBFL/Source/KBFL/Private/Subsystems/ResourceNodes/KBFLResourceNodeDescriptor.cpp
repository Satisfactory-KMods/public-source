﻿#pragma once
#include "Subsystems/ResourceNodes/KBFLResourceNodeDescriptor.h"
#include "KBFLLogging.h"

bool UKBFLResourceNodeDescriptor::IsAllowedToRemoveActor(AActor* InActor)
{
	const AFGResourceNodeBase* ResourceNode = Cast<AFGResourceNodeBase>(InActor);
	if (ResourceNode && !mRemoveOccupied)
	{
		if (ResourceNode->IsOccupied())
		{
			UE_LOGFMT(KBFLActorSpawnerLog, Log, "IsAllowedToRemoveActor: '{0}' is occupied, preventing removal", 
				InActor->GetName());
			return false;
		}
	}
	
	UE_LOGFMT(KBFLActorSpawnerLog, Log, "IsAllowedToRemoveActor: '{0}' allowed to be removed", 
		InActor->GetName());
	return true;
}
