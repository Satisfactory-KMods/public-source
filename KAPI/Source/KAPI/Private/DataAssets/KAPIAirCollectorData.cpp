#include "DataAssets/KAPIAirCollectorData.h"

#include "FGDestructibleActor.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Resources/FGExtractableResourceInterface.h"
#include "Resources/FGResourceDescriptor.h"
#include "Resources/FGResourceNode.h"

bool UKAPIAirCollectorData::TestHit(AActor* InActor, int32& HitCount) const
{
	if (mActorClass.IsEmpty() && !bCheckNodes)
	{
		HitCount = 0;
		return false;
	}

	FVector ActorLocation = InActor->GetActorLocation();

	TArray<AActor*> ActorsToIgnore = TArray<AActor*>{InActor};
	TArray<AActor*> OutActors;

	TSubclassOf<AActor> ActorClass = AActor::StaticClass();
	if (mActorClass.Num() == 1 && IsValid(mActorClass[0]) && !bCheckNodes)
	{
		ActorClass = mActorClass[0];
	}
	else if (mActorClass.IsEmpty() && bCheckNodes)
	{
		ActorClass = AFGResourceNode::StaticClass();
	}

	HitCount = 0;
	if (UKismetSystemLibrary::SphereOverlapActors(
		InActor, ActorLocation, mRangeToScan, mTraceChannel, ActorClass, ActorsToIgnore,
		OutActors))
	{
		for (AActor* OutActor : OutActors)
		{
			if (OutActor == InActor)
			{
				continue;
			}

			if (AFGDestructibleActor* DestructibleActor = Cast<AFGDestructibleActor>(OutActor))
			{
				if (DestructibleActor->GetDestructibleActorState() != EDestructibleActorState::DSS_Intact)
				{
					continue;
				}
			}

			for (TSubclassOf<AActor> Class : mActorClass)
			{
				if (!IsValid(Class))
				{
					continue;
				}
				if (OutActor->IsA(Class) || OutActor->GetClass()->IsChildOf(Class))
				{
					HitCount++;
					if (HitCount >= mMaxHit && HitCount > 0)
					{
						return true;
					}
				}
			}

			if (bCheckNodes && OutActor->Implements<UFGExtractableResourceInterface>())
			{
				TScriptInterface<IFGExtractableResourceInterface> ExtractableResourceInterface;
				ExtractableResourceInterface.SetObject(OutActor);
				ExtractableResourceInterface.SetInterface(Cast<IFGExtractableResourceInterface>(OutActor));

				TSubclassOf<UFGResourceDescriptor> ResourceDescriptor =
					ExtractableResourceInterface->GetResourceClass();
				if (!ExtractableResourceInterface->IsOccupied() && ResourceDescriptor == mItemClass)
				{
					HitCount++;
					if (HitCount >= mMaxHit && HitCount > 0)
					{
						return true;
					}
				}
			}
		}
	}

	return HitCount > 0;
}
