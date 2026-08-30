#include "DataAssets/KAPIAirCollectorData.h"

#include "FGDestructibleActor.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Resources/FGExtractableResourceInterface.h"
#include "Resources/FGResourceDescriptor.h"
#include "Resources/FGResourceNode.h"

bool UKAPIAirCollectorData::TestHit(AActor* InActor, int32& HitCount) const
{
	if (!IsValid(InActor))
	{
		HitCount = 0;
		return false;
	}

	if (mActorClass.IsEmpty() && !bCheckNodes)
	{
		HitCount = 0;
		return false;
	}

	const FVector ActorLocation = InActor->GetActorLocation();

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
	if (!UKismetSystemLibrary::SphereOverlapActors(InActor, ActorLocation, mRangeToScan, mTraceChannel, ActorClass,
												   ActorsToIgnore, OutActors))
	{
		return false;
	}

	for (AActor* OutActor : OutActors)
	{
		if (OutActor == InActor)
		{
			continue;
		}

		if (const AFGDestructibleActor* DestructibleActor = Cast<AFGDestructibleActor>(OutActor))
		{
			if (DestructibleActor->GetDestructibleActorState() != EDestructibleActorState::DSS_Intact)
			{
				continue;
			}
		}

		bool bMatched = false;

		for (const TSubclassOf<AActor>& Class : mActorClass)
		{
			if (!IsValid(Class))
			{
				continue;
			}
			if (OutActor->IsA(Class))
			{
				bMatched = true;
				break;
			}
		}

		if (!bMatched && bCheckNodes && OutActor->Implements<UFGExtractableResourceInterface>())
		{
			TScriptInterface<IFGExtractableResourceInterface> ExtractableResourceInterface;
			ExtractableResourceInterface.SetObject(OutActor);
			ExtractableResourceInterface.SetInterface(Cast<IFGExtractableResourceInterface>(OutActor));

			const TSubclassOf<UFGResourceDescriptor> ResourceDescriptor =
				ExtractableResourceInterface->GetResourceClass();
			if (!ExtractableResourceInterface->IsOccupied() && ResourceDescriptor == mItemClass)
			{
				bMatched = true;
			}
		}

		if (bMatched)
		{
			++HitCount;

			if (mMaxHit > 0 && HitCount >= mMaxHit)
			{
				return true;
			}
		}
	}

	return HitCount > 0;
}
