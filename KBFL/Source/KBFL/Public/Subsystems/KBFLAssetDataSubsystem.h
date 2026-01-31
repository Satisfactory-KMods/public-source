#pragma once

#include "AssetRegistry/AssetRegistryModule.h"
#include "CoreMinimal.h"
#include "FGDriveablePawn.h"
#include "FGResearchTree.h"
#include "FGSchematic.h"
#include "FGVehicle.h"
#include "Interfaces/KBFLContentCDOHelperInterface.h"
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
		mAllFoundedCDOHelpers.Empty();
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
		case 7:
			mAllFoundedCDOHelpers.Add(Class);
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
	TSet<USMLSessionSetting*> mAllFoundAGS = {};

	UPROPERTY(BlueprintReadOnly)
	TSet<UClass*> mAllFoundResearchTrees = {};

	UPROPERTY(BlueprintReadOnly)
	TSet<UClass*> mAllFoundedSchematics = {};

	UPROPERTY(BlueprintReadOnly)
	TSet<UClass*> mAllFoundedRecipes = {};

	UPROPERTY(BlueprintReadOnly)
	TSet<UClass*> mAllFoundedItems = {};

	UPROPERTY(BlueprintReadOnly)
	TSet<UClass*> mAllFoundedBuildables = {};

	UPROPERTY(BlueprintReadOnly)
	TSet<UClass*> mAllFoundedDriveablePawns = {};

	UPROPERTY(BlueprintReadOnly)
	TSet<UClass*> mAllFoundedHolograms = {};

	UPROPERTY(BlueprintReadOnly)
	TSet<UClass*> mAllFoundedModModules = {};

	UPROPERTY(BlueprintReadOnly)
	TSet<UClass*> mAllFoundedCDOHelpers = {};

	UPROPERTY(BlueprintReadOnly)
	TSet<UClass*> mAllFoundedResourceDescriptors = {};
};


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

	void setMapClass(UClass* Class, int32 Type)
	{
		TArray<FString> DirectoryArray;
		Class->GetFullName().ParseIntoArray(DirectoryArray, TEXT("/"));
		FString ModName = DirectoryArray[1];
		// UE_LOG( LogTemp, Warning, TEXT("setMapClass: %s > %s > %d"), *ModName, *Class->GetFullName( ), Type );

		if (FKBFLAssetData* AssetData = mDirectoryMappings.Find(FName(ModName.ToLower())))
		{
			AssetData->WriteClass(Class, Type);
		}
		else
		{
			FKBFLAssetData KBFLAssetData = FKBFLAssetData();
			KBFLAssetData.WriteClass(Class, Type);
			mDirectoryMappings.Add(FName(ModName.ToLower()), KBFLAssetData);
		}
	}

	void setMapClass(UObject* Object, int32 Type)
	{
		TArray<FString> DirectoryArray;
		Object->GetFullName().ParseIntoArray(DirectoryArray, TEXT("/"));
		FString ModName = DirectoryArray[1];
		// UE_LOG(LogTemp, Warning, TEXT("setMapClass (OBJECT): %s > %s > %d"), *ModName, *Object->GetFullName( ),
		// Type);

		if (FKBFLAssetData* AssetData = mDirectoryMappings.Find(FName(ModName.ToLower())))
		{
			AssetData->WriteObject(Object, Type);
		}
		else
		{
			FKBFLAssetData KBFLAssetData = FKBFLAssetData();
			KBFLAssetData.WriteObject(Object, Type);
			mDirectoryMappings.Add(FName(ModName.ToLower()), KBFLAssetData);
		}
	}

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

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Asset Data Subsystem")
	void GetAllBuildableDesc(TArray<TSubclassOf<UFGBuildingDescriptor>>& Out);
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


	/** Get All CDOHelpers that hit the Child Classes  */
	UFUNCTION(BlueprintCallable, Category = "Asset Data Subsystem")
	void GetCDOHelpersOfChilds(TArray<UClass*> Childs, TArray<TSubclassOf<UKBFL_CDOHelperClass_Base>>& Out_Items,
							   bool UseNativeCheck = false);

	/** get data from a mod */
	UFUNCTION(BlueprintCallable, Category = "Asset Data Subsystem")
	FKBFLAssetData GetModRelatedData(UModModule* ModModule);

	/** Get All ResourceDescriptors that hit the Child Classes  */
	UFUNCTION(BlueprintCallable, Category = "Asset Data Subsystem")
	void GetResourceDescriptorsOfChilds(TArray<UClass*> Childs,
										TArray<TSubclassOf<UKBFLActorSpawnDescriptorBase>>& Out_Items,
										bool UseNativeCheck = false);

	inline static bool bWasInit = false;

	/** Return the Index where the ItemClass allowed on Slot in Inventory */
	template <class T>
	static bool GetAllClassesOfSubclass(TArray<FAssetData> AllAssets, TSet<TSubclassOf<T>>& OutClasses);


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

private:
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
	TSet<TSubclassOf<UKBFL_CDOHelperClass_Base>> mAllFoundedCDOHelpers;

	UPROPERTY()
	TSet<TSubclassOf<UFGResourceDescriptor>> mAllFoundedResourceDescriptors;

	UPROPERTY()
	TSet<UClass*> mAllFoundedObjects;

	UPROPERTY()
	TSet<TSubclassOf<UFGResearchTree>> mAllFoundResearchTrees;

	UPROPERTY()
	TSet<USMLSessionSetting*> mAllFoundAGS;

	UPROPERTY()
	TMap<FName, FKBFLAssetData> mDirectoryMappings;

public:
	TMap<UClass*, FAssetData> mAssetClassMap;

	// Small fix for PassiveMode
	TArray<FString> mPreventStrings = {"/PassiveMode/"};
};

template <class T>
void UKBFLAssetDataSubsystem::PrintArray(TSet<T> List)
{
	return; // Disbaled for debug reasons
	for (UClass* Class : List)
	{
		UE_LOG(AssetDataSubsystemLog, Log, TEXT("Class: %s"), *Class->GetClassPathName().ToString());
	}
}

template <class T>
void UKBFLAssetDataSubsystem::GetObjectsOfChilds_Internal(const TArray<UClass*> Childs,
														  TArray<TSubclassOf<T>>& Out_Items, bool UseNativeCheck)
{
	if (!bWasInit)
	{
		UE_LOG(LogTemp, Error, TEXT("Try to get classes without Init before the subsytem! CANCEL!"));
		return;
	}

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
bool UKBFLAssetDataSubsystem::GetAllClassesOfSubclass(TArray<FAssetData> AllAssets, TSet<TSubclassOf<T>>& OutClasses)
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
				// Load the native class and verify inheritance
				if (UClass* NativeClass = LoadObject<UClass>(nullptr, *NativeClassObjectPath))
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
			if (SoftClass.IsPending() || SoftClass.IsValid())
			{
				UClass* LoadedClass = SoftClass.LoadSynchronous();
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

		// Load the generated class
		UClass* ClassObject = LoadObject<UClass>(nullptr, *GeneratedClassPath);
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
