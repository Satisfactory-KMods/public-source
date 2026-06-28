#pragma once

#include "AssetRegistry/AssetRegistryModule.h"
#include "CoreMinimal.h"
#include "HelperClasses/KBFLCDOOverwrite.h"
#include "Module/ModModule.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UObject/UObjectArray.h"

#include "KBFLContentCDOHelperSubsystem.generated.h"

/**
 *
 */
UCLASS()
class KBFL_API UKBFLContentCDOHelperSubsystem : public UGameInstanceSubsystem,
												public FUObjectArray::FUObjectCreateListener
{
	GENERATED_BODY()

	/** Implement this for initialization of instances of the system */
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

	/**
	 * Find all data assets of a specific class
	 * @param OutDataAssets - Set of data assets
	 * @return true if found any data assets
	 */
	template <class T>
	bool FindAllDataAssetsOfClass(TSet<TObjectPtr<T>>& OutDataAssets);

	/** Schedules OnTimerCallback once a valid game UWorld is available. */
	UFUNCTION()
	void WaitForWorldAndScheduleCallback();

	// Lazy-load CDO re-apply hooks (a class loaded after the initial pass).
	// Editor: OnEndLoadPackage (fires after objects are linked).
	// Shipping: FUObjectArray create listener — OnEndLoadPackage is WITH_EDITOR-only in the runtime loader.
	virtual void NotifyUObjectCreated(const UObjectBase* Object, int32 Index) override;
	virtual void OnUObjectArrayShutdown() override;

private:
	bool Initialized = false;
	FDelegateHandle mWorldAddedHandle;
	FDelegateHandle mPackageLoadHandle; // WITH_EDITOR only (OnEndLoadPackage)

	// Cached once in Initialize so the lazy-load listener doesn't GetSubsystem on every class.
	UPROPERTY()
	TObjectPtr<class UKBFLAssetDataSubsystem> mAssetDataSubsystem;

	UPROPERTY()
	TSet<TObjectPtr<UKBFLCDOOverwriteBase>> mCDOOverwritesToCall;

	// Cached subset of mCDOOverwritesToCall excluding world-based overwrites. The lazy-load listener
	// iterates this so it never has to Cast<UKBFLCDOOverwriteWorldBasedBase> + skip on every class.
	UPROPERTY()
	TArray<TObjectPtr<UKBFLCDOOverwriteBase>> mNonWorldOverwritesToCall;

	UPROPERTY()
	TArray<TObjectPtr<UModModule>> mModulesToCall;

	// TSet (not TArray) so Add/Remove are O(1) — these grow with every CDO class processed.
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

	// Find list of all UStat, and USkill assets in Content Browser.
	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(FName("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FAssetData> AssetList;
	AssetRegistry.GetAssetsByClass(T::StaticClass()->GetClassPathName(), AssetList, true);

	// Split assets into separate arrays.
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
