// ILikeBanas

#include "Buildable/KLBuildableAirCollector.h"

#include <Net/UnrealNetwork.h>

#include "BlueprintFunctionLib/KPCLBlueprintFunctionLib.h"
#include "Buildables/FGBuildableFoundation.h"
#include "Cpp/KBFLCppInventoryHelper.h"
#include "FGFactoryConnectionComponent.h"
#include "Kismet/KismetSystemLibrary.h"

void AKLBuildableAirCollector::SetHittedElements(int32 NewValue)
{
	mHittedElements = NewValue;
	mPropertyReplicator.MarkPropertyDirty(FName("mHittedElements"));
	RecalculateProduction();
}

void AKLBuildableAirCollector::SetScannerInformation(UKAPIAirCollectorData* NewInfo)
{
	mScannerInformation = NewInfo;
	mPropertyReplicator.MarkPropertyDirty(FName("mScannerInformation"));
	RecalculateProduction();
}

void AKLBuildableAirCollector::SetCollectorHeight(float NewHeight)
{
	mCollectorHeight = NewHeight;
	mPropertyReplicator.MarkPropertyDirty(FName("mCollectorHeight"));
	RecalculateProduction();
}

AKLBuildableAirCollector::AKLBuildableAirCollector() : mCollectorHeight(0)
{
	mOutputInventory = CreateDefaultSubobject<UFGInventoryComponent>(FKPCLInventoryStructure::InputName);
	mOutputInventory->SetDefaultSize(3);
	bEnableCustomOverclocking = true;
}

void AKLBuildableAirCollector::Overclocking_GetInfo_Implementation(FKPCLOverclockingProductionInfo& OutProductionInfo)
{
	Super::Overclocking_GetInfo_Implementation(OutProductionInfo);

	if (GetScannerInformation())
	{
		OutProductionInfo.mItemClass = GetScannerInformation()->mItemClass;
		OutProductionInfo.mAmount = CalculateProduce();
	}
}

void AKLBuildableAirCollector::Overclocking_GetProductionResults_Implementation(
	TArray<FKPCLOverclockingProductionResults>& OutIngredients, TArray<FKPCLOverclockingProductionResults>& OutProducts)
{
	Super::Overclocking_GetProductionResults_Implementation(OutIngredients, OutProducts);

	if (GetScannerInformation())
	{
		OutProducts.Add(FKPCLOverclockingProductionResults(GetScannerInformation()->mItemClass, CalculateProduce(),
														   GetScannerInformation()->mProductionTime));
	}
}

void AKLBuildableAirCollector::Factory_TickAuthOnly(float dt)
{
	Super::Factory_TickAuthOnly(dt);

	if (IsValid(GetInventory()) && IsValid(GetScannerInformation()))
	{
		FInventoryStack OutStack;
		GetInventory()->GetStackFromIndex(PRODUCT_INV_IDX, OutStack);
		if (OutStack.HasItems())
		{
			TSubclassOf<UFGItemDescriptor> ProduceItem = GetScannerInformation()->mItemClass;
			if (OutStack.Item.GetItemClass() != ProduceItem)
			{
				GetInventory()->Empty();
			}
		}
	}
}

void AKLBuildableAirCollector::GetConditionalReplicatedProps(TArray<FFGCondReplicatedProperty>& outProps) const
{
	Super::GetConditionalReplicatedProps(outProps);

	FG_DOREPCONDITIONAL(ThisClass, mHittedElements);
	FG_DOREPCONDITIONAL(ThisClass, mScannerInformation);
	FG_DOREPCONDITIONAL(ThisClass, mCollectorHeight);
	FG_DOREPCONDITIONAL(ThisClass, mCachedNearCollectors);
	FG_DOREPCONDITIONAL(ThisClass, mCachedProduceAmount);
	FG_DOREPCONDITIONAL(ThisClass, mCachedProduceWithoutMalus);
}

void AKLBuildableAirCollector::CollectAndPushPipes(float dt, bool IsPush)
{
	Super::CollectAndPushPipes(dt, IsPush);

	if (IsPush)
	{
		UKBFLCppInventoryHelper::PushPipe(GetInventory(), PRODUCT_INV_IDX, dt, GetPipe(0, KPCLOutput));
	}
}

