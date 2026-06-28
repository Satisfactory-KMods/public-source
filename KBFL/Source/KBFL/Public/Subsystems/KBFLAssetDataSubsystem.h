#pragma once

#include "AssetRegistry/AssetRegistryModule.h"
#include "CoreMinimal.h"
#include "FGDriveablePawn.h"
#include "FGResearchTree.h"
#include "FGSchematic.h"
#include "FGVehicle.h"
#include "KBFLLogging.h"
#include "Module/WorldModule.h"
#include "ResourceNodes/KBFLActorSpawnDescriptorBase.h"
#include "ResourceNodes/KBFLSubLevelSpawning.h"
#include "Resources/FGBuildingDescriptor.h"
#include "Resources/FGResourceDescriptor.h"
#include "Resources/FGVehicleDescriptor.h"
#include "SessionSettings/SessionSetting.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "KBFLAssetDataSubsystem.generated.h"

USTRUCT(Blueprintable)
struct FKBFLAssetData
{
	GENERATED_BODY()

	void cleanup()
	{
		mAllFoundedBuildables.Empty();
		mAllFoundedDriveablePawns.Empty();
		mAllFoundedHolograms.Empty();
		mAllFoundedItems.Empty();
		mAllFoundedModModules.Empty();
		mAllFoundedRecipes.Empty();
		mAllFoundedResourceDescriptors.Empty();
		mAllFoundedSchematics.Empty();
		mAllFoundResearchTrees.Empty();
		mAllFoundAGS.Empty();
	}

	void WriteClass(UClass* Class, int32 Type)
	{
		switch (Type)
		{
		case 0:
			mAllFoundedSchematics.Add(Class);
			break;
		case 1:
			mAllFoundedRecipes.Add(Class);
			break;
		case 2:
			mAllFoundedItems.Add(Class);
			break;
		case 3:
			mAllFoundedBuildables.Add(Class);
			break;
		case 4:
			mAllFoundedDriveablePawns.Add(Class);
			break;
		case 5:
			mAllFoundedHolograms.Add(Class);
			break;
		case 6:
			mAllFoundedModModules.Add(Class);
			break;
		case 8:
			mAllFoundedResourceDescriptors.Add(Class);
			break;
		case 10:
			mAllFoundResearchTrees.Add(Class);
			break;
		default:
			break;
		}
	}

	void WriteObject(UObject* Object, int32 Type)
	{
		switch (Type)
		{
		case 11:
			if (USMLSessionSetting* SessionSetting = Cast<USMLSessionSetting>(Object))
			{
				mAllFoundAGS.Add(SessionSetting);
			}
			break;
		default:
			break;
		}
	}

	UPROPERTY(BlueprintReadOnly)
	TSet<TObjectPtr<USMLSessionSetting>> mAllFoundAGS = {};

	UPROPERTY(BlueprintReadOnly)
	TSet<TObjectPtr<UClass>> mAllFoundResearchTrees = {};

	UPROPERTY(BlueprintReadOnly)
	TSet<TObjectPtr<UClass>> mAllFoundedSchematics = {};

	UPROPERTY(BlueprintReadOnly)
	TSet<TObjectPtr<UClass>> mAllFoundedRecipes = {};

	UPROPERTY(BlueprintReadOnly)
	TSet<TObjectPtr<UClass>> mAllFoundedItems = {};

	UPROPERTY(BlueprintReadOnly)
	TSet<TObjectPtr<UClass>> mAllFoundedBuildables = {};

	UPROPERTY(BlueprintReadOnly)
	TSet<TObjectPtr<UClass>> mAllFoundedDriveablePawns = {};

	UPROPERTY(BlueprintReadOnly)
	TSet<TObjectPtr<UClass>> mAllFoundedHolograms = {};

	UPROPERTY(BlueprintReadOnly)
	TSet<TObjectPtr<UClass>> mAllFoundedModModules = {};

