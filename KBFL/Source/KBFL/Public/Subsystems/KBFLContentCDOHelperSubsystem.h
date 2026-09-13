// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "AssetRegistry/AssetRegistryModule.h"
#include "Containers/Ticker.h"
#include "CoreMinimal.h"
#include "HelperClasses/KBFLCDOOverwrite.h"
#include "Module/ModModule.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UObject/UObjectArray.h"

#include <atomic>

#include "KBFLContentCDOHelperSubsystem.generated.h"

UCLASS()
class KBFL_API UKBFLContentCDOHelperSubsystem : public UGameInstanceSubsystem,
												public FUObjectArray::FUObjectCreateListener
{
	GENERATED_BODY()

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	virtual void Deinitialize() override;

public:

	static UKBFLContentCDOHelperSubsystem* Get(UObject* Context);

	UFUNCTION()
	void OnTimerCallback();

	UFUNCTION(BlueprintPure, Category = "KMods", meta = (DeterminesOutputType = "Class"))
	UObject* GetAndStoreDefaultObject(UClass* Class);

	UFUNCTION(BlueprintPure, Category = "KMods", meta = (DeterminesOutputType = "Class", DefaultToSelf = "Context"))
	static UObject* GetAndStoreCDO(UClass* Class, UObject* Context);

	template <typename T>
	T* GetAndStoreDefaultObject_Native(UClass* Class);

	template <typename T>
	static T* GetAndStoreDefaultObject_Native(UClass* Class, UObject* Context);

	UFUNCTION(BlueprintCallable, Category = "KMods")
	void StoreClass(UClass* Class);

	UFUNCTION(BlueprintCallable, Category = "KMods")
	void StoreObject(UObject* Object);

	UFUNCTION(BlueprintCallable, Category = "KMods")
	void RemoveClass(UClass* Class);

	UFUNCTION(BlueprintCallable, Category = "KMods")
	void RemoveObject(UObject* Object);

	template <class T>
	bool FindAllDataAssetsOfClass(TSet<TObjectPtr<T>>& OutDataAssets);

	UFUNCTION()
	void WaitForWorldAndScheduleCallback();

	void EnsureCDOsApplied();

	virtual void NotifyUObjectCreated(const UObjectBase* Object, int32 Index) override;
	virtual void OnUObjectArrayShutdown() override;

private:

	void QueueLazyClassForProcessing(UClass* NewClass);

	bool ProcessPendingLazyClasses(float DeltaTime);

	bool Initialized = false;
	bool bCreateListenerRegistered = false;
	FDelegateHandle mWorldAddedHandle;
	FDelegateHandle mPreWorldInitHandle;
	FDelegateHandle mPackageLoadHandle;
	FTSTicker::FDelegateHandle mBootstrapTickerHandle;

	UPROPERTY()
	TObjectPtr<class UKBFLAssetDataSubsystem> mAssetDataSubsystem;

	UPROPERTY()
	TSet<TObjectPtr<UKBFLCDOOverwriteBase>> mCDOOverwritesToCall;

	UPROPERTY()
	TArray<TObjectPtr<UKBFLCDOOverwriteBase>> mNonWorldOverwritesToCall;

	UPROPERTY()
	TArray<TObjectPtr<UModModule>> mModulesToCall;

	std::atomic<bool> bHasNonWorldOverwrites{false};

	TSet<TWeakObjectPtr<UClass>> mPendingLazyClasses;
	TSet<TWeakObjectPtr<UClass>> mAppliedLazyClasses;
	FTSTicker::FDelegateHandle mLazyClassTickerHandle;

	UPROPERTY()
	TSet<TObjectPtr<UObject>> mCalledObjects;

	UPROPERTY()
	TSet<TObjectPtr<UClass>> mCalledClasses;
};

template <typename T>
T* UKBFLContentCDOHelperSubsystem::GetAndStoreDefaultObject_Native(UClass* Class)
{
#if WITH_EDITOR
	return Class->GetDefaultObject<T>();
#endif

	if (IsValid(Class))
	{
		mCalledClasses.Add(Class);
		mCalledObjects.Add(Class->GetDefaultObject());
		return Cast<T>(Class->GetDefaultObject());
	}
	return nullptr;
}

template <typename T>
T* UKBFLContentCDOHelperSubsystem::GetAndStoreDefaultObject_Native(UClass* Class, UObject* Context)
{
	if (UKBFLContentCDOHelperSubsystem* Sub = Get(Context))
	{
		return Sub->GetAndStoreDefaultObject_Native<T>(Class);
	}
	return nullptr;
}

template <class T>
bool UKBFLContentCDOHelperSubsystem::FindAllDataAssetsOfClass(TSet<TObjectPtr<T>>& OutDataAssets)
{
	OutDataAssets.Empty();

	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(FName("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FAssetData> AssetList;
	AssetRegistry.GetAssetsByClass(T::StaticClass()->GetClassPathName(), AssetList, true);

	for (const FAssetData& Asset : AssetList)
	{
		UObject* Obj = Asset.GetAsset();

		T* CastedAsset = Cast<T>(Obj);
		if (!CastedAsset)
		{
			UE_LOG(LogKBFLCDOOverwrite, Warning, TEXT("Invalid asset type: %s"), *Asset.AssetName.ToString());
			continue;
		}
		OutDataAssets.Add(CastedAsset);
	}

	UE_LOG(LogKBFLCDOOverwrite, Warning, TEXT("Found %d of: %s"), OutDataAssets.Num(),
		   *T::StaticClass()->GetPathName());
	return OutDataAssets.Num() > 0;
}
