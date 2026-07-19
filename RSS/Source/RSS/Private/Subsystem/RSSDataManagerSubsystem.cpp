// Fill out your copyright notice in the Description page of Project Settings.
#include "Subsystem/RSSDataManagerSubsystem.h"

#include "Async/Async.h"
#include "BFL/KBFL_Player.h"
#include "Buildable/RSSSignRCO.h"
#include "FGGameState.h"
#include "FGVehicle.h"
#include "Interface/RssSignInterface.h"
#include "Kismet/KismetSystemLibrary.h"
#include "RssBlueprintFunctionLibrary.h"
#include "Subsystem/RSSImageSubsystem.h"
#include "Widget/RssWidgetRenderComponent.h"

// ========== FRssComponentPoolBucket Implementation ==========

void FRssComponentPoolBucket::ReturnComponent(URssWidgetRenderComponent* Component, int32 MaxPoolSize)
{
	if (!Component)
	{
		return;
	}

	FScopeLock Lock(&mBucketLock);

	TWeakObjectPtr<URssWidgetRenderComponent> ComponentPtr(Component);

	// Remove from active set
	mActiveSet.Remove(ComponentPtr);

	// Check if total created components would exceed MaxPoolSize
	// We should never destroy components if we're under MaxPoolSize total
	if (mTotalCreated > MaxPoolSize)
	{
		// We have more components than MaxPoolSize allows - destroy this one
		Component->DestroyComponent();
		mTotalCreated--;

		UE_LOG(LogRSS, Warning, TEXT("RssComponentPoolBucket: Destroyed component - exceeds MaxPoolSize (%d/%d)"),
		       mTotalCreated + 1, MaxPoolSize);
		return;
	}

	// Count valid components in available queue and clean up invalid ones
	int32 ValidAvailableCount = 0;
	TWeakObjectPtr<URssWidgetRenderComponent> TempPtr;
	TQueue<TWeakObjectPtr<URssWidgetRenderComponent>> TempQueue;

	while (mAvailableQueue.Dequeue(TempPtr))
	{
		if (TempPtr.IsValid())
		{
			ValidAvailableCount++;
			TempQueue.Enqueue(TempPtr);
		}
		// Skip invalid pointers - don't re-enqueue them
	}

	// Restore queue with only valid components
	while (TempQueue.Dequeue(TempPtr))
	{
		mAvailableQueue.Enqueue(TempPtr);
	}

	// Add component back to available pool
	mAvailableQueue.Enqueue(ComponentPtr);
}

void FRssComponentPoolBucket::TickActiveComponents(float DeltaTime)
{
	FScopeLock Lock(&mBucketLock);

	// Create a copy of active set to avoid issues if components are returned during tick
	TArray<TWeakObjectPtr<URssWidgetRenderComponent>> ActiveComponents = mActiveSet.Array();

	for (const TWeakObjectPtr<URssWidgetRenderComponent>& ComponentPtr : ActiveComponents)
	{
		if (ComponentPtr.IsValid())
		{
			ComponentPtr->PoolTick(DeltaTime);
		}
	}
}

void FRssComponentPoolBucket::GetStats(int32& OutActive, int32& OutAvailable, int32& OutTotal, int32& OutHits,
                                       int32& OutMisses) const
{
	FScopeLock Lock(&const_cast<FCriticalSection&>(mBucketLock));

	OutActive = 0;
	for (const TWeakObjectPtr<URssWidgetRenderComponent>& Ptr : mActiveSet)
	{
		if (Ptr.IsValid())
		{
			OutActive++;
		}
	}

	// Count available components without copying the queue
	// We can't copy TQueue, so we just report the total created minus active
	OutAvailable = 0;
	TWeakObjectPtr<URssWidgetRenderComponent> TempPtr;
	TQueue<TWeakObjectPtr<URssWidgetRenderComponent>> CountQueue;

	// Drain the queue to count, then refill it
	while (const_cast<TQueue<TWeakObjectPtr<URssWidgetRenderComponent>>&>(mAvailableQueue).Dequeue(TempPtr))
	{
		if (TempPtr.IsValid())
		{
			OutAvailable++;
		}
		CountQueue.Enqueue(TempPtr);
	}

	// Restore the queue
	while (CountQueue.Dequeue(TempPtr))
	{
		const_cast<TQueue<TWeakObjectPtr<URssWidgetRenderComponent>>&>(mAvailableQueue).Enqueue(TempPtr);
	}

	OutTotal = mTotalCreated;
	OutHits = mPoolHits;
	OutMisses = mPoolMisses;
}

void FRssComponentPoolBucket::Cleanup()
{
	FScopeLock Lock(&mBucketLock);

	// Destroy all active components
	for (const TWeakObjectPtr<URssWidgetRenderComponent>& ComponentPtr : mActiveSet)
	{
		if (ComponentPtr.IsValid())
		{
			ComponentPtr->DestroyComponent();
		}
	}
	mActiveSet.Empty();

	// Destroy all available components
	TWeakObjectPtr<URssWidgetRenderComponent> ComponentPtr;
	while (mAvailableQueue.Dequeue(ComponentPtr))
	{
		if (ComponentPtr.IsValid())
		{
			ComponentPtr->DestroyComponent();
		}
	}

	mTotalCreated = 0;
	mPoolHits = 0;
	mPoolMisses = 0;

	UE_LOG(LogRSS, Warning, TEXT("RssComponentPoolBucket: Cleaned up pool for class %s"),
	       mComponentClass ? *mComponentClass->GetName() : TEXT("Unknown"));
}