void AKLBuildableAirCollector::onProducingFinal_Implementation()
{
	fgcheck(GetScannerInformation());
	TSubclassOf<UFGItemDescriptor> ProduceItem = GetScannerInformation()->mItemClass;

	UKBFLCppInventoryHelper::StoreItemAmountInInventory(GetInventory(), PRODUCT_INV_IDX, ProduceItem,
														CalculateProduce());
}

void AKLBuildableAirCollector::BeginPlay()
{
	if (!mMeshOverwriteInformations.IsValidIndex(0))
	{
		SetInstancedMesh();
	}

	Super::BeginPlay();

	if (HasAuthority())
	{
		CheckGasType();

		if (IsValid(GetScannerInformation()))
		{
			TSubclassOf<UFGItemDescriptor> ProduceItem = GetScannerInformation()->mItemClass;
			UKPCLBlueprintFunctionLib::SetAllowOnIndex_ThreadSafe(GetInventory(), PRODUCT_INV_IDX, ProduceItem);
			SetProductionTime(GetScannerInformation()->mProductionTime);
		}

		GetInventory()->AddArbitrarySlotSize(PRODUCT_INV_IDX, 500000);

		// Cache other Collectors
		FindNearbyCollectos();
	}

	// Change Collector Height — triggers RecalculateProduction() via the setter.
	// Also covers the server path above since SetCollectorHeight calls RecalculateProduction().
	SetCollectorHeight(GetCollectorHeight());
}

bool AKLBuildableAirCollector::CanProduce_Implementation() const
{
	if (!Super::CanProduce_Implementation() || !HasPower() || !IsValid(GetScannerInformation()))
	{
		return false;
	}

	TSubclassOf<UFGItemDescriptor> ProduceItem = GetScannerInformation()->mItemClass;
	int32 Amount = CalculateProduce();

	return UKBFLCppInventoryHelper::CanStoreItem(GetInventory(), PRODUCT_INV_IDX, ProduceItem, Amount);
}

void AKLBuildableAirCollector::FindNearbyCollectos()
{
	// Self-scan: one SphereOverlapActors for this collector; also calls RecalculateProduction().
	CacheNearbyCollectors();

	// Incremental push: instead of asking each neighbour to re-scan the world (O(N) overlaps),
	// simply insert this collector into their cached list and trigger a cheap recalculate.
	for (TWeakObjectPtr<AKLBuildableAirCollector>& Collector : mCachedNearCollectors)
	{
		if (Collector.IsValid())
		{
			Collector->mCachedNearCollectors.AddUnique(this);
			Collector->mPropertyReplicator.MarkPropertyDirty(FName("mCachedNearCollectors"));
			Collector->RecalculateProduction();
		}
	}
}

void AKLBuildableAirCollector::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	if (endPlayReason == EEndPlayReason::Destroyed)
	{
		// Incremental remove: pull this collector out of each neighbour's cached list without
		// triggering a world query on their side. Then recompute their production cheaply.
		for (TWeakObjectPtr<AKLBuildableAirCollector>& Collector : mCachedNearCollectors)
		{
			if (Collector.IsValid())
			{
				Collector->mCachedNearCollectors.RemoveAll([this](const TWeakObjectPtr<AKLBuildableAirCollector>& Ptr)
														   { return Ptr.Get() == this; });
				Collector->mPropertyReplicator.MarkPropertyDirty(FName("mCachedNearCollectors"));
				Collector->RecalculateProduction();
			}
		}
	}

	Super::EndPlay(endPlayReason);
}

void AKLBuildableAirCollector::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

int32 AKLBuildableAirCollector::CalculateProduceMalus() const
{
	// Both cached values are already rounded/clamped by RecalculateProduction().
	return mCachedProduceWithoutMalus - mCachedProduceAmount;
}

void AKLBuildableAirCollector::GetCollectorHeightBonus(float& InPercentValue, float& InFloatValue) const
{
	const float HighMulti = GetHeightMultiplier();
	InPercentValue = HighMulti * 100.0f;
	InFloatValue = HighMulti;
}

