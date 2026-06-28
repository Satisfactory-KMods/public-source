// ILikeBanas

#pragma once

#include "AssetRegistry/AssetRegistryModule.h"
#include "Buildables/FGBuildableFactory.h"
#include "Buildables/FGBuildableResourceExtractorBase.h"
#include "CoreMinimal.h"
#include "DataAssets/KAPIAirCollectorData.h"
#include "DataAssets/KAPICleanerItemDescription.h"
#include "DataAssets/KAPIDataAssetBase.h"
#include "DataAssets/KAPIManufacturerModifications.h"
#include "DataAssets/KAPIModularMinerDescription.h"
#include "DataAssets/KAPISugHatchingData.h"
#include "DataAssets/KAPTooltipWidgetInjector.h"
#include "KAPIModule.h"
#include "ModLoading/ModLoadingLibrary.h"
#include "Resources/FGItemDescriptor.h"

#include "KAPIDataAssetSubsystem.generated.h"

UCLASS()
class KAPI_API UKAPIDataAssetSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UKAPIDataAssetSubsystem();

	//~ Begin USubsystem Interface
	/** Implement this for initialization of instances of the system */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	virtual void Deinitialize() override;
	//~ End USubsystem Interface

	UFUNCTION(BlueprintCallable, Category = "KAPI|DataAsset")
	TArray<UKAPIAirCollectorData*> AirCollector_GetAll() const;

	bool ApplyManufacturerAssign(AFGBuildableManufacturer* Manufacturer, TSubclassOf<class UFGRecipe> recipe);

	void ApplyManufacturerModifications(AFGBuildableManufacturer* Manufacturer, TSubclassOf<class UFGRecipe> recipe);

	UFUNCTION(BlueprintCallable, Category = "KAPI|DataAsset")
	TArray<UKAPICleanerItemDescription*> Cleaner_GetAllAssets();

	UFUNCTION(BlueprintCallable, Category = "KAPI|DataAsset")
	bool Cleaner_GetForKey(TSubclassOf<UFGItemDescriptor> Item, UKAPICleanerItemDescription*& OutItem);

	void EmptyAssets();

	/**
	 * Find all data assets of a specific class
	 * @param OutDataAssets - Set of data assets
	 * @return true if found any data assets
	 */
	template <class T>
	bool FindAllDataAssetsOfClass(TSet<T*>& OutDataAssets);

	template <class T>
	bool FindAllDataAssetsOfClass(TSet<TObjectPtr<T>>& OutDataAssets);

	/**
	 * Find all data assets of a specific class
	 * @param OutDataAssets - Set of data assets
	 * @return true if found any data assets
	 */
	template <class T>
	bool FindAllDataAssetsOfClassUnfiltered(TArray<T*>& OutDataAssets);

	static UKAPIDataAssetSubsystem* Get(UObject* Context);

	UFUNCTION(BlueprintCallable, Category = "KAPI|DataAsset",
			  meta = (DisplayName = "FindAllDataAssetsOfClass", DeterminesOutputType = "InClass"))
	TArray<UKAPTooltipWidgetInjector*> GetAllTooltipWidgetInjectors();

	static UKAPIDataAssetSubsystem* GetChecked(UObject* Context);

	UFUNCTION(BlueprintCallable, Category = "KAPI|DataAsset")
	UKAPIManufacturerModifications* GetModification(AFGBuildableManufacturer* Manufacturer);

	UFUNCTION(BlueprintPure, Category = "KAPI")
	static UModLoadingLibrary* GetModLoadingLibraryWithContext(UObject* Context);

	UFUNCTION(BlueprintCallable, Category = "KAPI",
			  meta = (DisplayName = "FindAllDataAssetsOfClass", DeterminesOutputType = "InClass"))
	TArray<UKAPIDataAssetBase*> K2_FindAllDataAssetsOfClass(TSubclassOf<UKAPIDataAssetBase> InClass);

	UFUNCTION(BlueprintCallable, Category = "KAPI|DataAsset")
	TArray<UKAPIModularMinerDescription*> Miner_GetAllAssets();

	UFUNCTION(BlueprintCallable, Category = "KAPI|DataAsset")
	bool Miner_GetForKey(TSubclassOf<UFGResourceDescriptor> Item, UKAPIModularMinerDescription*& OutItem);

	void ScanForAirCollectorDataAssets();
	void ScanForAllowList();
	void ScanForCleanerAssets();
	void ScanForManufacturerModifications();
	void ScanForMinerAssets();
	void ScanForSlugs();
	void ScanForWidgetInjector();

	UFUNCTION(BlueprintCallable, Category = "KAPI|DataAsset")
	TArray<UKAPISugHatchingData*> Slug_GetAll() const;

	UFUNCTION(BlueprintCallable, Category = "KAPI|DataAsset")
	bool Slug_GetForItem(TSubclassOf<UFGItemDescriptor> SlugOrEgg, UKAPISugHatchingData*& OutItem);

	/**
	 * Note: call this function only in the constructor of the subsystem OR if you change some mDisabled value in
	 * runtime to refresh cached data
	 */
	UFUNCTION(BlueprintCallable, Category = "KAPI|DataAsset")
	void StartScanForDataAssets();

	UPROPERTY(BlueprintReadOnly, Category = "KAPI|DataAsset")
	TSet<TObjectPtr<UKAPIDataAssetBase>> mDisabledDataAssets;

	UPROPERTY(BlueprintReadOnly, Category = "KAPI|DataAsset")
	TSet<TObjectPtr<UKAPIDataAssetBase>> mEnabledDataAssets;

	UPROPERTY(BlueprintReadOnly, Category = "KAPI|DataAsset")
	TSet<TObjectPtr<UKAPTooltipWidgetInjector>> mTooltipWidgetsDataAssets;

	UPROPERTY(BlueprintReadOnly, Category = "KAPI|DataAsset")
	TMap<TSubclassOf<UFGItemDescriptor>, TObjectPtr<UKAPICleanerItemDescription>> mCleanerItemMapping;

	UPROPERTY(BlueprintReadOnly, Category = "KAPI|DataAsset")
	TMap<TSubclassOf<UFGItemDescriptor>, TObjectPtr<UKAPISugHatchingData>> mSlugDatas;

	UPROPERTY(BlueprintReadOnly, Category = "KAPI|DataAsset")
	TMap<TSubclassOf<UFGResourceDescriptor>, TObjectPtr<UKAPIModularMinerDescription>> mMinerMapping;

	UPROPERTY(BlueprintReadOnly, Category = "KAPI|DataAsset")
	TSet<TSubclassOf<UFGResourceDescriptor>> mAllowedScannableResources;

	UPROPERTY(BlueprintReadOnly, Category = "KAPI|DataAsset")
	TSet<TSubclassOf<AFGBuildableResourceExtractorBase>> mAllowedResourceExtractors;

	UPROPERTY(BlueprintReadOnly, Category = "KAPI|DataAsset")
	TArray<TObjectPtr<UKAPIAirCollectorData>> mAirCollectorDataAssets;

	UPROPERTY(BlueprintReadOnly, Category = "KAPI|DataAsset")
	TSet<TObjectPtr<UKAPIManufacturerModifications>> mManufacturerModifications;
};

