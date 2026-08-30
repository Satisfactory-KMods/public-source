#include "Subsystems/KAPIDataAssetSubsystem.h"

#include "Buildables/FGBuildableManufacturer.h"
#include "DataAssets/KAPIExtractorAllowList.h"
#include "Engine/GameInstance.h"
#include "Logging/StructuredLog.h"

UKAPIDataAssetSubsystem::UKAPIDataAssetSubsystem() {}

void UKAPIDataAssetSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(FName("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	if (AssetRegistry.IsLoadingAssets())
	{
		AssetRegistry.OnFilesLoaded().AddUObject(this, &UKAPIDataAssetSubsystem::StartScanForDataAssets);
	}
	else
	{
		StartScanForDataAssets();
	}
}

void UKAPIDataAssetSubsystem::Deinitialize()
{
	Super::Deinitialize();
	EmptyAssets();
}

TArray<UKAPIAirCollectorData*> UKAPIDataAssetSubsystem::AirCollector_GetAll() const { return mAirCollectorDataAssets; }

bool UKAPIDataAssetSubsystem::ApplyManufacturerAssign(AFGBuildableManufacturer* Manufacturer,
													  TSubclassOf<class UFGRecipe> recipe)
{
	for (UKAPIManufacturerModifications* Modification : mOrderedManufacturerModifications)
	{
		if (Modification->bAllowMoreInputSlots && Modification->ApplyInventoryChanges(Manufacturer, recipe))
		{
			return true;
		}
	}
	return false;
}

void UKAPIDataAssetSubsystem::ApplyManufacturerModifications(AFGBuildableManufacturer* Manufacturer,
															 TSubclassOf<class UFGRecipe> recipe)
{
	for (UKAPIManufacturerModifications* Modification : mOrderedManufacturerModifications)
	{
		if (Modification->ApplyInventoryChanges(Manufacturer, recipe))
		{
			return;
		}
	}
}

TArray<UKAPICleanerItemDescription*> UKAPIDataAssetSubsystem::Cleaner_GetAllAssets()
{
	TArray<UKAPICleanerItemDescription*> OutItems;
	OutItems.Empty();
	TArray<TObjectPtr<UKAPICleanerItemDescription>> OutPtrs;
	mCleanerItemMapping.GenerateValueArray(OutPtrs);
	OutItems = TArray<UKAPICleanerItemDescription*>(MutableView(OutPtrs));
	return OutItems;
}

bool UKAPIDataAssetSubsystem::Cleaner_GetForKey(TSubclassOf<UFGItemDescriptor> Item,
												UKAPICleanerItemDescription*& OutItem)
{
	const TObjectPtr<UKAPICleanerItemDescription>* Ptr = mCleanerItemMapping.Find(Item);
	OutItem = nullptr;
	if (Ptr)
	{
		OutItem = *Ptr;
	}
	return IsValid(OutItem);
}

void UKAPIDataAssetSubsystem::EmptyAssets()
{
	mDisabledDataAssets.Empty();
	mEnabledDataAssets.Empty();
	mCleanerItemMapping.Empty();
	mMinerMapping.Empty();
	mAllowedScannableResources.Empty();
	mAllowedResourceExtractors.Empty();
	mTooltipWidgetsDataAssets.Empty();
	mOrderedTooltipWidgetsDataAssets.Empty();
	mAirCollectorDataAssets.Empty();
	mSlugDatas.Empty();
	mManufacturerModifications.Empty();
	mOrderedManufacturerModifications.Empty();
	mDeliveryTasks.Empty();
}

void UKAPIDataAssetSubsystem::CollectDataAssetsOfClass(const UClass* AssetClass,
													   TArray<UKAPIDataAssetBase*>& OutSortedAssets)
{
	OutSortedAssets.Empty();

	if (!IsValid(AssetClass))
	{
		return;
	}

	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(FName("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FAssetData> AssetList;
	AssetRegistry.GetAssetsByClass(AssetClass->GetClassPathName(), AssetList, true);

	TArray<FSortiableAsset> RawObjects;
	RawObjects.Reserve(AssetList.Num());

	for (const FAssetData& Asset : AssetList)
	{
		UObject* Obj = Asset.GetAsset();

		if (!IsValid(Obj) || !Obj->IsA(AssetClass))
		{
			UE_LOG(LogKApi, Warning, TEXT("Invalid asset type: %s"), *Asset.AssetName.ToString());
			continue;
		}

		if (UKAPIDataAssetBase* BaseAsset = Cast<UKAPIDataAssetBase>(Obj))
		{
			if (UKAPIDataAssetBase::IsEnabled(BaseAsset, GetWorld()))
			{
				UE_LOG(LogKApi, Log, TEXT("Found %s"), *BaseAsset->GetPathName());
				RawObjects.Add(FSortiableAsset(BaseAsset));
				mEnabledDataAssets.Add(BaseAsset);
				continue;
			}

			UE_LOG(LogKApi, Warning, TEXT("Asset Disabled: %s"), *BaseAsset->GetPathName());
			mDisabledDataAssets.Add(BaseAsset);
		}
	}

	RawObjects.Sort(FSortiableAsset::SortByPriorityAndPath);

	OutSortedAssets.Reserve(RawObjects.Num());
	for (const FSortiableAsset& RawObject : RawObjects)
	{
		OutSortedAssets.Add(RawObject.mObject);
	}
}

UKAPIDataAssetSubsystem* UKAPIDataAssetSubsystem::Get(UObject* Context)
{
	return GetGameInstanceSubsystemFromContext<UKAPIDataAssetSubsystem>(Context);
}

TArray<UKAPTooltipWidgetInjector*> UKAPIDataAssetSubsystem::GetAllTooltipWidgetInjectors()
{
	return mOrderedTooltipWidgetsDataAssets;
}

UKAPIDataAssetSubsystem* UKAPIDataAssetSubsystem::GetChecked(UObject* Context)
{
	UKAPIDataAssetSubsystem* Subsystem = Get(Context);
	fgcheck(Subsystem);
	return Subsystem;
}

UKAPIManufacturerModifications* UKAPIDataAssetSubsystem::GetModification(AFGBuildableManufacturer* Manufacturer)
{
	for (UKAPIManufacturerModifications* Modification : mOrderedManufacturerModifications)
	{
		if (Modification->MatchManufacturer(Manufacturer))
		{
			return Modification;
		}
	}
	return nullptr;
}

UModLoadingLibrary* UKAPIDataAssetSubsystem::GetModLoadingLibraryWithContext(UObject* Context)
{
	return GetGameInstanceSubsystemFromContext<UModLoadingLibrary>(Context);
}

TArray<UKAPIDataAssetBase*>
UKAPIDataAssetSubsystem::K2_FindAllDataAssetsOfClass(TSubclassOf<UKAPIDataAssetBase> InClass)
{
	TArray<UKAPIDataAssetBase*> OutDataAssets;

	if (!IsValid(InClass) || !InClass->IsChildOf(UKAPIDataAssetBase::StaticClass()))
	{
		UE_LOG(LogKApi, Warning, TEXT("K2_FindAllDataAssetsOfClass: Invalid or non-KAPIDataAssetBase InClass."));
		return OutDataAssets;
	}

	CollectDataAssetsOfClass(InClass, OutDataAssets);

	UE_LOG(LogKApi, Warning, TEXT("Found %d of: %s"), OutDataAssets.Num(), *InClass->GetPathName());
	return OutDataAssets;
}

TArray<UKAPIModularMinerDescription*> UKAPIDataAssetSubsystem::Miner_GetAllAssets()
{
	TArray<UKAPIModularMinerDescription*> OutItems;
	OutItems.Empty();
	TArray<TObjectPtr<UKAPIModularMinerDescription>> OutPtrs;
	mMinerMapping.GenerateValueArray(OutPtrs);
	OutItems = TArray<UKAPIModularMinerDescription*>(MutableView(OutPtrs));
	return OutItems;
}

bool UKAPIDataAssetSubsystem::Miner_GetForKey(TSubclassOf<UFGResourceDescriptor> Item,
											  UKAPIModularMinerDescription*& OutItem)
{
	OutItem = nullptr;
	if (mMinerMapping.Contains(Item))
	{
		OutItem = *mMinerMapping.Find(Item);
	}
	return IsValid(OutItem);
}

void UKAPIDataAssetSubsystem::ScanForAirCollectorDataAssets()
{
	TArray<UKAPIAirCollectorData*> Objects;
	FindAllDataAssetsOfClass(Objects);
	mAirCollectorDataAssets.Empty();
	for (UKAPIAirCollectorData* Object : Objects)
	{
		mAirCollectorDataAssets.Add(Object);
	}
}

void UKAPIDataAssetSubsystem::ScanForAllowList()
{
	TArray<UKAPIExtractorAllowList*> Objects;
	FindAllDataAssetsOfClass(Objects);

	for (UKAPIExtractorAllowList* Object : Objects)
	{
		for (TSubclassOf<UFGResourceDescriptor> AllowedResource : Object->mAllowedResources)
		{
			mAllowedScannableResources.Add(AllowedResource);
		}

		for (TSubclassOf<UFGResourceDescriptor> DisallowedResource : Object->mDisallowResources)
		{
			mAllowedScannableResources.Remove(DisallowedResource);
		}

		for (TSubclassOf<AFGBuildableResourceExtractorBase> AllowedResource : Object->mAllowedExtractors)
		{
			mAllowedResourceExtractors.Add(AllowedResource);
		}

		for (TSubclassOf<AFGBuildableResourceExtractorBase> DisallowedResource : Object->mDisallowExtractors)
		{
			mAllowedResourceExtractors.Remove(DisallowedResource);
		}
	}

	UE_LOG(LogKApi, Warning, TEXT("Found %d of: %s"), mAllowedResourceExtractors.Num(),
		   TEXT("mAllowedResourceExtractors"));
	for (TSubclassOf<AFGBuildableResourceExtractorBase> Class : mAllowedResourceExtractors)
	{
		UE_LOG(LogKApi, Warning, TEXT("mAllowedResourceExtractors Found %s"), *Class->GetPathName());
	}

	UE_LOG(LogKApi, Warning, TEXT("Found %d of: %s"), mAllowedScannableResources.Num(),
		   TEXT("mAllowedScannableResources"));
	for (TSubclassOf<UFGResourceDescriptor> Class : mAllowedScannableResources)
	{
		UE_LOG(LogKApi, Warning, TEXT("mAllowedScannableResources Found %s"), *Class->GetPathName());
	}
}

void UKAPIDataAssetSubsystem::ScanForCleanerAssets()
{
	TArray<UKAPICleanerItemDescription*> Objects;
	FindAllDataAssetsOfClass(Objects);
	mCleanerItemMapping.Empty();

	for (UKAPICleanerItemDescription* Object : Objects)
	{
		if (!IsValid(Object) || !IsValid(Object->mInFluid.ItemClass))
		{
			UE_LOGFMT(LogKApi, Warning,
					  "ScanForCleanerAssets: Skipping cleaner asset with invalid mInFluid.ItemClass {Field}",
					  GetNameSafe(Object));
			continue;
		}
		if (mCleanerItemMapping.Contains(Object->mInFluid.ItemClass))
		{
			UE_LOGFMT(LogKApi, Warning, "ScanForCleanerAssets: Ignoring lower-ranked duplicate {Asset} for {ItemClass}",
					  Object->GetPathName(), Object->mInFluid.ItemClass->GetPathName());
			continue;
		}
		mCleanerItemMapping.Add(Object->mInFluid.ItemClass, Object);
	}
}

void UKAPIDataAssetSubsystem::ScanForManufacturerModifications()
{
	TArray<UKAPIManufacturerModifications*> Objects;
	FindAllDataAssetsOfClass(Objects);
	mManufacturerModifications.Empty();
	mOrderedManufacturerModifications.Empty();
	for (UKAPIManufacturerModifications* Object : Objects)
	{
		mManufacturerModifications.Add(Object);
		mOrderedManufacturerModifications.Add(Object);
	}
}

void UKAPIDataAssetSubsystem::ScanForMinerAssets()
{
	TArray<UKAPIModularMinerDescription*> Objects;
	FindAllDataAssetsOfClass(Objects);
	mMinerMapping.Empty();

	for (UKAPIModularMinerDescription* Object : Objects)
	{
		if (!IsValid(Object) || !IsValid(Object->mResourceClass))
		{
			UE_LOGFMT(LogKApi, Warning, "ScanForMinerAssets: Skipping miner asset with invalid mResourceClass {Field}",
					  GetNameSafe(Object));
			continue;
		}
		if (mMinerMapping.Contains(Object->mResourceClass))
		{
			UE_LOGFMT(LogKApi, Warning,
					  "ScanForMinerAssets: Ignoring lower-ranked duplicate {Asset} for {ResourceClass}",
					  Object->GetPathName(), Object->mResourceClass->GetPathName());
			continue;
		}
		mMinerMapping.Add(Object->mResourceClass, Object);

		mAllowedScannableResources.Add(Object->mResourceClass);
	}
}

void UKAPIDataAssetSubsystem::ScanForSlugs()
{
	TArray<UKAPISugHatchingData*> Objects;
	FindAllDataAssetsOfClass(Objects);
	mSlugDatas.Empty();
	for (UKAPISugHatchingData* Object : Objects)
	{
		if (!mSlugDatas.Contains(Object->mSlug))
		{
			mSlugDatas.Add(Object->mSlug, Object);
		}
		if (!mSlugDatas.Contains(Object->mEgg))
		{
			mSlugDatas.Add(Object->mEgg, Object);
		}
	}
}

void UKAPIDataAssetSubsystem::ScanForWidgetInjector()
{
	if (IsRunningDedicatedServer())
	{
		return;
	}

	TArray<UKAPTooltipWidgetInjector*> Objects;
	FindAllDataAssetsOfClass(Objects);
	mTooltipWidgetsDataAssets.Empty();
	mOrderedTooltipWidgetsDataAssets.Empty();

	for (UKAPTooltipWidgetInjector* Object : Objects)
	{
		if (!IsValid(Object) || !IsValid(Object->mWidgetClass))
		{
			UE_LOGFMT(LogKApi, Warning,
					  "ScanForWidgetInjector: Skipping injector asset with invalid mWidgetClass {Field}",
					  GetNameSafe(Object));
			continue;
		}
		mTooltipWidgetsDataAssets.Add(Object);
		mOrderedTooltipWidgetsDataAssets.Add(Object);
	}
}

void UKAPIDataAssetSubsystem::ScanForDeliveryTasks()
{
	TArray<UKAPIDeliveryTask*> Objects;
	FindAllDataAssetsOfClass(Objects);
	mDeliveryTasks.Empty();
	mDeliveryTasks.Reserve(Objects.Num());

	for (UKAPIDeliveryTask* Object : Objects)
	{

		FIntVector2 Coordinate = Object->mCoordinate;
		TObjectPtr<UKAPIDeliveryTask>* ExistingTask = mDeliveryTasks.Find(Coordinate);
		if (ExistingTask && GetValid(*ExistingTask) && *ExistingTask != Object)
		{
			UE_LOGFMT(LogKApi, Fatal,
					  "%s and %s have the same coordinate (%d, %d). Delivery task coordinates must be unique.",
					  GetNameSafe(Object), GetNameSafe(*ExistingTask), Coordinate.X, Coordinate.Y);
		}

		mDeliveryTasks.Add(Object->mCoordinate, Object);
	}

	for (UKAPIDeliveryTask* Object : Objects)
	{
		Object->AssertTask(mDeliveryTasks);
	}

	bHasCompletedDeliveryTaskScan = true;
	++mDeliveryTaskRevision;
	mOnDeliveryTasksChanged.Broadcast();
}

TArray<UKAPISugHatchingData*> UKAPIDataAssetSubsystem::Slug_GetAll() const
{
	TArray<UKAPISugHatchingData*> OutArray;
	TArray<TObjectPtr<UKAPISugHatchingData>> OutPtrs;
	mSlugDatas.GenerateValueArray(OutPtrs);
	OutArray = TArray<UKAPISugHatchingData*>(MutableView(OutPtrs));
	TSet<UKAPISugHatchingData*> UniqueSet(OutArray);
	OutArray = UniqueSet.Array();
	return OutArray;
}

bool UKAPIDataAssetSubsystem::Slug_GetForItem(TSubclassOf<UFGItemDescriptor> SlugOrEgg, UKAPISugHatchingData*& OutItem)
{
	const TObjectPtr<UKAPISugHatchingData>* Ptr = mSlugDatas.Find(SlugOrEgg);
	OutItem = nullptr;
	if (Ptr)
	{
		OutItem = *Ptr;
	}
	return IsValid(OutItem);
}

TMap<FIntVector2, UKAPIDeliveryTask*> UKAPIDataAssetSubsystem::Delivery_GetAll() const
{
	TMap<FIntVector2, UKAPIDeliveryTask*> OutTasks;
	OutTasks.Reserve(mDeliveryTasks.Num());

	for (const TPair<FIntVector2, TObjectPtr<UKAPIDeliveryTask>>& Task : mDeliveryTasks)
	{
		if (!GetValid(Task.Value))
		{
			continue;
		}
		OutTasks.Add(Task.Key, Task.Value.Get());
	}

	return OutTasks;
}

void UKAPIDataAssetSubsystem::StartScanForDataAssets()
{
	EmptyAssets();

	ScanForAllowList();
	ScanForCleanerAssets();
	ScanForMinerAssets();
	ScanForWidgetInjector();
	ScanForAirCollectorDataAssets();
	ScanForSlugs();
	ScanForManufacturerModifications();
	ScanForDeliveryTasks();
}