int32 AKLBuildableAirCollector::CalculateProduce() const
{
	// Hot-path (may be called from worker thread via CanProduce_Implementation):
	// return the cached value computed on the game thread. Atomic int32 read, lock-free.
	return mCachedProduceAmount;
}

int32 AKLBuildableAirCollector::CalculateProduceWithoutMalus() const
{
	// Returns the cached value computed on the game thread.
	return mCachedProduceWithoutMalus;
}

void AKLBuildableAirCollector::RecalculateProduction()
{
	if (!HasAuthority())
	{
		return;
	}

	UKAPIAirCollectorData* ScannerInfo = GetScannerInformation();

	// --- Without-malus value ---
	int32 WithoutMalus;
	if (!IsValid(ScannerInfo))
	{
		WithoutMalus = 1000;
	}
	else if (ScannerInfo->bUseHightBasesdProduction)
	{
		WithoutMalus = CalculateProductionBasedOnHeight();
	}
	else
	{
		WithoutMalus = CalculateProductionBasedOnHits();
	}

	if (IsValid(ScannerInfo))
	{
		WithoutMalus = FMath::Clamp(WithoutMalus, ScannerInfo->mProduceItemCountMin, ScannerInfo->mProduceItemCountMax);
	}

	mCachedProduceWithoutMalus = WithoutMalus;
	mPropertyReplicator.MarkPropertyDirty(FName("mCachedProduceWithoutMalus"));

	// --- With-malus value (neighbour penalty) ---
	if (!IsValid(ScannerInfo))
	{
		mCachedProduceAmount = 1000;
	}
	else
	{
		const float ValidCollectorCount = static_cast<float>(GetValidNearCollectorCount());
		const float CollectorMalus = 1.0f + (ValidCollectorCount * 1.1f);
		const float EndProduceRaw = static_cast<float>(WithoutMalus) / CollectorMalus;

		// Round to nearest 0.5k and clamp.
		const float EndProduce = FMath::FloorToFloat(FMath::FloorToInt32(EndProduceRaw) / 500.0f) * 500.0f;
		const int32 Produce = FMath::FloorToInt32(EndProduce);
		mCachedProduceAmount =
			FMath::Clamp(Produce, ScannerInfo->mProduceItemCountMin, ScannerInfo->mProduceItemCountMax);
	}

	mPropertyReplicator.MarkPropertyDirty(FName("mCachedProduceAmount"));
}

int32 AKLBuildableAirCollector::CalculateProductionBasedOnHits() const
{
	if (!IsValid(GetScannerInformation()))
	{
		return 1000;
	}

	const int32 Hitted = mHittedElements;
	const int32 PerHit = GetScannerInformation()->mProductionPerHit;

	return Hitted * PerHit;
}

int32 AKLBuildableAirCollector::CalculateProductionBasedOnHeight() const
{
	if (!IsValid(GetScannerInformation()))
	{
		return 1000;
	}

	const int32 Produce = GetScannerInformation()->mProduceItemCountBase;
	return static_cast<int32>(Produce * GetHeightMultiplier());
}

void AKLBuildableAirCollector::CacheNearbyCollectors()
{
	if (!HasAuthority())
	{
		return;
	}

	const FVector ActorLocation = GetActorLocation();
	const TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes =
		TArray<TEnumAsByte<EObjectTypeQuery>>{ObjectTypeQuery1, ObjectTypeQuery2};

	TArray<AActor*> ActorsToIgnore = TArray<AActor*>{this};
	TArray<AActor*> OutActors;

	UKismetSystemLibrary::SphereOverlapActors(this, ActorLocation, mRangeForFindCollectors, ObjectTypes,
											  this->GetClass(), ActorsToIgnore, OutActors);

	mCachedNearCollectors.Empty();

	for (AActor* Actor : OutActors)
	{
		if (Actor == this)
		{
			continue;
		}
		AKLBuildableAirCollector* Collector = Cast<AKLBuildableAirCollector>(Actor);
		if (!IsValid(Collector))
		{
			continue;
		}
		mCachedNearCollectors.AddUnique(Collector);
	}

	mPropertyReplicator.MarkPropertyDirty(FName("mCachedNearCollectors"));

	// Production depends on how many valid neighbours are cached, so recompute.
	RecalculateProduction();
}