struct FSortiableAsset
{
	FSortiableAsset(UKAPIDataAssetBase* Object)
	{
		this->mPriority = Object->mPriority;
		this->mObject = Object;
	}

	int32 mPriority = 0;

	UKAPIDataAssetBase* mObject = nullptr;
};

template <class T>
bool UKAPIDataAssetSubsystem::FindAllDataAssetsOfClass(TSet<TObjectPtr<T>>& OutDataAssets)
{
	TSet<T*> TempSet;
	bool bResult = FindAllDataAssetsOfClass(TempSet);
	for (T* Item : TempSet)
	{
		OutDataAssets.Add(TObjectPtr<T>(Item));
	}
	return bResult;
}

template <class T>
bool UKAPIDataAssetSubsystem::FindAllDataAssetsOfClass(TSet<T*>& OutDataAssets)
{
	OutDataAssets.Empty();

	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(FName("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FAssetData> AssetList;
	AssetRegistry.GetAssetsByClass(T::StaticClass()->GetClassPathName(), AssetList, true);

	TArray<FSortiableAsset> rawObjects;

	for (const FAssetData& Asset : AssetList)
	{
		UObject* Obj = Asset.GetAsset();

		T* CastedAsset = Cast<T>(Obj);
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
		if (T* CastedAsset = Cast<T>(Str.mObject))
		{
			OutDataAssets.Add(Cast<T>(CastedAsset));
		}
	}

	rawObjects.Empty();

	UE_LOG(LogKApi, Warning, TEXT("Found %d of: %s"), OutDataAssets.Num(), *T::StaticClass()->GetPathName());
	return OutDataAssets.Num() > 0;
}

template <class T>
bool UKAPIDataAssetSubsystem::FindAllDataAssetsOfClassUnfiltered(TArray<T*>& OutDataAssets)
{
	OutDataAssets.Empty();

	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(FName("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FAssetData> AssetList;
	AssetRegistry.GetAssetsByClass(T::StaticClass()->GetClassPathName(), AssetList, true);

	TArray<FSortiableAsset> rawObjects;

	for (const FAssetData& Asset : AssetList)
	{
		UObject* Obj = Asset.GetAsset();

		T* CastedAsset = Cast<T>(Obj);
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
		if (T* CastedAsset = Cast<T>(Str.mObject))
		{
			OutDataAssets.Add(Cast<T>(CastedAsset));
		}
	}

	rawObjects.Empty();

	UE_LOG(LogKApi, Warning, TEXT("Found %d of: %s"), OutDataAssets.Num(), *T::StaticClass()->GetPathName());
	return OutDataAssets.Num() > 0;
}