	UPROPERTY(BlueprintReadOnly)
	TSet<TObjectPtr<UClass>> mAllFoundedResourceDescriptors = {};
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKBFLOnSchematicAdded, TSubclassOf<UFGSchematic>, Schematic);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKBFLOnRecipeAdded, TSubclassOf<UFGRecipe>, Recipe);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKBFLOnItemAdded, TSubclassOf<UFGItemDescriptor>, Item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKBFLOnBuildableAdded, TSubclassOf<AFGBuildable>, Buildable);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKBFLOnDriveablePawnAdded, TSubclassOf<AFGDriveablePawn>, DriveablePawn);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKBFLOnHologramAdded, TSubclassOf<AFGHologram>, Hologram);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKBFLOnModModuleAdded, TSubclassOf<UModModule>, ModModule);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKBFLOnResourceDescriptorAdded, TSubclassOf<UFGResourceDescriptor>,
											ResourceDescriptor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKBFLOnResearchTreeAdded, TSubclassOf<UFGResearchTree>, ResearchTree);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKBFLOnSessionSettingAdded, USMLSessionSetting*, SessionSetting);

// Single-cast variants used by BindOn* helpers — Blueprint wires these via "Create Event" nodes.
DECLARE_DYNAMIC_DELEGATE_OneParam(FKBFLOnSchematicAddedEvent, TSubclassOf<UFGSchematic>, Schematic);
DECLARE_DYNAMIC_DELEGATE_OneParam(FKBFLOnRecipeAddedEvent, TSubclassOf<UFGRecipe>, Recipe);
DECLARE_DYNAMIC_DELEGATE_OneParam(FKBFLOnItemAddedEvent, TSubclassOf<UFGItemDescriptor>, Item);
DECLARE_DYNAMIC_DELEGATE_OneParam(FKBFLOnBuildableAddedEvent, TSubclassOf<AFGBuildable>, Buildable);
DECLARE_DYNAMIC_DELEGATE_OneParam(FKBFLOnDriveablePawnAddedEvent, TSubclassOf<AFGDriveablePawn>, DriveablePawn);
DECLARE_DYNAMIC_DELEGATE_OneParam(FKBFLOnHologramAddedEvent, TSubclassOf<AFGHologram>, Hologram);
DECLARE_DYNAMIC_DELEGATE_OneParam(FKBFLOnModModuleAddedEvent, TSubclassOf<UModModule>, ModModule);
DECLARE_DYNAMIC_DELEGATE_OneParam(FKBFLOnResourceDescriptorAddedEvent, TSubclassOf<UFGResourceDescriptor>,
								  ResourceDescriptor);
DECLARE_DYNAMIC_DELEGATE_OneParam(FKBFLOnResearchTreeAddedEvent, TSubclassOf<UFGResearchTree>, ResearchTree);
DECLARE_DYNAMIC_DELEGATE_OneParam(FKBFLOnSessionSettingAddedEvent, USMLSessionSetting*, SessionSetting);


/**
 *
 */
UCLASS(Blueprintable)
class KBFL_API UKBFLAssetDataSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

	/** Implement this for initialization of instances of the system */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	virtual void Deinitialize() override;

	UFUNCTION()
	void ScanOnInitialize();