void FRssComponentPoolBucket::InitializePool(AActor* SubsystemOwner, int32 MinPoolSize,
                                             TArray<TObjectPtr<URssWidgetRenderComponent>>& OutAllComponents)
{
	if (!SubsystemOwner || !mComponentClass || MinPoolSize <= 0)
	{
		return;
	}

	FScopeLock Lock(&mBucketLock);

	UE_LOG(LogRSS, Log, TEXT("RssComponentPoolBucket: Initializing pool for class %s with %d components"),
	       *mComponentClass->GetName(), MinPoolSize);

	for (int32 i = 0; i < MinPoolSize; i++)
	{
		// Create component on game thread
		URssWidgetRenderComponent* NewComponent =
			NewObject<URssWidgetRenderComponent>(SubsystemOwner, mComponentClass, NAME_None, RF_Transient);

		if (NewComponent)
		{
			// Disable component ticking - subsystem will handle it
			NewComponent->PrimaryComponentTick.bCanEverTick = false;
			NewComponent->SetComponentTickEnabled(false);

			// Add to subsystem's GC-protected array
			OutAllComponents.Add(NewComponent);

			// Add to available queue
			mAvailableQueue.Enqueue(TWeakObjectPtr<URssWidgetRenderComponent>(NewComponent));
			NewComponent->RegisterComponentWithWorld(SubsystemOwner->GetWorld());

			mTotalCreated++;
		}
	}

	UE_LOG(LogRSS, Log, TEXT("RssComponentPoolBucket: Pool initialized with %d components"), mTotalCreated);
}

URssWidgetRenderComponent*
FRssComponentPoolBucket::AcquireComponent(AActor* SubsystemOwner, int32 MaxPoolSize,
                                          TArray<TObjectPtr<URssWidgetRenderComponent>>& OutAllComponents,
                                          bool bForceLoad)
{
	FScopeLock Lock(&mBucketLock);

	TWeakObjectPtr<URssWidgetRenderComponent> ComponentPtr;

	// Try to dequeue from available pool
	if (mAvailableQueue.Dequeue(ComponentPtr) && ComponentPtr.IsValid())
	{
		URssWidgetRenderComponent* Component = ComponentPtr.Get();
		mActiveSet.Add(ComponentPtr);
		mPoolHits++;
		UE_LOG(LogRSS, Warning, TEXT("RssComponentPoolBucket: Reused component from pool (Hits: %d)"), mPoolHits);
		return Component;
	}

	// Pool miss - need to create new component
	mPoolMisses++;

	if (!mComponentClass || !SubsystemOwner)
	{
		UE_LOG(LogRSS, Error, TEXT("RssComponentPoolBucket: Cannot create component - invalid class or world!"));
		return nullptr;
	}

	// Check if we've reached MaxPoolSize (unless forcing load)
	if (!bForceLoad && mTotalCreated >= MaxPoolSize)
	{
		UE_LOG(LogRSS, VeryVerbose,
		       TEXT("RssComponentPoolBucket: Cannot create new component - MaxPoolSize (%d) reached!"), MaxPoolSize);
		return nullptr;
	}

	// Create component on game thread
	URssWidgetRenderComponent* NewComponent =
		NewObject<URssWidgetRenderComponent>(SubsystemOwner, mComponentClass, NAME_None, RF_Transient);

	if (NewComponent)
	{
		// Disable component ticking - subsystem will handle it
		NewComponent->PrimaryComponentTick.bCanEverTick = false;
		NewComponent->SetComponentTickEnabled(false);
		NewComponent->RegisterComponentWithWorld(SubsystemOwner->GetWorld());

		// Add to subsystem's GC-protected array
		OutAllComponents.Add(NewComponent);

		mTotalCreated++;
		mActiveSet.Add(NewComponent);

		if (bForceLoad)
		{
			UE_LOG(LogRSS, Warning,
			       TEXT("RssComponentPoolBucket: FORCE created component of class %s (Total: %d/%d, Misses: %d)"),
			       *mComponentClass->GetName(), mTotalCreated, MaxPoolSize, mPoolMisses);
		}
		else
		{
			UE_LOG(LogRSS, Log,
			       TEXT("RssComponentPoolBucket: Created new component of class %s (Total: %d/%d, Misses: %d)"),
			       *mComponentClass->GetName(), mTotalCreated, MaxPoolSize, mPoolMisses);
		}
	}

	return NewComponent;
}

// ========== ARssDataManagerSubsystem Implementation ==========

// Sets default values
ARssDataManagerSubsystem::ARssDataManagerSubsystem()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnOnServer_Replicate;
}