void AKLBuildableAirCollector::GetCachedNearCollectors(TArray<AKLBuildableAirCollector*>& Collectors) const
{
	for (const TWeakObjectPtr<AKLBuildableAirCollector>& Collector : mCachedNearCollectors)
	{
		if (Collector.IsValid())
		{
			Collectors.AddUnique(Collector.Get());
		}
	}
}

int32 AKLBuildableAirCollector::GetNumOfCollectorsInRange() const { return GetValidNearCollectorCount(); }

UKAPIAirCollectorData* AKLBuildableAirCollector::GetScannerInformation() const
{
	if (!IsValid(mScannerInformation))
	{
		return mScannerFallback;
	}

	return mScannerInformation;
}

TArray<UKAPIAirCollectorData*> AKLBuildableAirCollector::GetAllScans() const
{
	UKAPIDataAssetSubsystem* Subsystem = UKAPIDataAssetSubsystem::Get(GetWorld());
	if (!IsValid(Subsystem))
	{
		return {};
	}
	return Subsystem->AirCollector_GetAll();
}

void AKLBuildableAirCollector::CheckGasType()
{
	if (IsValid(mScannerInformation))
	{
		if (mHittedElements >= mScannerInformation->mMaxHit)
		{
			SetHittedElements(mScannerInformation->mMaxHit);
		}
		return;
	}

	SetHittedElements(1);
	UKAPIAirCollectorData* ScannerResult = mScannerFallback;

	for (UKAPIAirCollectorData* Data : GetAllScans())
	{
		int32 Hitted = 0;
		if (Data->TestHit(this, Hitted))
		{
			// Guard against a null fallback — treat any match as higher priority if no winner yet.
			if (!IsValid(ScannerResult) || Data->mRecipePrio >= ScannerResult->mRecipePrio)
			{
				ScannerResult = Data;
				SetHittedElements(Hitted);
			}
		}
	}

	// ScannerResult may still be null if fallback is unset and nothing matched.
	// GetScannerInformation() handles null mScannerInformation gracefully via mScannerFallback.
	SetScannerInformation(ScannerResult);
}

void AKLBuildableAirCollector::SetInstancedMesh()
{
	const TArray<AActor*> ActorsToIgnore{this};
	TArray<AActor*> OutActors;
	FHitResult Result;

	ETraceTypeQuery TraceChannel = TraceTypeQuery1;
	if (UKismetSystemLibrary::LineTraceSingle(GetWorld(), GetActorLocation() + FVector(0, 0, 100),
											  GetActorLocation() - FVector(0, 0, 100), TraceChannel, false,
											  ActorsToIgnore, EDrawDebugTrace::None, Result, true))
	{
		AActor* HitActor = UKPCLBlueprintFunctionLib::ResolveHitResult(Result);
		if (const AFGBuildableFoundation* Foundation = Cast<AFGBuildableFoundation>(HitActor))
		{
			if (const int32 Key = FMath::TruncToInt(Foundation->mHeight / 100))
			{
				if (mFoundationMeshMapping.Contains(Key))
				{
					FKPCLMeshOverwriteInformation Information;
					Information.mOverwriteMesh = mFoundationMeshMapping[Key];
					Information.mOverwriteHandleIndex = 0;
					if (!mMeshOverwriteInformations.IsValidIndex(0))
					{
						mMeshOverwriteInformations.Add(Information);
					}
					else
					{
						mMeshOverwriteInformations[0] = Information;
					}
				}
			}
		}
	}
}

float AKLBuildableAirCollector::GetCollectorHeight() const { return GetActorLocation().Z; }

int32 AKLBuildableAirCollector::GetValidNearCollectorCount() const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<AKLBuildableAirCollector>& Collector : mCachedNearCollectors)
	{
		if (Collector.IsValid())
		{
			++Count;
		}
	}
	return Count;
}

float AKLBuildableAirCollector::GetHeightMultiplier() const
{
	return FMath::Clamp(1.0f +
							(mCollectorHeight - static_cast<float>(mCollectorMinHeight)) /
								(static_cast<float>(mCollectorMaxHeight) - static_cast<float>(mCollectorMinHeight)) *
								4.0f,
						1.0f, 5.0f);
}