public:
	UFUNCTION(BlueprintCallable, Category = "Asset Data Subsystem")
	void DoScan(bool Force = false);

	/**
	 * Scan the asset registry for candidate Blueprint/DataAsset metadata WITHOUT loading any class.
	 * Cheap (registry metadata only). Class objects are loaded lazily, per category, on first query.
	 */
	void EnsureRegistryScanned();

	/**
	 * Resolve (load) a single asset category on demand and cache it. Type codes match WriteClass:
	 * 0=Schematics 1=Recipes 2=Items 3=Buildables 4=DriveablePawns 5=Holograms 6=ModModules
	 * 8=ResourceDescriptors 10=ResearchTrees 11=SessionSettings.
	 */
	void EnsureCategoryResolved(int32 Type);

	/** Resolve every category. Required by union (GetObjectsOfChilds) and per-mod (GetModRelatedData) queries. */
	void EnsureAllResolved();

	void setMapClass(UClass* Class, int32 Type);

	void setMapClass(UObject* Object, int32 Type);

	// NATIVE GETTER
	static UKBFLAssetDataSubsystem* Get(const UObject* WorldContext);
	static UKBFLAssetDataSubsystem* GetChecked(const UObject* WorldContext);

	void PrintFound();

	template <class T>
	void PrintArray(TSet<T> List);

	void InitAssetFinder();

	static bool FilterAsset(const FAssetData& AssetData);

	bool Local_FilterAsset(const FAssetData& AssetData) const;

	// Begin Items
	/** Get All Items that hit the Forms  */
	UFUNCTION(BlueprintCallable, Category = "Asset Data Subsystem")
	void GetItemsOfForms(TArray<EResourceForm> Forms, TArray<TSubclassOf<UFGItemDescriptor>>& Out_Items);

	/** Get All Items that hit the Child Classes  */
	UFUNCTION(BlueprintCallable, Category = "Asset Data Subsystem")
	void GetItemsOfChilds(TArray<UClass*> Childs, TArray<TSubclassOf<UFGItemDescriptor>>& Out_Items,
						  bool UseNativeCheck = false);

	/** Get All Items with a filter for Items & Equip  */
	UFUNCTION(BlueprintCallable, Category = "Asset Data Subsystem")
	void GetItemsFiltered(TArray<TSubclassOf<UFGItemDescriptor>>& Out_Items);

	/** Get All Items with a filter for Items & Equip & Forms  */
	UFUNCTION(BlueprintCallable, Category = "Asset Data Subsystem")
	void GetItemsFilteredWithForm(TArray<EResourceForm> Forms, TArray<TSubclassOf<UFGItemDescriptor>>& Out_Items);

	/** Get All Items that found while reading the game */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Asset Data Subsystem")
	TArray<TSubclassOf<UFGItemDescriptor>> GetAllItems();

	// END Items


	// Begin Schematics
	/** Get All Schematics that hit the Types  */
	UFUNCTION(BlueprintCallable, Category = "Asset Data Subsystem")
	void GetSchematicsOfTypes(TArray<ESchematicType> Types, TArray<TSubclassOf<UFGSchematic>>& Out_Schematics);

	/** Get All Items that hit the Child Classes  */
	UFUNCTION(BlueprintCallable, Category = "Asset Data Subsystem")
	void GetSchematicsOfChilds(TArray<UClass*> Childs, TArray<TSubclassOf<UFGSchematic>>& Out_Items,
							   bool UseNativeCheck = false);

	/** Get All Schematics that found while reading the game */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Asset Data Subsystem")
	TArray<TSubclassOf<UFGSchematic>> GetAllSchematics();

	// END Schematics


	// Begin Recipes
	/** Get All Items that hit the Child Classes  */
	UFUNCTION(BlueprintCallable, Category = "Asset Data Subsystem")
	void GetRecipesOfChilds(TArray<UClass*> Childs, TArray<TSubclassOf<UFGRecipe>>& Out_Items,
							bool UseNativeCheck = false);

	/** Get All Items that hit the Child Classes  */
	UFUNCTION(BlueprintCallable, Category = "Asset Data Subsystem")
	void GetRecipesOfProducer(TArray<TSubclassOf<UObject>> Producers, TArray<TSubclassOf<UFGRecipe>>& Out_Items);

	/** Get All Recipes that found while reading the game */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Asset Data Subsystem")
	TArray<TSubclassOf<UFGRecipe>> GetAllRecipes();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Asset Data Subsystem")
	TArray<TSubclassOf<UFGResearchTree>> GetAllResearchTrees();

	// END Recipes

	// Begin Buildables
	/** Get All Items that hit the Child Classes  */
	UFUNCTION(BlueprintCallable, Category = "Asset Data Subsystem")
	void GetBuildableOfChilds(TArray<UClass*> Childs, TArray<TSubclassOf<AFGBuildable>>& Out_Items,
							  bool UseNativeCheck = false);

	/** Get All Buildables that found while reading the game */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Asset Data Subsystem")
	TArray<TSubclassOf<AFGBuildable>> GetAllBuildable();

	// END Buildables


	// Start General Functions
	/** Check is a child of Array Class */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Asset Data Subsystem")
	static bool CheckChild(UClass* TestClass, TArray<UClass*> ClassesToTest, bool UseNativeCheck = false);

	/** Check has recipe producer of Array Class */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Asset Data Subsystem")
	static bool CheckHasRecipeProducer(TSubclassOf<UFGRecipe> TestClass, TArray<TSubclassOf<UObject>> Producers);


	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Asset Data Subsystem")
	TSubclassOf<UFGBuildingDescriptor> GetDescForBuildable(TSubclassOf<AFGBuildable> BuildableClass);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Asset Data Subsystem")
	TSubclassOf<UFGVehicleDescriptor> GetDescForVehicle(TSubclassOf<AFGVehicle> Class);

	// END General Functions


	/** Get All Objects that hit the Child Classes  */
	UFUNCTION(BlueprintCallable, Category = "Asset Data Subsystem")
	void GetObjectsOfChilds(TArray<UClass*> Childs, TArray<TSubclassOf<UObject>>& Out_Items,
							bool UseNativeCheck = false);

	template <class T>
	void GetObjectsOfChilds_Internal(TArray<UClass*> Childs, TArray<TSubclassOf<T>>& Out_Items,
									 bool UseNativeCheck = false);


	/** Get All DriveablePawns that hit the Child Classes  */
	UFUNCTION(BlueprintCallable, Category = "Asset Data Subsystem")
	void GetDriveablePawnsOfChilds(TArray<UClass*> Childs, TArray<TSubclassOf<AFGDriveablePawn>>& Out_Items,
								   bool UseNativeCheck = false);


	/** Get All Holograms that hit the Child Classes  */
	UFUNCTION(BlueprintCallable, Category = "Asset Data Subsystem")
	void GetHologramsOfChilds(TArray<UClass*> Childs, TArray<TSubclassOf<AFGHologram>>& Out_Items,
							  bool UseNativeCheck = false);


	/** Get All tModModules that hit the Child Classes  */
	UFUNCTION(BlueprintCallable, Category = "Asset Data Subsystem")
	void GetModModulesOfChilds(TArray<UClass*> Childs, TArray<TSubclassOf<UModModule>>& Out_Items,
							   bool UseNativeCheck = false);


	/** get data from a mod */
	UFUNCTION(BlueprintCallable, Category = "Asset Data Subsystem")
	FKBFLAssetData GetModRelatedData(UModModule* ModModule);

	/** Get All ResourceDescriptors that hit the Child Classes  */
	UFUNCTION(BlueprintCallable, Category = "Asset Data Subsystem")
	void GetResourceDescriptorsOfChilds(TArray<UClass*> Childs,
										TArray<TSubclassOf<UKBFLActorSpawnDescriptorBase>>& Out_Items,
										bool UseNativeCheck = false);

	/** True once the registry metadata scan has run. Does NOT imply any class is loaded. */
	inline static bool bWasInit = false;

	/** Return the Index where the ItemClass allowed on Slot in Inventory */
	template <class T>
	static bool GetAllClassesOfSubclass(const TArray<FAssetData>& AllAssets, TSet<TSubclassOf<T>>& OutClasses);


	/**
	 * Find all data assets of a specific class
	 * @param OutDataAssets - Set of data assets
	 * @return true if found any data assets
	 */
	template <class T>
	static bool FindAllDataAssetsOfClass(TSet<T*>& OutDataAssets);

	template <class T>
	static bool FindAllDataAssetsOfClass(TSet<TObjectPtr<T>>& OutDataAssets);

	template <class T>
	static bool FindFirstDataAssetsOfClass(T*& OutDataAssets);

	// Fired per-class as async-loaded BP classes arrive after initial sync resolution.
	UPROPERTY(BlueprintAssignable, Category = "Asset Data Subsystem|Events")
	FKBFLOnSchematicAdded OnSchematicAdded;

	UPROPERTY(BlueprintAssignable, Category = "Asset Data Subsystem|Events")
	FKBFLOnRecipeAdded OnRecipeAdded;

	UPROPERTY(BlueprintAssignable, Category = "Asset Data Subsystem|Events")
	FKBFLOnItemAdded OnItemAdded;

	UPROPERTY(BlueprintAssignable, Category = "Asset Data Subsystem|Events")
	FKBFLOnBuildableAdded OnBuildableAdded;

	UPROPERTY(BlueprintAssignable, Category = "Asset Data Subsystem|Events")
	FKBFLOnDriveablePawnAdded OnDriveablePawnAdded;

	UPROPERTY(BlueprintAssignable, Category = "Asset Data Subsystem|Events")
	FKBFLOnHologramAdded OnHologramAdded;

	// ===== Bind + replay helpers =====
	// Bind a typed callback and immediately invoke it for every already-loaded asset of that type.
	// Use these instead of binding the multicast delegates directly so late subscribers do not miss
	// assets that arrived before they bound.

	UFUNCTION(BlueprintCallable, Category = "Asset Data Subsystem|Events")
	void BindOnSchematicAdded(FKBFLOnSchematicAddedEvent Delegate, bool bEnsureLoaded = false);

	UFUNCTION(BlueprintCallable, Category = "Asset Data Subsystem|Events")
	void BindOnRecipeAdded(FKBFLOnRecipeAddedEvent Delegate, bool bEnsureLoaded = false);

	UFUNCTION(BlueprintCallable, Category = "Asset Data Subsystem|Events")
	void BindOnItemAdded(FKBFLOnItemAddedEvent Delegate, bool bEnsureLoaded = false);

	UFUNCTION(BlueprintCallable, Category = "Asset Data Subsystem|Events")
	void BindOnBuildableAdded(FKBFLOnBuildableAddedEvent Delegate, bool bEnsureLoaded = false);

	UFUNCTION(BlueprintCallable, Category = "Asset Data Subsystem|Events")
	void BindOnDriveablePawnAdded(FKBFLOnDriveablePawnAddedEvent Delegate, bool bEnsureLoaded = false);

	UFUNCTION(BlueprintCallable, Category = "Asset Data Subsystem|Events")
	void BindOnHologramAdded(FKBFLOnHologramAddedEvent Delegate, bool bEnsureLoaded = false);

	UFUNCTION(BlueprintCallable, Category = "Asset Data Subsystem|Events")
	void BindOnModModuleAdded(FKBFLOnModModuleAddedEvent Delegate, bool bEnsureLoaded = false);

	UFUNCTION(BlueprintCallable, Category = "Asset Data Subsystem|Events")
	void BindOnResourceDescriptorAdded(FKBFLOnResourceDescriptorAddedEvent Delegate, bool bEnsureLoaded = false);

	UFUNCTION(BlueprintCallable, Category = "Asset Data Subsystem|Events")
	void BindOnResearchTreeAdded(FKBFLOnResearchTreeAddedEvent Delegate, bool bEnsureLoaded = false);

	UFUNCTION(BlueprintCallable, Category = "Asset Data Subsystem|Events")
	void BindOnSessionSettingAdded(FKBFLOnSessionSettingAddedEvent Delegate, bool bEnsureLoaded = false);

	/**
	 * Notify the subsystem that a class was (lazily) loaded. Classifies it into its category and routes to
	 * AddAsyncLoadedClassToCategory, which only adds + broadcasts the matching OnXAdded event if it is new.
	 * Called by UKBFLContentCDOHelperSubsystem when a class is first seen at runtime.
	 */
	void NotifyClassLoaded(UClass* Class);

	UPROPERTY(BlueprintAssignable, Category = "Asset Data Subsystem|Events")
	FKBFLOnModModuleAdded OnModModuleAdded;

	UPROPERTY(BlueprintAssignable, Category = "Asset Data Subsystem|Events")
	FKBFLOnResourceDescriptorAdded OnResourceDescriptorAdded;

	UPROPERTY(BlueprintAssignable, Category = "Asset Data Subsystem|Events")
	FKBFLOnResearchTreeAdded OnResearchTreeAdded;

	UPROPERTY(BlueprintAssignable, Category = "Asset Data Subsystem|Events")
	FKBFLOnSessionSettingAdded OnSessionSettingAdded;