ARssDataManagerSubsystem* ARssDataManagerSubsystem::GetRSSDataManagerSubsystem(UObject* WorldContext)
{
	return UKBFL_Util::GetSubsystem<ARssDataManagerSubsystem>(WorldContext);
}

void ARssDataManagerSubsystem::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	CheckKeys();

	// Update visible components async
	mFrustumCheckAccumulator += DeltaSeconds;
	if (mFrustumCheckAccumulator >= mFrustumCheckInterval && !bFrustumCheckInProgress)
	{
		mFrustumCheckAccumulator = 0.0f;
		UpdateVisibleComponentsAsync();
	}

	// Process pending component requests BEFORE ticking active components
	if (bEnableComponentPooling && mComponentRequestProcessTimer.Tick(DeltaSeconds))
	{
		ProcessPendingComponentRequests();
	}

	// Tick all active components in all pool buckets (only visible ones)
	if (bEnableComponentPooling && mSignFPS.Tick(DeltaSeconds))
	{
		TickAllActiveComponents(DeltaSeconds);
	}

	// Stats logging every 5 seconds
	if (bEnablePoolStats && mPoolStatsLogInterval.Tick(DeltaSeconds))
	{
		LogPoolStats();
	}

	if (mTimer.Tick(DeltaSeconds))
	{
		CheckCopy();
	}
}

void ARssDataManagerSubsystem::BeginPlay()
{
	Super::BeginPlay();

	if (IsValid(mModConfig))
	{
		LoadConfig();
	}
}

void ARssDataManagerSubsystem::LoadConfig()
{
	bEnableComponentPooling = UKBFL_ConfigTools::GetBoolFromConfig(mModConfig, TEXT("UsePoolingSystem"), GetWorld());
	bEnablePoolStats = UKBFL_ConfigTools::GetBoolFromConfig(mModConfig, TEXT("Debug"), GetWorld());
	mMaxPoolSize = UKBFL_ConfigTools::GetIntFromConfig(mModConfig, TEXT("MaxPoolSize"), GetWorld());
	mMinPoolSize = UKBFL_ConfigTools::GetIntFromConfig(mModConfig, TEXT("MinPoolSize"), GetWorld());
	mSignRenderingDistance = UKBFL_ConfigTools::GetIntFromConfig(mModConfig, TEXT("RenderingRange"), GetWorld());

	mMaxPoolSize = FMath::Max(mMaxPoolSize, 50);
	mMinPoolSize = FMath::Max(mMinPoolSize, 50);

	mMaxPoolSize = FMath::Max(mMaxPoolSize, mMinPoolSize);

	mMaxPoolSize = FMath::Clamp(mMaxPoolSize, 50, 15000);
	mMinPoolSize = FMath::Clamp(mMinPoolSize, 50, 15000);
	mSignRenderingDistance = FMath::Clamp(mSignRenderingDistance, 100, 2000);
}

void ARssDataManagerSubsystem::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CleanupComponentPools();
	Super::EndPlay(EndPlayReason);
}

bool ARssDataManagerSubsystem::AcquirePooledComponent(AActor* InOwner,
                                                      TSubclassOf<URssWidgetRenderComponent> ComponentClass)
{
	if (!bEnableComponentPooling || !ComponentClass || !InOwner)
	{
		UE_LOG(LogRSS, Warning, TEXT("RSSDataManagerSubsystem: Cannot acquire component - invalid parameters"));
		return false;
	}
	if (HasActorAlreadyAssignedComponent(InOwner))
	{
		return true;
	}

	// Check if actor implements significance interface
	if (!UKismetSystemLibrary::DoesImplementInterface(InOwner, UFGSignificanceInterface::StaticClass()))
	{
		UE_LOG(LogRSS, Warning,
		       TEXT("RSSDataManagerSubsystem: Actor %s does not implement IFGSignificanceInterface - cannot queue"),
		       *InOwner->GetName());
		return false;
	}

	FScopeLock QueueLock(&mPendingQueueLock);

	// Check for duplicates - avoid adding same actor multiple times
	if (mPendingRequestClasses.Contains(InOwner))
	{
		if (bEnablePoolStats)
		{
			UE_LOG(LogRSS, Warning, TEXT("RSSDataManagerSubsystem: Actor %s already in pending queue - skipping"),
			       *InOwner->GetName());
		}
		return false;
	}

	// Add to pending queue
	mPendingComponentRequests.AddObject(InOwner);
	mPendingRequestClasses.Add(InOwner, ComponentClass);

	// Set sort reference to player and sort queue by distance
	if (AFGCharacterPlayer* Player = UKBFL_Player::GetFGCharacter(this))
	{
		mPendingComponentRequests.SetSortRef(Player);
		mPendingComponentRequests.Sort();
	}

	if (bEnablePoolStats)
	{
		int32 QueueSize = mPendingComponentRequests.Num();
		UE_LOG(LogRSS, Log, TEXT("RSSDataManagerSubsystem: Added actor %s to pending queue (Queue size: %d)"),
		       *InOwner->GetName(), QueueSize);

		if (QueueSize > 100)
		{
			UE_LOG(LogRSS, Warning, TEXT("RSSDataManagerSubsystem: Large pending queue detected (%d actors)"),
			       QueueSize);
		}
	}

	// Always return nullptr - component will be created asynchronously via queue processing
	return true;
}

