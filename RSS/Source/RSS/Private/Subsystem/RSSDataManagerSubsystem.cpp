
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

void FRssComponentPoolBucket::ReturnComponent(URssWidgetRenderComponent* Component, int32 MaxPoolSize)
{
	if (!Component)
	{
		return;
	}

	FScopeLock Lock(&mBucketLock);

	TWeakObjectPtr<URssWidgetRenderComponent> ComponentPtr(Component);

	mActiveSet.Remove(ComponentPtr);

	if (mTotalCreated > MaxPoolSize)
	{

		Component->DestroyComponent();
		mTotalCreated--;

		UE_LOG(LogRSS, Warning, TEXT("RssComponentPoolBucket: Destroyed component - exceeds MaxPoolSize (%d/%d)"),
		       mTotalCreated + 1, MaxPoolSize);
		return;
	}

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

	}

	while (TempQueue.Dequeue(TempPtr))
	{
		mAvailableQueue.Enqueue(TempPtr);
	}

	mAvailableQueue.Enqueue(ComponentPtr);
}

void FRssComponentPoolBucket::TickActiveComponents(float DeltaTime)
{
	FScopeLock Lock(&mBucketLock);

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

	OutAvailable = 0;
	TWeakObjectPtr<URssWidgetRenderComponent> TempPtr;
	TQueue<TWeakObjectPtr<URssWidgetRenderComponent>> CountQueue;

	while (const_cast<TQueue<TWeakObjectPtr<URssWidgetRenderComponent>>&>(mAvailableQueue).Dequeue(TempPtr))
	{
		if (TempPtr.IsValid())
		{
			OutAvailable++;
		}
		CountQueue.Enqueue(TempPtr);
	}

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

	for (const TWeakObjectPtr<URssWidgetRenderComponent>& ComponentPtr : mActiveSet)
	{
		if (ComponentPtr.IsValid())
		{
			ComponentPtr->DestroyComponent();
		}
	}
	mActiveSet.Empty();

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

		URssWidgetRenderComponent* NewComponent =
			NewObject<URssWidgetRenderComponent>(SubsystemOwner, mComponentClass, NAME_None, RF_Transient);

		if (NewComponent)
		{

			NewComponent->PrimaryComponentTick.bCanEverTick = false;
			NewComponent->SetComponentTickEnabled(false);

			OutAllComponents.Add(NewComponent);

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

	if (mAvailableQueue.Dequeue(ComponentPtr) && ComponentPtr.IsValid())
	{
		URssWidgetRenderComponent* Component = ComponentPtr.Get();
		mActiveSet.Add(ComponentPtr);
		mPoolHits++;
		UE_LOG(LogRSS, Warning, TEXT("RssComponentPoolBucket: Reused component from pool (Hits: %d)"), mPoolHits);
		return Component;
	}

	mPoolMisses++;

	if (!mComponentClass || !SubsystemOwner)
	{
		UE_LOG(LogRSS, Error, TEXT("RssComponentPoolBucket: Cannot create component - invalid class or world!"));
		return nullptr;
	}

	if (!bForceLoad && mTotalCreated >= MaxPoolSize)
	{
		UE_LOG(LogRSS, VeryVerbose,
		       TEXT("RssComponentPoolBucket: Cannot create new component - MaxPoolSize (%d) reached!"), MaxPoolSize);
		return nullptr;
	}

	URssWidgetRenderComponent* NewComponent =
		NewObject<URssWidgetRenderComponent>(SubsystemOwner, mComponentClass, NAME_None, RF_Transient);

	if (NewComponent)
	{

		NewComponent->PrimaryComponentTick.bCanEverTick = false;
		NewComponent->SetComponentTickEnabled(false);
		NewComponent->RegisterComponentWithWorld(SubsystemOwner->GetWorld());

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

ARssDataManagerSubsystem::ARssDataManagerSubsystem()
{

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

	mFrustumCheckAccumulator += DeltaSeconds;
	if (mFrustumCheckAccumulator >= mFrustumCheckInterval && !bFrustumCheckInProgress)
	{
		mFrustumCheckAccumulator = 0.0f;
		UpdateVisibleComponentsAsync();
	}

	if (bEnableComponentPooling && mComponentRequestProcessTimer.Tick(DeltaSeconds))
	{
		ProcessPendingComponentRequests();
	}

	if (bEnableComponentPooling && mSignFPS.Tick(DeltaSeconds))
	{
		TickAllActiveComponents(DeltaSeconds);
	}

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

	if (!UKismetSystemLibrary::DoesImplementInterface(InOwner, UFGSignificanceInterface::StaticClass()))
	{
		UE_LOG(LogRSS, Warning,
		       TEXT("RSSDataManagerSubsystem: Actor %s does not implement IFGSignificanceInterface - cannot queue"),
		       *InOwner->GetName());
		return false;
	}

	FScopeLock QueueLock(&mPendingQueueLock);

	if (mPendingRequestClasses.Contains(InOwner))
	{
		if (bEnablePoolStats)
		{
			UE_LOG(LogRSS, Warning, TEXT("RSSDataManagerSubsystem: Actor %s already in pending queue - skipping"),
			       *InOwner->GetName());
		}
		return false;
	}

	mPendingComponentRequests.AddObject(InOwner);
	mPendingRequestClasses.Add(InOwner, ComponentClass);

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

	{
		FScopeLock VisLock(&mVisibilityLock);
		TWeakObjectPtr<URssWidgetRenderComponent> ComponentPtr(Component);
		mVisibleComponents.Remove(ComponentPtr);
	}

	Component->ResetForPooling();

	AActor* OldOwner = Component->GetOwner();
	MoveComponentToActor(Component, OldOwner, this);

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

	for (auto& Pair : mComponentPoolBuckets)
	{
		if (Pair.Value.IsValid())
		{
			Pair.Value->Cleanup();
		}
	}

	mComponentPoolBuckets.Empty();

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

	if (!UKismetSystemLibrary::DoesImplementInterface(SignActor, URssSignInterface::StaticClass()))
	{
		UE_LOG(LogRSS, Warning,
		       TEXT("RSSDataManagerSubsystem::ForceLoadSign - SignActor does not implement IRssSignInterface!"));
		return;
	}

	URssWidgetRenderComponent* ExistingComponent = SignActor->FindComponentByClass<URssWidgetRenderComponent>();
	if (IsValid(ExistingComponent))
	{
		ExistingComponent->RequestRedraw();
		return;
	}

	RemoveFromPendingQueue(SignActor);

	TSubclassOf<URssWidgetRenderComponent> ComponentClass =
		IRssSignInterface::Execute_GetWidgetRenderComponentClass(SignActor);
	if (!ComponentClass)
	{
		UE_LOG(LogRSS, Error, TEXT("RSSDataManagerSubsystem::ForceLoadSign - No component class found!"));
		return;
	}

	FScopeLock MapLock(&mPoolMapLock);

	TSharedPtr<FRssComponentPoolBucket>* BucketPtr = mComponentPoolBuckets.Find(ComponentClass);
	TSharedPtr<FRssComponentPoolBucket> Bucket;

	if (!BucketPtr)
	{

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

	AFGCharacterPlayer* Player = UKBFL_Player::GetFGCharacter(this);
	if (!Player)
	{

		return;
	}

	int32 QueueSize = mPendingComponentRequests.Num();
	if (QueueSize == 0)
	{
		return;
	}

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

			mPendingRequestClasses.Remove(Actor);
			RemovedCount++;
			continue;
		}

		if (AFGVehicle* Vehicle = Cast<AFGVehicle>(Actor))
		{

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

		TSubclassOf<URssWidgetRenderComponent>* ComponentClassPtr = mPendingRequestClasses.Find(Actor);
		if (!ComponentClassPtr)
		{
			if (bEnablePoolStats)
			{
				UE_LOG(LogRSS, Error,
				       TEXT("RSSDataManagerSubsystem: No component class found in pending map for actor %s - removing"),
				       *Actor->GetName());
			}

			RemovedCount++;
			continue;
		}

		TSubclassOf<URssWidgetRenderComponent> ComponentClass = *ComponentClassPtr;
		if (HasActorAlreadyAssignedComponent(Actor))
		{
			mPendingRequestClasses.Remove(Actor);
			continue;
		}

		FScopeLock MapLock(&mPoolMapLock);

		TSharedPtr<FRssComponentPoolBucket>* BucketPtr = mComponentPoolBuckets.Find(ComponentClass);
		TSharedPtr<FRssComponentPoolBucket> Bucket;

		if (!BucketPtr)
		{

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

			mPendingComponentRequests.AddObject(Actor);
			continue;
		}

		URssWidgetRenderComponent* Component =
			Bucket->AcquireComponent(this, mMaxPoolSize, mAllPooledComponents, false);

		if (Component)
		{

			CreateComponentForActor(Actor, Component);

			mPendingRequestClasses.Remove(Actor);
			SuccessCount++;
		}
		else
		{

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

	if (mPendingRequestClasses.Remove(Actor) > 0)
	{

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

	MoveComponentToActor(Component, this, OwnerActor);

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
	const float MinDot = FMath::Cos(FOVRadians * 1.2f);

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

	TWeakObjectPtr<ARssDataManagerSubsystem> WeakThis(this);
	AsyncTask(
		ENamedThreads::AnyBackgroundThreadNormalTask,
		[WeakThis, AllActiveComponents = MoveTemp(AllActiveComponents), OwnerOrigins = MoveTemp(OwnerOrigins),
			ShouldCheckInView = MoveTemp(ShouldCheckInView), CameraLocation, CameraForward, MinDot]()
		{
			TArray<TWeakObjectPtr<URssWidgetRenderComponent>> NewVisibleComponents;

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

				            const FVector DirectionToOwner = (OwnerOrigins[Index] - CameraLocation).GetSafeNormal();
				            const float DotProduct = FVector::DotProduct(CameraForward, DirectionToOwner);

				            if (DotProduct >= MinDot)
				            {
					            FScopeLock Lock(&TempLock);
					            NewVisibleComponents.Add(ComponentPtr);
				            }
			            });

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
	return mSignRenderingDistance * 100.0f;
}

void ARssDataManagerSubsystem::MoveComponentToActor(UActorComponent* Component, AActor* SourceActor,
                                                    AActor* TargetActor)
{

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