private:
	/** Add an already-resolved category's classes to the union set + per-mod directory map. */
	void IndexResolvedCategory(int32 Type);

	/** Registry metadata for candidate assets - filled by EnsureRegistryScanned, holds NO loaded classes. */
	TArray<FAssetData> mScannedAssets;

	/** Set of category Type codes already resolved (loaded + indexed), to avoid re-loading. */
	TSet<int32> mResolvedTypes;

	/** Scan mScannedAssets for unloaded BP classes of the given category and kick off async loads. */
	void TriggerAsyncLoadsForCategory(int32 Type);
	void OnCategoryAsyncLoaded(int32 Type);
	void AddAsyncLoadedClassToCategory(UClass* Class, int32 Type);
	void BroadcastCategoryEvent(UClass* Class, int32 Type);

	/** Paths queued for async loading per category type — cleared when each batch completes. */
	TMap<int32, TArray<FSoftObjectPath>> mPendingAsyncLoads;

	UPROPERTY()
	TSet<TSubclassOf<UFGSchematic>> mAllFoundedSchematics;

	UPROPERTY()
	TSet<TSubclassOf<UFGRecipe>> mAllFoundedRecipes;

	UPROPERTY()
	TSet<TSubclassOf<UFGItemDescriptor>> mAllFoundedItems;

	UPROPERTY()
	TSet<TSubclassOf<AFGBuildable>> mAllFoundedBuildables;

	UPROPERTY()
	TSet<TSubclassOf<AFGDriveablePawn>> mAllFoundedDriveablePawns;

	UPROPERTY()
	TSet<TSubclassOf<AFGHologram>> mAllFoundedHolograms;

	UPROPERTY()
	TSet<TSubclassOf<UModModule>> mAllFoundedModModules;

	UPROPERTY()
	TSet<TSubclassOf<UFGResourceDescriptor>> mAllFoundedResourceDescriptors;

	UPROPERTY()
	TSet<TObjectPtr<UClass>> mAllFoundedObjects;

	UPROPERTY()
	TSet<TSubclassOf<UFGResearchTree>> mAllFoundResearchTrees;

	UPROPERTY()
	TSet<TObjectPtr<USMLSessionSetting>> mAllFoundAGS;

	UPROPERTY()
	TMap<FName, FKBFLAssetData> mDirectoryMappings;