void ARssDataManagerSubsystem::ReturnPooledComponent(URssWidgetRenderComponent* Component)
{
	if (!Component)
	{
		return;
	}

	if (!bEnableComponentPooling)
	{
		Component->DestroyComponent();
		return;
	}

	TSubclassOf<URssWidgetRenderComponent> ComponentClass = Component->GetClass();

	FScopeLock MapLock(&mPoolMapLock);

	TSharedPtr<FRssComponentPoolBucket>* BucketPtr = mComponentPoolBuckets.Find(ComponentClass);
	if (!BucketPtr || !BucketPtr->IsValid())
	{
		UE_LOG(LogRSS, Warning, TEXT("RSSDataManagerSubsystem: No bucket found for component class %s - destroying"),
		       *ComponentClass->GetName());
		Component->DestroyComponent();
		return;
	}

	TSharedPtr<FRssComponentPoolBucket> Bucket = *BucketPtr;

	// Remove from visible components list
	{
		FScopeLock VisLock(&mVisibilityLock);
		TWeakObjectPtr<URssWidgetRenderComponent> ComponentPtr(Component);
		mVisibleComponents.Remove(ComponentPtr);
	}

	// Reset component for pooling
	Component->ResetForPooling();

	AActor* OldOwner = Component->GetOwner();
	MoveComponentToActor(Component, OldOwner, this);

	// Return to bucket
	Bucket->ReturnComponent(Component, mMaxPoolSize);

	UE_LOG(LogRSS, Warning, TEXT("RSSDataManagerSubsystem: Returned component of class %s to pool"),
	       *ComponentClass->GetName());
}

void ARssDataManagerSubsystem::TickAllActiveComponents(float DeltaTime)
{
	FScopeLock MapLock(&mPoolMapLock);

	for (auto& Pair : mComponentPoolBuckets)
	{
		if (Pair.Value.IsValid())
		{
			Pair.Value->TickActiveComponents(DeltaTime);
		}
	}
}

void ARssDataManagerSubsystem::CleanupComponentPools()
{
	FScopeLock MapLock(&mPoolMapLock);

	UE_LOG(LogRSS, Warning, TEXT("RSSDataManagerSubsystem: Cleaning up component pools..."));

	// Clean up pending queue first
	{
		FScopeLock QueueLock(&mPendingQueueLock);
		int32 PendingCount = mPendingComponentRequests.Num();
		mPendingComponentRequests.Empty();
		mPendingRequestClasses.Empty();

		if (PendingCount > 0)
		{
			UE_LOG(LogRSS, Warning, TEXT("RSSDataManagerSubsystem: Cleared %d pending component requests"),
			       PendingCount);
		}
	}

	// Clean up component pool buckets
	for (auto& Pair : mComponentPoolBuckets)
	{
		if (Pair.Value.IsValid())
		{
			Pair.Value->Cleanup();
		}
	}

	mComponentPoolBuckets.Empty();

	// Clear the GC-protected array - this allows components to be garbage collected
	mAllPooledComponents.Empty();

	UE_LOG(LogRSS, Warning, TEXT("RSSDataManagerSubsystem: Component pools cleaned up"));
}

void ARssDataManagerSubsystem::LogPoolStats()
{
	if (!bEnablePoolStats)
	{
		return;
	}

	FScopeLock MapLock(&mPoolMapLock);

	UE_LOG(LogRSS, Warning, TEXT("========== RSS Component Pool Statistics =========="));

	int32 TotalActive = 0;
	int32 TotalAvailable = 0;
	int32 mTotalCreated = 0;
	int32 TotalHits = 0;
	int32 TotalMisses = 0;

	for (const auto& Pair : mComponentPoolBuckets)
	{
		if (!Pair.Value.IsValid())
		{
			continue;
		}

		int32 Active, Available, Created, Hits, Misses;
		Pair.Value->GetStats(Active, Available, Created, Hits, Misses);

		TotalActive += Active;
		TotalAvailable += Available;
		mTotalCreated += Created;
		TotalHits += Hits;
		TotalMisses += Misses;

		float HitRate = (Hits + Misses > 0) ? (static_cast<float>(Hits) / (Hits + Misses) * 100.0f) : 0.0f;

		UE_LOG(LogRSS, Warning, TEXT("  Class: %s"), *Pair.Key->GetName());
		UE_LOG(LogRSS, Warning, TEXT("    Active: %d, Available: %d, Total Created: %d"), Active, Available, Created);
		UE_LOG(LogRSS, Warning, TEXT("    Hits: %d, Misses: %d, Hit Rate: %.1f%%"), Hits, Misses, HitRate);
	}

	float OverallHitRate =
		(TotalHits + TotalMisses > 0) ? (static_cast<float>(TotalHits) / (TotalHits + TotalMisses) * 100.0f) : 0.0f;

	UE_LOG(LogRSS, Warning, TEXT("  TOTAL - Active: %d, Available: %d, Created: %d"), TotalActive, TotalAvailable,
	       mTotalCreated);
	UE_LOG(LogRSS, Warning, TEXT("  TOTAL - Hits: %d, Misses: %d, Hit Rate: %.1f%%"), TotalHits, TotalMisses,
	       OverallHitRate);

	// Add pending queue statistics
	{
		FScopeLock QueueLock(&mPendingQueueLock);
		int32 PendingQueueSize = mPendingComponentRequests.Num();
		int32 PendingMapSize = mPendingRequestClasses.Num();

		UE_LOG(LogRSS, Warning, TEXT("  ----- Pending Queue Statistics -----"));
		UE_LOG(LogRSS, Warning, TEXT("  Pending Requests: %d (Map Size: %d)"), PendingQueueSize, PendingMapSize);

		if (PendingQueueSize > 100)
		{
			UE_LOG(LogRSS, Warning, TEXT("  WARNING: Large pending queue detected (%d actors waiting)"),
			       PendingQueueSize);
		}

		if (PendingQueueSize > 1000)
		{
			UE_LOG(LogRSS, Warning,
			       TEXT("  PERFORMANCE: Very large queue (%d actors) - using optimized sort frequency"),
			       PendingQueueSize);
		}
	}

	UE_LOG(LogRSS, Warning, TEXT("===================================================="));
}

