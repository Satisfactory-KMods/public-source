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

	// Narrow the overlap query when possible to reduce the result set.
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

		// Skip destroyed destructible actors.
		if (const AFGDestructibleActor* DestructibleActor = Cast<AFGDestructibleActor>(OutActor))
		{
			if (DestructibleActor->GetDestructibleActorState() != EDestructibleActorState::DSS_Intact)
			{
				continue;
			}
		}

		// Each actor counts at most once. Try class-list match first; fall through to node check
		// only when the class list did not match (the two paths are mutually exclusive so a single
		// actor can never be counted twice even if it satisfies both conditions).
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
			// mMaxHit <= 0 means "no cap" — do not short-circuit.
			if (mMaxHit > 0 && HitCount >= mMaxHit)
			{
				return true;
			}
		}
	}

	return HitCount > 0;
}
