#pragma once

#include "AssetRegistry/AssetRegistryModule.h"
#include "Buildables/FGBuildableFactory.h"
#include "Buildables/FGBuildableResourceExtractorBase.h"
#include "CoreMinimal.h"
#include "DataAssets/KAPIAirCollectorData.h"
#include "DataAssets/KAPICleanerItemDescription.h"
#include "DataAssets/KAPIDataAssetBase.h"
#include "DataAssets/KAPIDeliveryTask.h"
#include "DataAssets/KAPIManufacturerModifications.h"
#include "DataAssets/KAPIModularMinerDescription.h"
#include "DataAssets/KAPISugHatchingData.h"
#include "DataAssets/KAPTooltipWidgetInjector.h"
#include "Engine/GameInstance.h"
#include "KAPIModule.h"
#include "ModLoading/ModLoadingLibrary.h"
#include "Resources/FGItemDescriptor.h"

#include "KAPIDataAssetSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE(FKAPIOnDeliveryTasksChanged);

UCLASS()
class KAPI_API UKAPIDataAssetSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UKAPIDataAssetSubsystem();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "KAPI|DataAsset")
	TArray<UKAPIAirCollectorData*> AirCollector_GetAll() const;

	bool ApplyManufacturerAssign(AFGBuildableManufacturer* Manufacturer, TSubclassOf<class UFGRecipe> recipe);

	void ApplyManufacturerModifications(AFGBuildableManufacturer* Manufacturer, TSubclassOf<class UFGRecipe> recipe);

	UFUNCTION(BlueprintCallable, Category = "KAPI|DataAsset")
	TArray<UKAPICleanerItemDescription*> Cleaner_GetAllAssets();

	UFUNCTION(BlueprintCallable, Category = "KAPI|DataAsset")
	bool Cleaner_GetForKey(TSubclassOf<UFGItemDescriptor> Item, UKAPICleanerItemDescription*& OutItem);

	void EmptyAssets();

	template <class T>
	bool FindAllDataAssetsOfClass(TSet<T*>& OutDataAssets);

	template <class T>
	bool FindAllDataAssetsOfClass(TSet<TObjectPtr<T>>& OutDataAssets);

	template <class T>
	bool FindAllDataAssetsOfClass(TArray<T*>& OutDataAssets);

	template <class T>
	bool FindAllDataAssetsOfClassUnfiltered(TArray<T*>& OutDataAssets);

	static UKAPIDataAssetSubsystem* Get(UObject* Context);

	template <class T>
	static T* GetGameInstanceSubsystemFromContext(UObject* Context);

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
	void ScanForDeliveryTasks();

	UFUNCTION(BlueprintCallable, Category = "KAPI|DataAsset")
	TArray<UKAPISugHatchingData*> Slug_GetAll() const;

	UFUNCTION(BlueprintCallable, Category = "KAPI|DataAsset")
	bool Slug_GetForItem(TSubclassOf<UFGItemDescriptor> SlugOrEgg, UKAPISugHatchingData*& OutItem);

	UFUNCTION(BlueprintCallable, Category = "KAPI|DataAsset")
	TMap<FIntVector2, UKAPIDeliveryTask*> Delivery_GetAll() const;

	const TMap<FIntVector2, TObjectPtr<UKAPIDeliveryTask>>& Delivery_GetAllView() const { return mDeliveryTasks; }

	UFUNCTION(BlueprintPure, Category = "KAPI|DataAsset")
	bool Delivery_HasAny() const { return !mDeliveryTasks.IsEmpty(); }

	uint32 Delivery_GetRevision() const { return mDeliveryTaskRevision; }
	bool Delivery_HasCompletedScan() const { return bHasCompletedDeliveryTaskScan; }

	FKAPIOnDeliveryTasksChanged mOnDeliveryTasksChanged;

	UFUNCTION(BlueprintCallable, Category = "KAPI|DataAsset")
	void StartScanForDataAssets();

	UPROPERTY(BlueprintReadOnly, Category = "KAPI|DataAsset")
	TSet<TObjectPtr<UKAPIDataAssetBase>> mDisabledDataAssets;

	UPROPERTY(BlueprintReadOnly, Category = "KAPI|DataAsset")
	TSet<TObjectPtr<UKAPIDataAssetBase>> mEnabledDataAssets;

	UPROPERTY(BlueprintReadOnly, Category = "KAPI|DataAsset")
	TSet<TObjectPtr<UKAPTooltipWidgetInjector>> mTooltipWidgetsDataAssets;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UKAPTooltipWidgetInjector>> mOrderedTooltipWidgetsDataAssets;

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

	UPROPERTY(BlueprintReadOnly, Category = "KAPI|DataAsset")
	TMap<FIntVector2, TObjectPtr<UKAPIDeliveryTask>> mDeliveryTasks;

	uint32 mDeliveryTaskRevision = 0;
	bool bHasCompletedDeliveryTaskScan = false;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UKAPIManufacturerModifications>> mOrderedManufacturerModifications;

protected:

	void CollectDataAssetsOfClass(const UClass* AssetClass, TArray<UKAPIDataAssetBase*>& OutSortedAssets);
};

struct FSortiableAsset
{
	FSortiableAsset(UKAPIDataAssetBase* Object)
	{
		this->mPriority = Object->mPriority;
		this->mObject = Object;
		this->mObjectPath = Object->GetPathName();
	}

	int32 mPriority = 0;

	UKAPIDataAssetBase* mObject = nullptr;

	FString mObjectPath;

	static bool SortByPriorityAndPath(const FSortiableAsset& A, const FSortiableAsset& B)
	{
		if (A.mPriority != B.mPriority)
		{
			return A.mPriority > B.mPriority;
		}

		return A.mObjectPath.Compare(B.mObjectPath, ESearchCase::CaseSensitive) < 0;
	}
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
	TArray<T*> OrderedDataAssets;
	const bool bResult = FindAllDataAssetsOfClass(OrderedDataAssets);
	for (T* Item : OrderedDataAssets)
	{
		OutDataAssets.Add(Item);
	}
	return bResult;
}

template <class T>
bool UKAPIDataAssetSubsystem::FindAllDataAssetsOfClass(TArray<T*>& OutDataAssets)
{
	OutDataAssets.Empty();

	TArray<UKAPIDataAssetBase*> SortedAssets;
	CollectDataAssetsOfClass(T::StaticClass(), SortedAssets);

	OutDataAssets.Reserve(SortedAssets.Num());
	for (UKAPIDataAssetBase* Asset : SortedAssets)
	{
		if (T* CastedAsset = Cast<T>(Asset))
		{
			OutDataAssets.Add(CastedAsset);
		}
	}

	UE_LOG(LogKApi, Warning, TEXT("Found %d of: %s"), OutDataAssets.Num(), *T::StaticClass()->GetPathName());
	return OutDataAssets.Num() > 0;
}

template <class T>
bool UKAPIDataAssetSubsystem::FindAllDataAssetsOfClassUnfiltered(TArray<T*>& OutDataAssets)
{
	return FindAllDataAssetsOfClass(OutDataAssets);
}

template <class T>
T* UKAPIDataAssetSubsystem::GetGameInstanceSubsystemFromContext(UObject* Context)
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

	return GameInstance->GetSubsystem<T>();
}