bool ARssDataManagerSubsystem::HasActorAlreadyAssignedComponent(AActor* Actor) const
{
	if (!IsValid(Actor))
	{
		return true;
	}

	URssWidgetRenderComponent* ExistingComponent = Actor->FindComponentByClass<URssWidgetRenderComponent>();

	return IsValid(ExistingComponent);
}

void ARssDataManagerSubsystem::ForceLoadSign(AActor* SignActor)
{
	if (!SignActor)
	{
		UE_LOG(LogRSS, Warning, TEXT("RSSDataManagerSubsystem::ForceLoadSign - Invalid SignActor!"));
		return;
	}

	// Check if sign implements the interface
	if (!UKismetSystemLibrary::DoesImplementInterface(SignActor, URssSignInterface::StaticClass()))
	{
		UE_LOG(LogRSS, Warning,
		       TEXT("RSSDataManagerSubsystem::ForceLoadSign - SignActor does not implement IRssSignInterface!"));
		return;
	}

	// Check if component already exists
	URssWidgetRenderComponent* ExistingComponent = SignActor->FindComponentByClass<URssWidgetRenderComponent>();
	if (IsValid(ExistingComponent))
	{
		ReturnPooledComponent(ExistingComponent);

		UE_LOG(LogRSS, Warning,
		       TEXT("RSSDataManagerSubsystem::ForceLoadSign - SignActor already has a component - returning to pool "
			       "first."));
	}

	// Remove from pending queue if it's there
	RemoveFromPendingQueue(SignActor);

	// Get component class
	TSubclassOf<URssWidgetRenderComponent> ComponentClass =
		IRssSignInterface::Execute_GetWidgetRenderComponentClass(SignActor);
	if (!ComponentClass)
	{
		UE_LOG(LogRSS, Error, TEXT("RSSDataManagerSubsystem::ForceLoadSign - No component class found!"));
		return;
	}

	// Get or create bucket for this component class
	FScopeLock MapLock(&mPoolMapLock);

	TSharedPtr<FRssComponentPoolBucket>* BucketPtr = mComponentPoolBuckets.Find(ComponentClass);
	TSharedPtr<FRssComponentPoolBucket> Bucket;

	if (!BucketPtr)
	{
		// Create new bucket for this class
		Bucket = MakeShared<FRssComponentPoolBucket>(ComponentClass);
		mComponentPoolBuckets.Add(ComponentClass, Bucket);
		Bucket->InitializePool(this, mMinPoolSize, mAllPooledComponents);
	}
	else
	{
		Bucket = *BucketPtr;
	}

	if (!Bucket.IsValid())
	{
		UE_LOG(LogRSS, Error, TEXT("RSSDataManagerSubsystem::ForceLoadSign - Failed to get bucket!"));
		return;
	}

	// Force acquire component (bypassing MaxPoolSize limit)
	URssWidgetRenderComponent* Component = Bucket->AcquireComponent(this, mMaxPoolSize, mAllPooledComponents, true);

	if (Component)
	{
		CreateComponentForActor(SignActor, Component);
		UE_LOG(LogRSS, Warning, TEXT("RSSDataManagerSubsystem::ForceLoadSign - Force loaded sign: %s"),
		       *SignActor->GetName());
	}
	else
	{
		UE_LOG(LogRSS, Error, TEXT("RSSDataManagerSubsystem::ForceLoadSign - Failed to acquire component!"));
	}
}