public:
	TMap<UClass*, FAssetData> mAssetClassMap;

	// Small fix for PassiveMode
	TArray<FString> mPreventStrings = {"/PassiveMode/"};
};

#pragma warning(push)
#pragma warning(disable : 4702)
template <class T>
void UKBFLAssetDataSubsystem::PrintArray(TSet<T> List)
{
	return; // Disbaled for debug reasons
	for (UClass* Class : List)
	{
		UE_LOG(AssetDataSubsystemLog, Log, TEXT("Class: %s"), *Class->GetClassPathName().ToString());
	}
}
#pragma warning(pop)

template <class T>
void UKBFLAssetDataSubsystem::GetObjectsOfChilds_Internal(const TArray<UClass*> Childs,
														  TArray<TSubclassOf<T>>& Out_Items, bool UseNativeCheck)
{
	// Union query needs every category loaded - resolve lazily on first use.
	EnsureAllResolved();

	for (UClass* FoundedClass : mAllFoundedObjects)
	{
		if (TSubclassOf<UObject> Class = FoundedClass)
		{
			if (CheckChild(Class, Childs, UseNativeCheck))
			{
				if (TSubclassOf<T> CastObject = TSubclassOf<T>(Class))
				{
					Out_Items.Add(CastObject);
				}
			}
		}
	}
}

