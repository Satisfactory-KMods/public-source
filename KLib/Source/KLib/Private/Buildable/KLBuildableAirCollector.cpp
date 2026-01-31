// ILikeBanas


#include "Buildable/KLBuildableAirCollector.h"

#include "BlueprintFunctionLib/KPCLBlueprintFunctionLib.h"
#include "Buildables/FGBuildableFoundation.h"
#include "Cpp/KBFLCppInventoryHelper.h"
#include "FGFactoryConnectionComponent.h"
#include "Kismet/KismetSystemLibrary.h"

#include "Net/UnrealNetwork.h"
#include "Resources/FGResourceNode.h"

TArray<UActorComponent*> GTempComps;

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

void AKLBuildableAirCollector::Factory_Tick(float dt)
{
	Super::Factory_Tick(dt);

	if (HasAuthority())
	{
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
}

void AKLBuildableAirCollector::GetConditionalReplicatedProps(TArray<FFGCondReplicatedProperty>& outProps) const
{
	Super::GetConditionalReplicatedProps(outProps);

	FG_DOREPCONDITIONAL(ThisClass, mHittedElements);
	FG_DOREPCONDITIONAL(ThisClass, mScannerInformation);
	FG_DOREPCONDITIONAL(ThisClass, mCollectorHeight);
	FG_DOREPCONDITIONAL(ThisClass, mCachedNearCollectors);
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

		TSubclassOf<UFGItemDescriptor> ProduceItem = GetScannerInformation()->mItemClass;
		UKPCLBlueprintFunctionLib::SetAllowOnIndex_ThreadSafe(GetInventory(), PRODUCT_INV_IDX, ProduceItem);
		SetProductionTime(GetScannerInformation()->mProductionTime);

		GetInventory()->AddArbitrarySlotSize(PRODUCT_INV_IDX, 500000);

		// Cache other Collectors
		FindNearbyCollectos();
	}

	// Change Collector Height
	mCollectorHeight = GetCollectorHeight();
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
	CacheNearbyCollectors();

	for (TWeakObjectPtr<AKLBuildableAirCollector> Collector : mCachedNearCollectors)
	{
		if (Collector.IsValid())
		{
			Collector->CacheNearbyCollectors();
		}
	}
}