void ARssDataManagerSubsystem::ProcessPendingComponentRequests()
{
	FScopeLock QueueLock(&mPendingQueueLock);

	// Get player for distance sorting
	AFGCharacterPlayer* Player = UKBFL_Player::GetFGCharacter(this);
	if (!Player)
	{
		// No player available - skip processing this tick
		return;
	}

	int32 QueueSize = mPendingComponentRequests.Num();
	if (QueueSize == 0)
	{
		return;
	}

	// Performance optimization: only sort every 3rd tick for large queues
	bool bShouldSort = true;
	if (QueueSize > 1000)
	{
		mSortTickCounter++;
		if (mSortTickCounter % 3 != 0)
		{
			bShouldSort = false;
		}
	}

	if (bShouldSort)
	{
		mPendingComponentRequests.SetSortRef(Player);
		mPendingComponentRequests.Sort();
	}

	// Process up to mMaxComponentRequestsPerTick actors
	int32 ProcessedCount = 0;
	int32 SuccessCount = 0;
	int32 RemovedCount = 0;

	for (int32 i = 0; i < mMaxComponentRequestsPerTick && !mPendingComponentRequests.IsEmpty(); i++)
	{
		AActor* Actor = mPendingComponentRequests.PopClosestValid();
		if (!Actor)
		{
			continue;
		}

		ProcessedCount++;

		// Check if actor is still significant
		if (!UKismetSystemLibrary::DoesImplementInterface(Actor, UFGSignificanceInterface::StaticClass()) ||
			!UKismetSystemLibrary::DoesImplementInterface(Actor, URssSignInterface::StaticClass()) ||
			!IRssSignInterface::Execute_IsBuildingSignificance(Actor))
		{
			if (bEnablePoolStats)
			{
				UE_LOG(LogRSS, Error,
				       TEXT("RSSDataManagerSubsystem: Actor %s is no longer significant - removing from queue"),
				       *Actor->GetName());
			}

			// Actor lost significance interface - remove from queue
			mPendingRequestClasses.Remove(Actor);
			RemovedCount++;
			continue;
		}

		if (AFGVehicle* Vehicle = Cast<AFGVehicle>(Actor))
		{
			// If buildable is under construction, skip it and re-add to queue
			if (Vehicle->IsPlayingBuildEffect())
			{
				if (bEnablePoolStats)
				{
					UE_LOG(LogRSS, Error,
					       TEXT("RSSDataManagerSubsystem: Actor %s is under construction - re-adding to queue"),
					       *Actor->GetName());
				}
				mPendingComponentRequests.AddObject(Actor);
				continue;
			}
		}

		if (AFGBuildable* Buildable = Cast<AFGBuildable>(Actor))
		{
			// If buildable is under construction, skip it and re-add to queue
			if (Buildable->IsPlayingBuildEffect())
			{
				if (bEnablePoolStats)
				{
					UE_LOG(LogRSS, Error,
					       TEXT("RSSDataManagerSubsystem: Actor %s is under construction - re-adding to queue"),
					       *Actor->GetName());
				}
				mPendingComponentRequests.AddObject(Actor);
				continue;
			}
		}

		// Get component class for this actor
		TSubclassOf<URssWidgetRenderComponent>* ComponentClassPtr = mPendingRequestClasses.Find(Actor);
		if (!ComponentClassPtr)
		{
			if (bEnablePoolStats)
			{
				UE_LOG(LogRSS, Error,
				       TEXT("RSSDataManagerSubsystem: No component class found in pending map for actor %s - removing"),
				       *Actor->GetName());
			}

			// No component class found - skip
			RemovedCount++;
			continue;
		}

		TSubclassOf<URssWidgetRenderComponent> ComponentClass = *ComponentClassPtr;
		if (HasActorAlreadyAssignedComponent(Actor))
		{
			mPendingRequestClasses.Remove(Actor);
			continue;
		}

		// Get or create bucket for this component class
		FScopeLock MapLock(&mPoolMapLock);

		TSharedPtr<FRssComponentPoolBucket>* BucketPtr = mComponentPoolBuckets.Find(ComponentClass);
		TSharedPtr<FRssComponentPoolBucket> Bucket;

		if (!BucketPtr)
		{
			// Create new bucket for this class
			Bucket = MakeShared<FRssComponentPoolBucket>(ComponentClass);
			mComponentPoolBuckets.Add(ComponentClass, Bucket);
			Bucket->InitializePool(this, mMinPoolSize, mAllPooledComponents);
		}
		else
		{
			Bucket = *BucketPtr;
		}

		if (!Bucket.IsValid())
		{
			// Failed to get bucket - re-add to queue for next attempt
			mPendingComponentRequests.AddObject(Actor);
			continue;
		}

		// Try to acquire component from bucket
		URssWidgetRenderComponent* Component =
			Bucket->AcquireComponent(this, mMaxPoolSize, mAllPooledComponents, false);

		if (Component)
		{
			// Successfully acquired component - create and initialize
			CreateComponentForActor(Actor, Component);

			// Remove from pending map
			mPendingRequestClasses.Remove(Actor);
			SuccessCount++;
		}
		else
		{
			// Failed to acquire - add back to queue for next tick
			mPendingComponentRequests.AddObject(Actor);
		}
	}

	if (bEnablePoolStats && ProcessedCount > 0)
	{
		UE_LOG(LogRSS, Warning,
		       TEXT("Processed %d pending component requests - Success: %d, Removed: %d, Remaining: %d"),
		       ProcessedCount, SuccessCount, RemovedCount, mPendingComponentRequests.Num());
	}
}