template <class T>
bool UKBFLAssetDataSubsystem::GetAllClassesOfSubclass(const TArray<FAssetData>& AllAssets,
													  TSet<TSubclassOf<T>>& OutClasses)
{
	const UClass* TargetClass = T::StaticClass();

	for (const FAssetData& AssetData : AllAssets)
	{
		// Early exit: Check if this asset could potentially be a child of our target class
		// using NativeParentClass tag (much faster than loading the class)
		FString NativeParentClassPath;
		if (AssetData.GetTagValue(FBlueprintTags::NativeParentClassPath, NativeParentClassPath))
		{
			// Parse the native parent class path
			FString NativeClassName;
			FString NativeClassObjectPath;
			if (FPackageName::ParseExportTextPath(NativeParentClassPath, &NativeClassName, &NativeClassObjectPath))
			{
				// FindObject only: native C++ classes are always in memory
				if (UClass* NativeClass = FindObject<UClass>(nullptr, *NativeClassObjectPath))
				{
					if (!NativeClass->IsChildOf(TargetClass))
					{
						continue; // Skip this asset, native parent isn't related to target
					}
				}
			}
		}

		// Handle UBlueprintGeneratedClass assets directly
		if (AssetData.AssetClassPath == FTopLevelAssetPath(UBlueprintGeneratedClass::StaticClass()))
		{
			TSoftClassPtr<UObject> SoftClass = TSoftClassPtr(FSoftObjectPath(AssetData.GetObjectPathString()));
			if (SoftClass.IsValid())
			{
				UClass* LoadedClass = SoftClass.Get();
				if (LoadedClass && LoadedClass->IsChildOf(TargetClass))
				{
					OutClasses.Add(LoadedClass);
				}
			}
			continue;
		}

		// Handle UBlueprint assets
		if (AssetData.AssetClassPath != FTopLevelAssetPath(UBlueprint::StaticClass()))
		{
			continue;
		}

		// Retrieve GeneratedClass tag
		FString GeneratedClassExportedPath;
		if (!AssetData.GetTagValue(FBlueprintTags::GeneratedClassPath, GeneratedClassExportedPath))
		{
			continue; // Skip if no generated class path
		}

		// Parse export path
		FString GeneratedClassPath;
		if (!FPackageName::ParseExportTextPath(GeneratedClassExportedPath, nullptr, &GeneratedClassPath))
		{
			continue; // Skip if parsing fails
		}

		// FindObject only: force-loading BP classes here corrupts CSS's replication graph init
		UClass* ClassObject = FindObject<UClass>(nullptr, *GeneratedClassPath);
		if (ClassObject && ClassObject->IsChildOf(TargetClass))
		{
			OutClasses.Add(ClassObject);
		}
	}

	return !OutClasses.IsEmpty();
}

template <class T>
bool UKBFLAssetDataSubsystem::FindAllDataAssetsOfClass(TSet<T*>& OutDataAssets)
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
			UE_LOG(AssetDataSubsystemLog, Warning, TEXT("Invalid asset type: %s"), *Asset.AssetName.ToString());
			continue;
		}

		OutDataAssets.Add(CastedAsset);
	}

	return OutDataAssets.Num() > 0;
}
template <class T>
bool UKBFLAssetDataSubsystem::FindAllDataAssetsOfClass(TSet<TObjectPtr<T>>& OutDataAssets)
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
bool UKBFLAssetDataSubsystem::FindFirstDataAssetsOfClass(T*& OutDataAssets)
{
	OutDataAssets = nullptr;
	TSet<T*> DataAssets;
	if (UKBFLAssetDataSubsystem::FindAllDataAssetsOfClass(DataAssets))
	{
		OutDataAssets = *DataAssets.CreateConstIterator();
	}
	return IsValid(OutDataAssets);
}
