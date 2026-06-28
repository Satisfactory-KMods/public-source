#include "Subsystems/KAPIDataAssetSubsystem.h"

#include "Buildables/FGBuildableManufacturer.h"
#include "DataAssets/KAPIExtractorAllowList.h"
#include "Logging/StructuredLog.h"

UKAPIDataAssetSubsystem::UKAPIDataAssetSubsystem() {}

//~ Begin USubsystem Interface
void UKAPIDataAssetSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(FName("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	// Must wait until all assets are discovered before populating list of assets.
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
//~ End USubsystem Interface

TArray<UKAPIAirCollectorData*> UKAPIDataAssetSubsystem::AirCollector_GetAll() const { return mAirCollectorDataAssets; }

bool UKAPIDataAssetSubsystem::ApplyManufacturerAssign(AFGBuildableManufacturer* Manufacturer,
													  TSubclassOf<class UFGRecipe> recipe)
{
	for (UKAPIManufacturerModifications* Modification : mManufacturerModifications)
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
	for (UKAPIManufacturerModifications* Modification : mManufacturerModifications)
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
	mAirCollectorDataAssets.Empty();
	mSlugDatas.Empty();
	mManufacturerModifications.Empty();
}

UKAPIDataAssetSubsystem* UKAPIDataAssetSubsystem::Get(UObject* Context)
{
	if (!IsValid(Context))
	{
		return nullptr;
	}

	UWorld* World = Context->GetWorld();
	if (!IsValid(World))
	{
		return nullptr;
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	if (!IsValid(GameInstance))
	{
		return nullptr;
	}

	return GameInstance->GetSubsystem<UKAPIDataAssetSubsystem>();
}

TArray<UKAPTooltipWidgetInjector*> UKAPIDataAssetSubsystem::GetAllTooltipWidgetInjectors()
{
	return mTooltipWidgetsDataAssets.Array();
}

UKAPIDataAssetSubsystem* UKAPIDataAssetSubsystem::GetChecked(UObject* Context)
{
	UKAPIDataAssetSubsystem* Subsystem = Get(Context);
	fgcheck(Subsystem);
	return Subsystem;
}

UKAPIManufacturerModifications* UKAPIDataAssetSubsystem::GetModification(AFGBuildableManufacturer* Manufacturer)
{
	for (UKAPIManufacturerModifications* Modification : mManufacturerModifications)
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
	if (!IsValid(Context))
	{
		return nullptr;
	}

	UWorld* World = Context->GetWorld();
	if (!IsValid(World))
	{
		return nullptr;
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	if (!IsValid(GameInstance))
	{
		return nullptr;
	}

	return GameInstance->GetSubsystem<UModLoadingLibrary>();
}

TArray<UKAPIDataAssetBase*>
UKAPIDataAssetSubsystem::K2_FindAllDataAssetsOfClass(TSubclassOf<UKAPIDataAssetBase> InClass)
{
	TArray<UKAPIDataAssetBase*> OutDataAssets;
	OutDataAssets.Empty();

	if (!IsValid(InClass) || !InClass->IsChildOf(UKAPIDataAssetBase::StaticClass()))
	{
		UE_LOG(LogKApi, Warning, TEXT("K2_FindAllDataAssetsOfClass: Invalid or non-KAPIDataAssetBase InClass."));
		return OutDataAssets;
	}

	// Find list of all UStat, and USkill assets in Content Browser.
	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(FName("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FAssetData> AssetList;
	AssetRegistry.GetAssetsByClass(InClass->GetClassPathName(), AssetList, true);

	TArray<FSortiableAsset> rawObjects;

	// Split assets into separate arrays.
	for (const FAssetData& Asset : AssetList)
	{
		UObject* Obj = Asset.GetAsset();

		UKAPIDataAssetBase* CastedAsset = Cast<UKAPIDataAssetBase>(Obj);
		if (!CastedAsset)
		{
			UE_LOG(LogKApi, Warning, TEXT("Invalid asset type: %s"), *Asset.AssetName.ToString());
			continue;
		}

		if (UKAPIDataAssetBase* BaseAsset = Cast<UKAPIDataAssetBase>(Obj))
		{
			if (UKAPIDataAssetBase::IsEnabled(BaseAsset, GetWorld()))
			{
				UE_LOG(LogKApi, Log, TEXT("Found %s"), *BaseAsset->GetPathName());
				rawObjects.Add(FSortiableAsset(BaseAsset));
				mEnabledDataAssets.Add(BaseAsset);
				continue;
			}

			UE_LOG(LogKApi, Warning, TEXT("Asset Disabled: %s"), *BaseAsset->GetPathName());
			mDisabledDataAssets.Add(BaseAsset);
		}
	}

	rawObjects.Sort([&](const FSortiableAsset& A, const FSortiableAsset& B) { return A.mPriority > B.mPriority; });

	for (FSortiableAsset Str : rawObjects)
	{
		OutDataAssets.Add(Str.mObject);
	}

	rawObjects.Empty();

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
	TSet<UKAPIAirCollectorData*> Objects;
	FindAllDataAssetsOfClass(Objects);
	mAirCollectorDataAssets = Objects.Array();
}

void UKAPIDataAssetSubsystem::ScanForAllowList()
{
	TArray<UKAPIExtractorAllowList*> Objects;
	FindAllDataAssetsOfClassUnfiltered(Objects);

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
	TSet<UKAPICleanerItemDescription*> Objects;
	FindAllDataAssetsOfClass(Objects);

	for (UKAPICleanerItemDescription* Object : Objects)
	{
		if (!IsValid(Object) || !IsValid(Object->mInFluid.ItemClass))
		{
			UE_LOGFMT(LogKApi, Warning,
					  "ScanForCleanerAssets: Skipping cleaner asset with invalid mInFluid.ItemClass {Field}",
					  GetNameSafe(Object));
			continue;
		}
		mCleanerItemMapping.Add(Object->mInFluid.ItemClass, Object);
	}
}

void UKAPIDataAssetSubsystem::ScanForManufacturerModifications()
{
	FindAllDataAssetsOfClass(mManufacturerModifications);
}

void UKAPIDataAssetSubsystem::ScanForMinerAssets()
{
	TSet<UKAPIModularMinerDescription*> Objects;
	FindAllDataAssetsOfClass(Objects);

	for (UKAPIModularMinerDescription* Object : Objects)
	{
		if (!IsValid(Object) || !IsValid(Object->mResourceClass))
		{
			UE_LOGFMT(LogKApi, Warning, "ScanForMinerAssets: Skipping miner asset with invalid mResourceClass {Field}",
					  GetNameSafe(Object));
			continue;
		}
		mMinerMapping.Add(Object->mResourceClass, Object);

		mAllowedScannableResources.Add(Object->mResourceClass);
	}
}

void UKAPIDataAssetSubsystem::ScanForSlugs()
{
	TSet<UKAPISugHatchingData*> Objects;
	FindAllDataAssetsOfClass(Objects);
	for (UKAPISugHatchingData* Object : Objects)
	{
		mSlugDatas.Add(Object->mSlug, Object);
		mSlugDatas.Add(Object->mEgg, Object);
	}
}

void UKAPIDataAssetSubsystem::ScanForWidgetInjector()
{
	if (IsRunningDedicatedServer())
	{
		return;
	}

	TSet<UKAPTooltipWidgetInjector*> Objects;
	FindAllDataAssetsOfClass(Objects);

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
	}
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
}