void ARssDataManagerSubsystem::RemoveFromPendingQueue(AActor* Actor)
{
	if (!Actor)
	{
		return;
	}

	FScopeLock QueueLock(&mPendingQueueLock);

	// Remove from map
	if (mPendingRequestClasses.Remove(Actor) > 0)
	{
		// Also remove from sorted array
		mPendingComponentRequests.RemoveObject(Actor);

		if (bEnablePoolStats)
		{
			UE_LOG(LogRSS, Log, TEXT("RSSDataManagerSubsystem: Removed actor %s from pending queue (Remaining: %d)"),
			       *Actor->GetName(), mPendingComponentRequests.Num());
		}
	}
}

void ARssDataManagerSubsystem::CreateComponentForActor(AActor* OwnerActor, URssWidgetRenderComponent* Component)
{
	if (!OwnerActor || !Component)
	{
		return;
	}

	// Move component to actor and register with owner
	MoveComponentToActor(Component, this, OwnerActor);

	// Initialize the component after registration (required for pooled components)
	Component->InitComponent();
	Component->RequestRedraw();

	if (bEnablePoolStats)
	{
		UE_LOG(LogRSS, Log, TEXT("RSSDataManagerSubsystem: Created component for actor %s"), *OwnerActor->GetName());
	}
}

void ARssDataManagerSubsystem::UpdateVisibleComponentsAsync()
{
	if (!GetWorld() || !GetWorld()->GetFirstPlayerController())
	{
		return;
	}

	bFrustumCheckInProgress = true;

	// Collect all active components from all buckets
	TArray<TWeakObjectPtr<URssWidgetRenderComponent>> AllActiveComponents;
	{
		FScopeLock MapLock(&mPoolMapLock);
		for (auto& Pair : mComponentPoolBuckets)
		{
			if (Pair.Value.IsValid())
			{
				FScopeLock BucketLock(&Pair.Value->mBucketLock);
				AllActiveComponents.Append(Pair.Value->mActiveSet.Array());
			}
		}
	}

	// Get camera info on game thread
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC || !PC->PlayerCameraManager)
	{
		bFrustumCheckInProgress = false;
		return;
	}

	FVector CameraLocation;
	FRotator CameraRotation;
	PC->PlayerCameraManager->GetCameraViewPoint(CameraLocation, CameraRotation);
	const float FOV = PC->PlayerCameraManager->GetFOVAngle();
	const FVector CameraForward = CameraRotation.Vector();
	const float FOVRadians = FMath::DegreesToRadians(FOV * 0.5f);
	const float MinDot = FMath::Cos(FOVRadians * 1.2f); // Add 20% margin

	// GetActorBounds / GetOwner are not thread-safe and caused data races in the prior implementation.
	TArray<FVector> OwnerOrigins;
	TArray<bool> ShouldCheckInView;
	OwnerOrigins.Reserve(AllActiveComponents.Num());
	ShouldCheckInView.Reserve(AllActiveComponents.Num());

	for (const TWeakObjectPtr<URssWidgetRenderComponent>& ComponentPtr : AllActiveComponents)
	{
		if (ComponentPtr.IsValid())
		{
			AActor* Own = ComponentPtr->GetOwner();
			FVector Origin, BoxExtent;
			if (Own)
			{
				Own->GetActorBounds(false, Origin, BoxExtent);
			}
			else
			{
				Origin = FVector::ZeroVector;
			}
			OwnerOrigins.Add(Origin);
			ShouldCheckInView.Add(ComponentPtr->bShouldCheckInView);
		}
		else
		{
			OwnerOrigins.Add(FVector::ZeroVector);
			ShouldCheckInView.Add(false);
		}
	}

	// Process in background thread using AsyncTask — pure math on snapshotted data, no UObject access.
	TWeakObjectPtr<ARssDataManagerSubsystem> WeakThis(this);
	AsyncTask(
		ENamedThreads::AnyBackgroundThreadNormalTask,
		[WeakThis, AllActiveComponents = MoveTemp(AllActiveComponents), OwnerOrigins = MoveTemp(OwnerOrigins),
			ShouldCheckInView = MoveTemp(ShouldCheckInView), CameraLocation, CameraForward, MinDot]()
		{
			TArray<TWeakObjectPtr<URssWidgetRenderComponent>> NewVisibleComponents;

			// Use ParallelFor for maximum performance — no UObject access on worker threads
			FCriticalSection TempLock;
			ParallelFor(AllActiveComponents.Num(),
			            [&](int32 Index)
			            {
				            const TWeakObjectPtr<URssWidgetRenderComponent>& ComponentPtr = AllActiveComponents[Index];
				            if (!ComponentPtr.IsValid())
				            {
					            return;
				            }

				            if (!ShouldCheckInView[Index])
				            {
					            FScopeLock Lock(&TempLock);
					            NewVisibleComponents.Add(ComponentPtr);
					            return;
				            }

				            // Use the pre-snapshotted origin — no game-thread data access
				            const FVector DirectionToOwner = (OwnerOrigins[Index] - CameraLocation).GetSafeNormal();
				            const float DotProduct = FVector::DotProduct(CameraForward, DirectionToOwner);

				            if (DotProduct >= MinDot)
				            {
					            FScopeLock Lock(&TempLock);
					            NewVisibleComponents.Add(ComponentPtr);
				            }
			            });

			// Update visible components list on game thread
			AsyncTask(ENamedThreads::GameThread,
			          [WeakThis, NewVisibleComponents = MoveTemp(NewVisibleComponents)]()
			          {
				          if (ARssDataManagerSubsystem* Self = WeakThis.Get())
				          {
					          FScopeLock Lock(&Self->mVisibilityLock);
					          Self->mVisibleComponents =
						          TSet<TWeakObjectPtr<URssWidgetRenderComponent>>(NewVisibleComponents);
					          Self->bFrustumCheckInProgress = false;
				          }
			          });
		});
}

