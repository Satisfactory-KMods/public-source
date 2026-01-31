#include "Subsystems/KAPIDataAssetSubsystem.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "DataAssets/KAPIExtractorAllowList.h"

UKAPIDataAssetSubsystem::UKAPIDataAssetSubsystem()
{
}

UKAPIDataAssetSubsystem* UKAPIDataAssetSubsystem::Get(UObject* Context)
{
	if (IsValid(Context))
	{
		return Context->GetWorld()->GetGameInstance()->GetSubsystem<UKAPIDataAssetSubsystem>();
	}
	return nullptr;
}

UKAPIDataAssetSubsystem* UKAPIDataAssetSubsystem::GetChecked(UObject* Context)
{
	UKAPIDataAssetSubsystem* Subsystem = Get(Context);
	fgcheck(Subsystem);
	return Subsystem;
}

UModLoadingLibrary* UKAPIDataAssetSubsystem::GetModLoadingLibraryWithContext(UObject* Context)
{
	return Context->GetWorld()->GetGameInstance()->GetSubsystem<UModLoadingLibrary>();
}

void UKAPIDataAssetSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(
		FName("AssetRegistry"));
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
}

void UKAPIDataAssetSubsystem::ScanForMinerAssets()
{
	TSet<UKAPIModularMinerDescription*> Objects;
	FindAllDataAssetsOfClass(Objects);

	for (UKAPIModularMinerDescription* Object : Objects)
	{
		fgcheck(Object->mResourceClass);
		mMinerMapping.Add(Object->mResourceClass, Object);

		mAllowedScannableResources.Add(Object->mResourceClass);
	}
}

void UKAPIDataAssetSubsystem::ScanForCleanerAssets()
{
	TSet<UKAPICleanerItemDescription*> Objects;
	FindAllDataAssetsOfClass(Objects);

	for (UKAPICleanerItemDescription* Object : Objects)
	{
		fgcheck(Object->mInFluid.ItemClass);
		mCleanerItemMapping.Add(Object->mInFluid.ItemClass, Object);
	}
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

void UKAPIDataAssetSubsystem::ScanForWidgetInjector()
{
	TSet<UKAPTooltipWidgetInjector*> Objects;
	FindAllDataAssetsOfClass(Objects);

	for (UKAPTooltipWidgetInjector* Object : Objects)
	{
		fgcheck(Object->mWidgetClass);
		mTooltipWidgetsDataAssets.Add(Object);
	}
}

void UKAPIDataAssetSubsystem::ScanForAirCollectorDataAssets()
{
	TSet<UKAPIAirCollectorData*> Objects;
	FindAllDataAssetsOfClass(Objects);
	mAirCollectorDataAssets = Objects.Array();
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

TArray<UKAPISugHatchingData*> UKAPIDataAssetSubsystem::Slug_GetAll() const
{
	TArray<UKAPISugHatchingData*> OutArray;
	mSlugDatas.GenerateValueArray(OutArray);
	TSet<UKAPISugHatchingData*> UniqueSet(OutArray);
	OutArray = UniqueSet.Array();
	return OutArray;
}

bool UKAPIDataAssetSubsystem::Slug_GetForItem(TSubclassOf<UFGItemDescriptor> SlugOrEgg,
                                              UKAPISugHatchingData*& OutItem)
{
	UKAPISugHatchingData** Ptr = mSlugDatas.Find(SlugOrEgg);
	OutItem = nullptr;
	if (Ptr)
	{
		OutItem = *Ptr;
	}
	return IsValid(OutItem);
}

TArray<UKAPIAirCollectorData*> UKAPIDataAssetSubsystem::AirCollector_GetAll() const
{
	return mAirCollectorDataAssets;
}

bool UKAPIDataAssetSubsystem::Cleaner_GetForKey(TSubclassOf<UFGItemDescriptor> Item,
                                                UKAPICleanerItemDescription*& OutItem)
{
	UKAPICleanerItemDescription** Ptr = mCleanerItemMapping.Find(Item);
	OutItem = nullptr;
	if (Ptr)
	{
		OutItem = *Ptr;
	}
	return IsValid(OutItem);
}

TArray<UKAPICleanerItemDescription*> UKAPIDataAssetSubsystem::Cleaner_GetAllAssets()
{
	TArray<UKAPICleanerItemDescription*> OutItems;
	OutItems.Empty();
	mCleanerItemMapping.GenerateValueArray(OutItems);
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

TArray<UKAPIModularMinerDescription*> UKAPIDataAssetSubsystem::Miner_GetAllAssets()
{
	TArray<UKAPIModularMinerDescription*> OutItems;
	OutItems.Empty();
	mMinerMapping.GenerateValueArray(OutItems);
	return OutItems;
}

TArray<UKAPTooltipWidgetInjector*> UKAPIDataAssetSubsystem::GetAllTooltipWidgetInjectors()
{
	return mTooltipWidgetsDataAssets.Array();
}

TArray<UKAPIDataAssetBase*> UKAPIDataAssetSubsystem::K2_FindAllDataAssetsOfClass(
	TSubclassOf<UKAPIDataAssetBase> InClass)
{
	fgcheck(InClass->IsChildOf(UKAPIDataAssetBase::StaticClass()));

	TArray<UKAPIDataAssetBase*> OutDataAssets;
	OutDataAssets.Empty();

	// Find list of all UStat, and USkill assets in Content Browser.
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(
		FName("AssetRegistry"));
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

	rawObjects.Sort([&](const FSortiableAsset& A, const FSortiableAsset& B)
	{
		return A.mPriority > B.mPriority;
	});

	for (FSortiableAsset Str : rawObjects)
	{
		OutDataAssets.Add(Str.mObject);
	}

	rawObjects.Empty();

	UE_LOG(LogKApi, Warning, TEXT("Found %d of: %s"), OutDataAssets.Num(), *InClass->GetPathName());
	return OutDataAssets;
}
