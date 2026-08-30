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

UCLASS(Blueprintable)
class KBFL_API UKBFLAssetDataSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	virtual void Deinitialize() override;

	UFUNCTION()
	void ScanOnInitialize();

public:
	UFUNCTION(BlueprintCallable, Category = "Asset Data Subsystem")
	void DoScan(bool Force = false);

	void EnsureRegistryScanned();

	void EnsureCategoryResolved(int32 Type);

	void EnsureAllResolved();

	void setMapClass(UClass* Class, int32 Type);

	void setMapClass(UObject* Object, int32 Type);

	static UKBFLAssetDataSubsystem* Get(const UObject* WorldContext);
	static UKBFLAssetDataSubsystem* GetChecked(const UObject* WorldContext);

	void PrintFound();

	template <class T>
	void PrintArray(TSet<T> List);

	void InitAssetFinder();

	static bool FilterAsset(const FAssetData& AssetData);

	bool Local_FilterAsset(const FAssetData& AssetData) const;

	UFUNCTION(BlueprintCallable, Category = "Asset Data Subsystem")
	void GetItemsOfForms(TArray<EResourceForm> Forms, TArray<TSubclassOf<UFGItemDescriptor>>& Out_Items);

	UFUNCTION(BlueprintCallable, Category = "Asset Data Subsystem")
	void GetItemsOfChilds(TArray<UClass*> Childs, TArray<TSubclassOf<UFGItemDescriptor>>& Out_Items,
						  bool UseNativeCheck = false);

	UFUNCTION(BlueprintCallable, Category = "Asset Data Subsystem")
	void GetItemsFiltered(TArray<TSubclassOf<UFGItemDescriptor>>& Out_Items);

	UFUNCTION(BlueprintCallable, Category = "Asset Data Subsystem")
	void GetItemsFilteredWithForm(TArray<EResourceForm> Forms, TArray<TSubclassOf<UFGItemDescriptor>>& Out_Items);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Asset Data Subsystem")
	TArray<TSubclassOf<UFGItemDescriptor>> GetAllItems();

	UFUNCTION(BlueprintCallable, Category = "Asset Data Subsystem")
	void GetSchematicsOfTypes(TArray<ESchematicType> Types, TArray<TSubclassOf<UFGSchematic>>& Out_Schematics);

	UFUNCTION(BlueprintCallable, Category = "Asset Data Subsystem")
	void GetSchematicsOfChilds(TArray<UClass*> Childs, TArray<TSubclassOf<UFGSchematic>>& Out_Items,
							   bool UseNativeCheck = false);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Asset Data Subsystem")
	TArray<TSubclassOf<UFGSchematic>> GetAllSchematics();

	UFUNCTION(BlueprintCallable, Category = "Asset Data Subsystem")
	void GetRecipesOfChilds(TArray<UClass*> Childs, TArray<TSubclassOf<UFGRecipe>>& Out_Items,
							bool UseNativeCheck = false);

	UFUNCTION(BlueprintCallable, Category = "Asset Data Subsystem")
	void GetRecipesOfProducer(TArray<TSubclassOf<UObject>> Producers, TArray<TSubclassOf<UFGRecipe>>& Out_Items);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Asset Data Subsystem")
	TArray<TSubclassOf<UFGRecipe>> GetAllRecipes();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Asset Data Subsystem")
	TArray<TSubclassOf<UFGResearchTree>> GetAllResearchTrees();

	UFUNCTION(BlueprintCallable, Category = "Asset Data Subsystem")
	void GetBuildableOfChilds(TArray<UClass*> Childs, TArray<TSubclassOf<AFGBuildable>>& Out_Items,
							  bool UseNativeCheck = false);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Asset Data Subsystem")
	TArray<TSubclassOf<AFGBuildable>> GetAllBuildable();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Asset Data Subsystem")
	static bool CheckChild(UClass* TestClass, TArray<UClass*> ClassesToTest, bool UseNativeCheck = false);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Asset Data Subsystem")
	static bool CheckHasRecipeProducer(TSubclassOf<UFGRecipe> TestClass, TArray<TSubclassOf<UObject>> Producers);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Asset Data Subsystem")
	TSubclassOf<UFGBuildingDescriptor> GetDescForBuildable(TSubclassOf<AFGBuildable> BuildableClass);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Asset Data Subsystem")
	TSubclassOf<UFGVehicleDescriptor> GetDescForVehicle(TSubclassOf<AFGVehicle> Class);

	UFUNCTION(BlueprintCallable, Category = "Asset Data Subsystem")
	void GetObjectsOfChilds(TArray<UClass*> Childs, TArray<TSubclassOf<UObject>>& Out_Items,
							bool UseNativeCheck = false);

	template <class T>
	void GetObjectsOfChilds_Internal(TArray<UClass*> Childs, TArray<TSubclassOf<T>>& Out_Items,
									 bool UseNativeCheck = false);

	UFUNCTION(BlueprintCallable, Category = "Asset Data Subsystem")
	void GetDriveablePawnsOfChilds(TArray<UClass*> Childs, TArray<TSubclassOf<AFGDriveablePawn>>& Out_Items,
								   bool UseNativeCheck = false);

	UFUNCTION(BlueprintCallable, Category = "Asset Data Subsystem")
	void GetHologramsOfChilds(TArray<UClass*> Childs, TArray<TSubclassOf<AFGHologram>>& Out_Items,
							  bool UseNativeCheck = false);

	UFUNCTION(BlueprintCallable, Category = "Asset Data Subsystem")
	void GetModModulesOfChilds(TArray<UClass*> Childs, TArray<TSubclassOf<UModModule>>& Out_Items,
							   bool UseNativeCheck = false);

	UFUNCTION(BlueprintCallable, Category = "Asset Data Subsystem")
	FKBFLAssetData GetModRelatedData(UModModule* ModModule);

	UFUNCTION(BlueprintCallable, Category = "Asset Data Subsystem")
	void GetResourceDescriptorsOfChilds(TArray<UClass*> Childs,
										TArray<TSubclassOf<UKBFLActorSpawnDescriptorBase>>& Out_Items,
										bool UseNativeCheck = false);

	inline static bool bWasInit = false;

	template <class T>
	static bool GetAllClassesOfSubclass(const TArray<FAssetData>& AllAssets, TSet<TSubclassOf<T>>& OutClasses);

	template <class T>
	static bool FindAllDataAssetsOfClass(TSet<T*>& OutDataAssets);

	template <class T>
	static bool FindAllDataAssetsOfClass(TSet<TObjectPtr<T>>& OutDataAssets);

	template <class T>
	static bool FindFirstDataAssetsOfClass(T*& OutDataAssets);

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

	UFUNCTION(BlueprintCallable, Category = "Asset Data Subsystem|Events")
	void BindOnSchematicAdded(FKBFLOnSchematicAddedEvent Delegate, bool bEnsureLoaded = true);

	UFUNCTION(BlueprintCallable, Category = "Asset Data Subsystem|Events")
	void BindOnRecipeAdded(FKBFLOnRecipeAddedEvent Delegate, bool bEnsureLoaded = true);

	UFUNCTION(BlueprintCallable, Category = "Asset Data Subsystem|Events")
	void BindOnItemAdded(FKBFLOnItemAddedEvent Delegate, bool bEnsureLoaded = true);

	UFUNCTION(BlueprintCallable, Category = "Asset Data Subsystem|Events")
	void BindOnBuildableAdded(FKBFLOnBuildableAddedEvent Delegate, bool bEnsureLoaded = true);

	UFUNCTION(BlueprintCallable, Category = "Asset Data Subsystem|Events")
	void BindOnDriveablePawnAdded(FKBFLOnDriveablePawnAddedEvent Delegate, bool bEnsureLoaded = true);

	UFUNCTION(BlueprintCallable, Category = "Asset Data Subsystem|Events")
	void BindOnHologramAdded(FKBFLOnHologramAddedEvent Delegate, bool bEnsureLoaded = true);

	UFUNCTION(BlueprintCallable, Category = "Asset Data Subsystem|Events")
	void BindOnModModuleAdded(FKBFLOnModModuleAddedEvent Delegate, bool bEnsureLoaded = true);

	UFUNCTION(BlueprintCallable, Category = "Asset Data Subsystem|Events")
	void BindOnResourceDescriptorAdded(FKBFLOnResourceDescriptorAddedEvent Delegate, bool bEnsureLoaded = true);

	UFUNCTION(BlueprintCallable, Category = "Asset Data Subsystem|Events")
	void BindOnResearchTreeAdded(FKBFLOnResearchTreeAddedEvent Delegate, bool bEnsureLoaded = true);

	UFUNCTION(BlueprintCallable, Category = "Asset Data Subsystem|Events")
	void BindOnSessionSettingAdded(FKBFLOnSessionSettingAddedEvent Delegate, bool bEnsureLoaded = true);

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

	void IndexResolvedCategory(int32 Type);

	TArray<FAssetData> mScannedAssets;

	TSet<int32> mResolvedTypes;

	TSet<int32> mDeferredCategoryTypes;

	void TriggerAsyncLoadsForCategory(int32 Type);
	void OnCategoryAsyncLoaded(int32 Type);
	void AddAsyncLoadedClassToCategory(UClass* Class, int32 Type);
	void BroadcastCategoryEvent(UClass* Class, int32 Type);

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

	TArray<FString> mPreventStrings = {"/PassiveMode/"};
};

#pragma warning(push)
#pragma warning(disable : 4702)
template <class T>
void UKBFLAssetDataSubsystem::PrintArray(TSet<T> List)
{
	return;
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

		FString NativeParentClassPath;
		if (AssetData.GetTagValue(FBlueprintTags::NativeParentClassPath, NativeParentClassPath))
		{

			FString NativeClassName;
			FString NativeClassObjectPath;
			if (FPackageName::ParseExportTextPath(NativeParentClassPath, &NativeClassName, &NativeClassObjectPath))
			{

				if (UClass* NativeClass = FindObject<UClass>(nullptr, *NativeClassObjectPath))
				{
					if (!NativeClass->IsChildOf(TargetClass))
					{
						continue;
					}
				}
			}
		}

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

		if (AssetData.AssetClassPath != FTopLevelAssetPath(UBlueprint::StaticClass()))
		{
			continue;
		}

		FString GeneratedClassExportedPath;
		if (!AssetData.GetTagValue(FBlueprintTags::GeneratedClassPath, GeneratedClassExportedPath))
		{
			continue;
		}

		FString GeneratedClassPath;
		if (!FPackageName::ParseExportTextPath(GeneratedClassExportedPath, nullptr, &GeneratedClassPath))
		{
			continue;
		}

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