bool ARssDataManagerSubsystem::IsComponentInFrustum(URssWidgetRenderComponent* Component) const
{
	if (!Component)
	{
		return false;
	}

	FScopeLock Lock(&const_cast<FCriticalSection&>(mVisibilityLock));
	return mVisibleComponents.Contains(TWeakObjectPtr<URssWidgetRenderComponent>(Component));
}

void ARssDataManagerSubsystem::CheckKeys()
{
	if (const AFGPlayerController* Controller = UKBFL_Player::GetFGController(this))
	{
		for (const FKey& Key : mKeysToCheck)
		{
			const bool bWasDown = mKeyDown.Contains(Key);
			const bool bIsDown = Controller->IsInputKeyDown(Key);
			if (!bWasDown && bIsDown)
			{
				mKeyDown.Add(Key);
				OnKeyPressed.Broadcast(Key);
			}
			else if (bWasDown && !bIsDown)
			{
				mKeyDown.Remove(Key);
				OnKeyReleased.Broadcast(Key);
			}
		}
	}
}

bool ARssDataManagerSubsystem::CheckCopy()
{
	const FString CopyString = UFGBlueprintFunctionLibrary::CopyTextFromClipboard().ToString().TrimStartAndEnd();

	bool success = false;
	if (ARSSImageSubsystem::CheckString(CopyString, {"mElements", "mSignType=RSS_", "mSignTypeSize=RSS_"}))
	{
		mCopiedSignData = StringToSignData(this, CopyString, success);
	}

	return success;
}

float ARssDataManagerSubsystem::GetSignRenderingDistance() const
{
	return mSignRenderingDistance * 100.0f; // Convert from meters to centimeters
}

void ARssDataManagerSubsystem::MoveComponentToActor(UActorComponent* Component, AActor* SourceActor,
                                                    AActor* TargetActor)
{
	// We try to get the owner from the component if no source actor is passed.
	if (!IsValid(SourceActor) && IsValid(Component))
	{
		SourceActor = Component->GetOwner();
	}

	if (!IsValid(Component) || !IsValid(SourceActor) || !IsValid(TargetActor))
	{
		return;
	}

	Component->UnregisterComponent();
	SourceActor->RemoveInstanceComponent(Component);

	Component->Rename(nullptr, TargetActor);
	Component->RegisterComponent();

	TargetActor->AddInstanceComponent(Component);
}

FString ARssDataManagerSubsystem::SignDataToString(UObject* WorldContext, FRssSignData SignData)
{
	UScriptStruct* Struct = SignData.StaticStruct();
	FString Output = TEXT("");
	FRssSignData DefaultData;
	Struct->ExportText(Output, &SignData, &DefaultData, WorldContext,
	                   (PPF_ExportsNotFullyQualified | PPF_Copy | PPF_Delimited | PPF_IncludeTransient), nullptr);
	return Output;
}

FRssSignData ARssDataManagerSubsystem::StringToSignData(UObject* WorldContext, FString SignData, bool& WasSuccess)
{
	FRssSignData OutStruc = FRssSignData();

	static UScriptStruct* Struct = OutStruc.StaticStruct();
	WasSuccess = Struct->ImportText(*SignData, &OutStruc, WorldContext,
		(PPF_Copy | PPF_Delimited | PPF_IncludeTransient), nullptr, "FRssSignData") != nullptr;

	return OutStruc;
}

FString ARssDataManagerSubsystem::CopySignDataToClipboard(FRssSignData SignData)
{
	FString SignDataString = SignDataToString(this, SignData);
	UFGBlueprintFunctionLibrary::CopyTextToClipboard(FText::FromString(SignDataString));
	mCopiedSignData = SignData;
	return SignDataString;
}

AActor* ARssDataManagerSubsystem::GetLastCopiedActor() const
{
	return mLastCopiedActor.IsValid() ? mLastCopiedActor.Get() : nullptr;
}

void ARssDataManagerSubsystem::SetLastCopiedActor(AActor* Actor) { mLastCopiedActor = Actor; }
