#pragma once

#include "CoreMinimal.h"
#include "HelperClasses/KBFLCDOOverwrite.h"
#include "HelperClasses/KBFL_CDOHelperClass_CategoryOrder.h"
#include "Interfaces/KBFLContentCDOHelperInterface.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "KBFLContentCDOHelperSubsystem.generated.h"

/**
 *
 */
UCLASS()
class KBFL_API UKBFLContentCDOHelperSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

	/** Implement this for initialization of instances of the system */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

public:
	static UKBFLContentCDOHelperSubsystem* Get(UObject* Context);

	UFUNCTION()
	void OnTimerCallback();

	/** Moves all Recipes from a building to a other */
	UFUNCTION(BlueprintCallable, Category = "KMods")
	void MoveRecipesFromBuilding(TSoftClassPtr<UObject> From, TSoftClassPtr<UObject> To,
								 TArray<TSubclassOf<UFGItemCategory>> IgnoreCategory,
								 TArray<TSubclassOf<UFGRecipe>> IgnoreRecipe);

	UFUNCTION(BlueprintCallable, Category = "KMods")
	void BeginCDOForModule(UModModule* Module, ELifecyclePhase Phase);

	UFUNCTION(BlueprintCallable, Category = "KMods")
	bool WasCDOForModuleCalled(UModModule* Module, ELifecyclePhase Phase) const;

	UFUNCTION(BlueprintCallable, Category = "KMods")
	void ResetCDOCallFromModule(UModModule* Module);

	UFUNCTION(BlueprintCallable, Category = "KMods")
	void DoCDOFromInfo(FKBFLCDOInformation Info);

	UFUNCTION(BlueprintCallable, Category = "KMods")
	void DoSetNewStackSize(TSubclassOf<UFGItemDescriptor> Item, EStackSize StackSize);

	UFUNCTION(BlueprintCallable, Category = "KMods")
	void CallCDOHelper(TSubclassOf<UKBFL_CDOHelperClass_Base> CDOHelperClass, bool IgnoreCallCheck = false);

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

	UFUNCTION(BlueprintPure)
	TArray<UModModule*> GetCalledModules() const;

	/**
	 * Find all data assets of a specific class
	 * @param OutDataAssets - Set of data assets
	 * @return true if found any data assets
	 */
	template <class T>
	bool FindAllDataAssetsOfClass(TSet<T*>& OutDataAssets);

private:
	bool Initialized = false;

	UPROPERTY()
	TSet<UKBFLCDOOverwriteBase*> mCDOOverwritesToCall;

	UPROPERTY()
	TArray<UKBFL_CDOHelperClass_CategoryOrder*> mCategoriesToCall;

	UPROPERTY()
	TArray<UModModule*> mModulesToCall;

	UPROPERTY()
	TMap<UModModule*, FKBFLPhases> mCDOCalled;

	UPROPERTY()
	TArray<UObject*> mCalledObjects;

	UPROPERTY()
	TArray<UClass*> mCalledClasses;
};

template <typename T>
T* UKBFLContentCDOHelperSubsystem::GetAndStoreDefaultObject_Native(UClass* Class)
{
	if (IsValid(Class))
	{
		mCalledClasses.AddUnique(Class);
		mCalledObjects.AddUnique(Class->GetDefaultObject());
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
bool UKBFLContentCDOHelperSubsystem::FindAllDataAssetsOfClass(TSet<T*>& OutDataAssets)
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