void AKLBuildableAirCollector::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	if (endPlayReason == EEndPlayReason::Destroyed)
	{
		for (TWeakObjectPtr<AKLBuildableAirCollector> Collector : mCachedNearCollectors)
		{
			if (Collector.IsValid())
			{
				Collector->CacheNearbyCollectors();
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
	int32 Produce = FMath::CeilToInt32(FMath::CeilToFloat(CalculateProduceWithoutMalus() / 500.0f) * 500.0f);
	int32 WithMalus = FMath::CeilToInt32(FMath::CeilToFloat(CalculateProduce() / 500.0f) * 500.0f);
	return WithMalus - Produce;
}

void AKLBuildableAirCollector::GetCollectorHeightBonus(float& InPercentValue, float& InFloatValue) const
{
	float HighMulti = 1 + (mCollectorHeight - mCollectorMinHeight) / (mCollectorMaxHeight - mCollectorMinHeight) * 4;

	if (HighMulti < 1)
	{
		HighMulti = 1;
	}
	if (HighMulti > 5)
	{
		HighMulti = 5;
	}

	InPercentValue = HighMulti * 100;
	InFloatValue = HighMulti;
}

int32 AKLBuildableAirCollector::CalculateProduce() const
{
	float CollectorMalus = 1 + (mCachedNearCollectors.Num() * 1.1);

	float WithoutMalus = CalculateProduceWithoutMalus();
	float EndProduce = WithoutMalus / CollectorMalus;


	// Round to nearest 0.5k
	EndProduce = FMath::CeilToFloat(FMath::CeilToInt32(EndProduce) / 500.0f) * 500.0f;

	if (CollectorMalus <= 1.f)
	{
		return FMath::CeilToInt32(EndProduce);
	}

	int32 Min = GetScannerInformation()->mProduceItemCountMin;
	int32 Max = GetScannerInformation()->mProduceItemCountMax;
	int32 Produce = FMath::CeilToInt32(EndProduce);

	// Clamp to min/max from the definition
	return FMath::Clamp(Produce, Min, Max);
}

int32 AKLBuildableAirCollector::CalculateProduceWithoutMalus() const
{
	if (!IsValid(GetScannerInformation()))
	{
		return 1000;
	}

	int32 Produce;
	if (GetScannerInformation()->bUseHightBasesdProduction)
	{
		Produce = CalculateProductionBasedOnHeight();
	}
	else
	{
		Produce = CalculateProductionBasedOnHits();
	}

	return FMath::Clamp(Produce, static_cast<float>(GetScannerInformation()->mProduceItemCountMin),
						static_cast<float>(GetScannerInformation()->mProduceItemCountMax));
}

int32 AKLBuildableAirCollector::CalculateProductionBasedOnHits() const
{
	if (!IsValid(GetScannerInformation()))
	{
		return 1000;
	}

	int32 Hitted = mHittedElements;
	int32 PerHit = GetScannerInformation()->mProductionPerHit;

	return Hitted * PerHit;
}

int32 AKLBuildableAirCollector::CalculateProductionBasedOnHeight() const
{
	if (!IsValid(GetScannerInformation()))
	{
		return 1000;
	}

	float HighMulti = FMath::Clamp(
		1 + (mCollectorHeight - mCollectorMinHeight) / (mCollectorMaxHeight - mCollectorMinHeight) * 4, 1.f, 5.f);

	int32 Produce = GetScannerInformation()->mProduceItemCountBase;

	return Produce * HighMulti;
}

void AKLBuildableAirCollector::CacheNearbyCollectors()
{
	FVector ActorLocation = GetActorLocation();
	const TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes =
		TArray<TEnumAsByte<EObjectTypeQuery>>{ObjectTypeQuery1, ObjectTypeQuery2};

	TArray<AActor*> ActorsToIgnore = TArray<AActor*>{this};
	TArray<AActor*> OutActors;

	UKismetSystemLibrary::SphereOverlapActors(this, ActorLocation, mRangeForFindCollectors, ObjectTypes,
											  this->GetClass(), ActorsToIgnore, OutActors);

	mCachedNearCollectors.Empty();

	for (AActor* Actor : OutActors)
	{
		if (Actor != this)
		{
			AKLBuildableAirCollector* Collector = Cast<AKLBuildableAirCollector>(Actor);

			mCachedNearCollectors.Add(Collector);
		}
	}
}

void AKLBuildableAirCollector::GetCachedNearCollectors(TArray<AKLBuildableAirCollector*>& Collectors) const
{
	for (const TWeakObjectPtr<AKLBuildableAirCollector> Collector : mCachedNearCollectors)
	{
		if (Collector.IsValid())
		{
			Collectors.AddUnique(Collector.Get());
		}
	}
}

int32 AKLBuildableAirCollector::GetNumOfCollectorsInRange() const { return mCachedNearCollectors.Num(); }

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
	return Subsystem->AirCollector_GetAll();
}

void AKLBuildableAirCollector::CheckGasType()
{
	if (IsValid(mScannerInformation))
	{
		if (mHittedElements >= mScannerInformation->mMaxHit)
		{
			mHittedElements = mScannerInformation->mMaxHit;
		}

		return;
	}

	mHittedElements = 1;
	UKAPIAirCollectorData* ScannerResult = mScannerFallback;

	for (UKAPIAirCollectorData* Data : GetAllScans())
	{
		int32 Hitted = 0;
		if (Data->TestHit(this, Hitted))
		{
			if (Data->mRecipePrio >= ScannerResult->mRecipePrio)
			{
				ScannerResult = Data;
				mHittedElements = Hitted;
			}
		}
	}

	mScannerInformation = ScannerResult;
	fgcheck(mScannerInformation);
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

float AKLBuildableAirCollector::GetCollectorHeight() const
{
	const FVector ActorLocation = GetActorLocation();
	return ActorLocation.Z;
}
