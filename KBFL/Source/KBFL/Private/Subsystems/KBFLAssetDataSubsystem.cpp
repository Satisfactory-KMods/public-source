#include "Subsystems/KBFLAssetDataSubsystem.h"

#include "FGGameState.h"
#include "KBFLLogging.h"
#include "SMLWorldModule.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Buildables/FGBuildableWire.h"
#include "Hologram/FGHologram.h"

#include "ModLoading/PluginModuleLoader.h"

#include "Module/MenuWorldModule.h"

#include "Resources/FGAnyUndefinedDescriptor.h"
#include "Resources/FGBuildDescriptor.h"
#include "Resources/FGBuildingDescriptor.h"
#include "Resources/FGNoneDescriptor.h"
#include "Resources/FGOverflowDescriptor.h"
#include "Resources/FGVehicleDescriptor.h"
#include "Resources/FGWildCardDescriptor.h"

void UKBFLAssetDataSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(FName("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	// Must wait until all assets are discovered before populating list of assets.
	if (AssetRegistry.IsLoadingAssets())
	{
		AssetRegistry.OnFilesLoaded().AddUObject(this, &UKBFLAssetDataSubsystem::ScanOnInitialize);
	}
	else
	{
		ScanOnInitialize();
	}
}

void UKBFLAssetDataSubsystem::Deinitialize()
{
	mAllFoundedSchematics.Empty();
	mAllFoundedRecipes.Empty();
	mAllFoundedItems.Empty();
	mAllFoundedBuildables.Empty();
	mAllFoundedDriveablePawns.Empty();
	mAllFoundedHolograms.Empty();
	mAllFoundedModModules.Empty();
	mAllFoundedCDOHelpers.Empty();
	mAllFoundedResourceDescriptors.Empty();
	mAllFoundedObjects.Empty();

	for (TTuple<FName, FKBFLAssetData> directoryMapping : mDirectoryMappings)
	{
		directoryMapping.Value.cleanup();
	}

	mDirectoryMappings.Empty();

	bWasInit = false;

	Super::Deinitialize();
}

void UKBFLAssetDataSubsystem::ScanOnInitialize()
{
	DoScan();
	bWasInit = false;
}

void UKBFLAssetDataSubsystem::DoScan(bool Force)
{
	if (!bWasInit || Force)
	{
		UE_LOG(AssetDataSubsystemLog, Log, TEXT("FORCE! Initialize Subsystem in WorldName: %s"),
			   *GetWorld()->GetMapName());
		InitAssetFinder();
		PrintFound();
	}
}

UKBFLAssetDataSubsystem* UKBFLAssetDataSubsystem::Get(const UObject* WorldContext)
{
	if (IsValid(WorldContext))
	{
		return WorldContext->GetWorld()->GetGameInstance()->GetSubsystem<UKBFLAssetDataSubsystem>();
	}
	return nullptr;
}

UKBFLAssetDataSubsystem* UKBFLAssetDataSubsystem::GetChecked(const UObject* WorldContext)
{
	UKBFLAssetDataSubsystem* Subsystem = Get(WorldContext);
	fgcheck(Subsystem);
	return Subsystem;
}

void UKBFLAssetDataSubsystem::PrintFound()
{
	UE_LOG(AssetDataSubsystemLog, Log, TEXT("--------------------------------------------"));
	UE_LOG(AssetDataSubsystemLog, Log, TEXT("Successful Initialize UKBFLAssetDataSubsystem"));
	UE_LOG(AssetDataSubsystemLog, Log, TEXT("mAllFoundedSchematics: %d"), mAllFoundedSchematics.Num());
	PrintArray(mAllFoundedSchematics);
	UE_LOG(AssetDataSubsystemLog, Log, TEXT("mAllFoundedBuildables: %d"), mAllFoundedBuildables.Num());
	PrintArray(mAllFoundedBuildables);
	UE_LOG(AssetDataSubsystemLog, Log, TEXT("mAllFoundedItems: %d"), mAllFoundedItems.Num());
	PrintArray(mAllFoundedItems);
	UE_LOG(AssetDataSubsystemLog, Log, TEXT("mAllFoundedRecipes: %d"), mAllFoundedRecipes.Num());
	PrintArray(mAllFoundedRecipes);
	UE_LOG(AssetDataSubsystemLog, Log, TEXT("mAllFoundedDriveablePawns: %d"), mAllFoundedDriveablePawns.Num());
	PrintArray(mAllFoundedDriveablePawns);
	UE_LOG(AssetDataSubsystemLog, Log, TEXT("mAllFoundedHolograms: %d"), mAllFoundedHolograms.Num());
	PrintArray(mAllFoundedHolograms);
	UE_LOG(AssetDataSubsystemLog, Log, TEXT("mAllFoundedModModules: %d"), mAllFoundedModModules.Num());
	PrintArray(mAllFoundedModModules);
	UE_LOG(AssetDataSubsystemLog, Log, TEXT("mAllFoundedCDOHelpers: %d"), mAllFoundedCDOHelpers.Num());
	PrintArray(mAllFoundedCDOHelpers);
	UE_LOG(AssetDataSubsystemLog, Log, TEXT("mAllFoundedResourceDescriptors: %d"),
		   mAllFoundedResourceDescriptors.Num());
	PrintArray(mAllFoundedResourceDescriptors);
	UE_LOG(AssetDataSubsystemLog, Log, TEXT("mAllFoundedObjects: %d"), mAllFoundedObjects.Num());
	PrintArray(mAllFoundedObjects);
	UE_LOG(AssetDataSubsystemLog, Log, TEXT("--------------------------------------------"));
}

void UKBFLAssetDataSubsystem::InitAssetFinder()
{
	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	bWasInit = true;

	// Use game world module by default
	TSubclassOf<UWorldModule> ModuleTypeClass = UGameWorldModule::StaticClass();

	// Use MenuWorldModule if we are in the main menu
	if (FPluginModuleLoader::IsMainMenuWorld(GetWorld()))
	{
		ModuleTypeClass = UMenuWorldModule::StaticClass();
	}

	// Discover modules of the relevant types
	const TArray<FDiscoveredModule> DiscoveredModules = FPluginModuleLoader::FindRootModulesOfType(ModuleTypeClass);

	TArray<FName> SearchPaths;
	SearchPaths.Add(FName("/Game"));

	for (const FDiscoveredModule& Module : DiscoveredModules)
	{
		UE_LOG(AssetDataSubsystemLog, Log, TEXT("Load Mod: %s"), *Module.OwnerPluginName);
		SearchPaths.Add(FName("/" + Module.OwnerPluginName));
	}

	// OPTIMIZATION: Create filter for only Blueprint assets to reduce initial scan
	// This significantly reduces the number of assets we need to process
	// Instead of loading ALL assets (textures, meshes, sounds, etc.), we only look at Blueprints
	FARFilter Filter;
	Filter.PackagePaths = SearchPaths;
	Filter.bRecursivePaths = true;
	Filter.bRecursiveClasses = true;

	// Only load Blueprint assets
	Filter.ClassPaths.Add(FTopLevelAssetPath(UBlueprint::StaticClass()));
	Filter.ClassPaths.Add(FTopLevelAssetPath(UBlueprintGeneratedClass::StaticClass()));

	// Also include DataAsset for session settings
	Filter.ClassPaths.Add(FTopLevelAssetPath(UDataAsset::StaticClass()));

	TArray<FAssetData> AssetData;
	if (AssetRegistry.GetAssets(Filter, AssetData))
	{
		UE_LOG(AssetDataSubsystemLog, Log, TEXT("Found %d assets to scan (filtered from all assets)"), AssetData.Num());

		// Scan specific types with filtered asset data
		// GetAllClassesOfSubclass is now optimized to use NativeParentClass filtering
		GetAllClassesOfSubclass(AssetData, mAllFoundedSchematics);
		GetAllClassesOfSubclass(AssetData, mAllFoundedRecipes);
		GetAllClassesOfSubclass(AssetData, mAllFoundedItems);
		GetAllClassesOfSubclass(AssetData, mAllFoundedBuildables);
		GetAllClassesOfSubclass(AssetData, mAllFoundedDriveablePawns);
		GetAllClassesOfSubclass(AssetData, mAllFoundedHolograms);
		GetAllClassesOfSubclass(AssetData, mAllFoundedModModules);
		GetAllClassesOfSubclass(AssetData, mAllFoundedCDOHelpers);
		GetAllClassesOfSubclass(AssetData, mAllFoundedResourceDescriptors);
		GetAllClassesOfSubclass(AssetData, mAllFoundResearchTrees);

		// Find DataAssets separately (different asset type)
		FindAllDataAssetsOfClass(mAllFoundAGS);

		UE_LOG(AssetDataSubsystemLog, Log,
			   TEXT("Scan complete - Found: Schematics=%d, Recipes=%d, Items=%d, Buildables=%d, ResearchTrees=%d"),
			   mAllFoundedSchematics.Num(), mAllFoundedRecipes.Num(), mAllFoundedItems.Num(),
			   mAllFoundedBuildables.Num(), mAllFoundResearchTrees.Num());

		// Build the mAllFoundedObjects set
		for (UClass* data : mAllFoundedSchematics)
		{
			mAllFoundedObjects.Add(data);
			setMapClass(data, 0);
		}
		for (UClass* data : mAllFoundedRecipes)
		{
			mAllFoundedObjects.Add(data);
			setMapClass(data, 1);
		}
		for (UClass* data : mAllFoundedItems)
		{
			mAllFoundedObjects.Add(data);
			setMapClass(data, 2);
		}
		for (UClass* data : mAllFoundedBuildables)
		{
			mAllFoundedObjects.Add(data);
			setMapClass(data, 3);
		}
		for (UClass* data : mAllFoundedDriveablePawns)
		{
			mAllFoundedObjects.Add(data);
			setMapClass(data, 4);
		}
		for (UClass* data : mAllFoundedHolograms)
		{
			mAllFoundedObjects.Add(data);
			setMapClass(data, 5);
		}
		for (UClass* data : mAllFoundedModModules)
		{
			mAllFoundedObjects.Add(data);
			setMapClass(data, 6);
		}
		for (UClass* data : mAllFoundedCDOHelpers)
		{
			mAllFoundedObjects.Add(data);
			setMapClass(data, 7);
		}
		for (UClass* data : mAllFoundedResourceDescriptors)
		{
			mAllFoundedObjects.Add(data);
			setMapClass(data, 8);
		}
		for (UClass* data : mAllFoundResearchTrees)
		{
			mAllFoundedObjects.Add(data);
			setMapClass(data, 10);
		}
		for (UObject* data : mAllFoundAGS)
		{
			setMapClass(data, 11);
		}
	}
	else
	{
		UE_LOG(AssetDataSubsystemLog, Error, TEXT("FAIL TO FIND ASSET PATH"));
	}
}

bool UKBFLAssetDataSubsystem::FilterAsset(const FAssetData& AssetData) { return true; }

bool UKBFLAssetDataSubsystem::Local_FilterAsset(const FAssetData& AssetData) const
{
	for (const auto TagValue : AssetData.TagsAndValues)
	{
		if (TagValue.Value.AsString().Contains("LevelScriptActor"))
		{
			// AssetData.PrintAssetData( );
			// UE_LOG( AssetDataSubsystemLog, Log, TEXT("SKIP BECAUSE Tag!!! > %s"), *TagValue.Value.AsString( ) );
			return false;
		}
	}

	FString ExportedPath = AssetData.GetObjectPathString();
	for (auto String : mPreventStrings)
	{
		if (ExportedPath.Contains(String))
		{
			// AssetData.PrintAssetData( );
			// UE_LOG( AssetDataSubsystemLog, Log, TEXT("SKIP BECAUSE ExportedPath!!! > %s"), *String );
			return false;
		}
	}
	return true;
}

void UKBFLAssetDataSubsystem::GetItemsOfForms(TArray<EResourceForm> Forms,
											  TArray<TSubclassOf<UFGItemDescriptor>>& Out_Items)
{
	if (!bWasInit)
	{
		UE_LOG(AssetDataSubsystemLog, Error,
			   TEXT("Try to get classes without Init before the subsytem! Do ForceScan!"));
		DoScan();
	}

	for (UClass* Class : mAllFoundedItems)
	{
		if (TSubclassOf<UFGItemDescriptor> Item = Class)
		{
			if (Forms.Contains(UFGItemDescriptor::GetForm(Item)))
			{
				Out_Items.Add(Item);
			}
		}
	}
}

void UKBFLAssetDataSubsystem::GetItemsOfChilds(TArray<UClass*> Childs,
											   TArray<TSubclassOf<UFGItemDescriptor>>& Out_Items, bool UseNativeCheck)
{
	if (!bWasInit)
	{
		UE_LOG(AssetDataSubsystemLog, Error,
			   TEXT("Try to get classes without Init before the subsytem! Do ForceScan!"));
		DoScan();
	}

	for (UClass* Class : mAllFoundedItems)
	{
		if (TSubclassOf<UFGItemDescriptor> Item = Class)
		{
			if (CheckChild(Item, Childs, UseNativeCheck))
			{
				Out_Items.Add(Item);
			}
		}
	}
}

void UKBFLAssetDataSubsystem::GetItemsFiltered(TArray<TSubclassOf<UFGItemDescriptor>>& Out_Items)
{
	if (!bWasInit)
	{
		UE_LOG(AssetDataSubsystemLog, Error,
			   TEXT("Try to get classes without Init before the subsytem! Do ForceScan!"));
		DoScan();
	}

	for (UClass* Class : mAllFoundedItems)
	{
		if (TSubclassOf<UFGItemDescriptor> Item = Class)
		{
			if (UFGItemDescriptor::GetForm(Item) != EResourceForm::RF_INVALID &&
				!Item->IsChildOf(UFGBuildDescriptor::StaticClass()) &&
				!Item->IsChildOf(UFGFactoryCustomizationDescriptor::StaticClass()) &&
				!UFGItemDescriptor::GetItemName(Item).IsEmpty() && !Item->IsChildOf(UFGNoneDescriptor::StaticClass()) &&
				!Item->IsChildOf(UFGAnyUndefinedDescriptor::StaticClass()) &&
				!Item->IsChildOf(UFGOverflowDescriptor::StaticClass()) &&
				!Item->IsChildOf(UFGWildCardDescriptor::StaticClass()))
			{
				Out_Items.Add(Item);
			}
		}
	}
}

void UKBFLAssetDataSubsystem::GetItemsFilteredWithForm(TArray<EResourceForm> Forms,
													   TArray<TSubclassOf<UFGItemDescriptor>>& Out_Items)
{
	if (!bWasInit)
	{
		UE_LOG(AssetDataSubsystemLog, Error,
			   TEXT("Try to get classes without Init before the subsytem! Do ForceScan!"));
		DoScan();
	}

	for (UClass* Class : mAllFoundedItems)
	{
		if (TSubclassOf<UFGItemDescriptor> Item = Class)
		{
			if (Forms.Contains(UFGItemDescriptor::GetForm(Item)) &&
				!Item->IsChildOf(UFGBuildDescriptor::StaticClass()) &&
				!Item->IsChildOf(UFGFactoryCustomizationDescriptor::StaticClass()) &&
				!UFGItemDescriptor::GetItemName(Item).IsEmpty() && !Item->IsChildOf(UFGNoneDescriptor::StaticClass()) &&
				!Item->IsChildOf(UFGAnyUndefinedDescriptor::StaticClass()) &&
				!Item->IsChildOf(UFGOverflowDescriptor::StaticClass()) &&
				!Item->IsChildOf(UFGWildCardDescriptor::StaticClass()))
			{
				Out_Items.Add(Item);
			}
		}
	}
}

TArray<TSubclassOf<UFGItemDescriptor>> UKBFLAssetDataSubsystem::GetAllItems()
{
	return TArray<TSubclassOf<UFGItemDescriptor>>(mAllFoundedItems.Array());
}

void UKBFLAssetDataSubsystem::GetSchematicsOfTypes(TArray<ESchematicType> Types,
												   TArray<TSubclassOf<UFGSchematic>>& Out_Schematics)
{
	if (!bWasInit)
	{
		UE_LOG(AssetDataSubsystemLog, Error,
			   TEXT("Try to get classes without Init before the subsytem! Do ForceScan!"));
		DoScan();
	}

	for (UClass* Class : mAllFoundedSchematics)
	{
		if (TSubclassOf<UFGSchematic> Item = Class)
		{
			if (Types.Contains(UFGSchematic::GetType(Item)))
			{
				Out_Schematics.Add(Item);
			}
		}
	}
}

void UKBFLAssetDataSubsystem::GetSchematicsOfChilds(TArray<UClass*> Childs,
													TArray<TSubclassOf<UFGSchematic>>& Out_Items, bool UseNativeCheck)
{
	if (!bWasInit)
	{
		UE_LOG(AssetDataSubsystemLog, Error,
			   TEXT("Try to get classes without Init before the subsytem! Do ForceScan!"));
		DoScan();
	}

	for (UClass* FoundedClass : mAllFoundedSchematics)
	{
		if (TSubclassOf<UFGSchematic> Class = FoundedClass)
		{
			if (CheckChild(Class, Childs, UseNativeCheck))
			{
				Out_Items.Add(Class);
			}
		}
	}
}

TArray<TSubclassOf<UFGSchematic>> UKBFLAssetDataSubsystem::GetAllSchematics() { return mAllFoundedSchematics.Array(); }

void UKBFLAssetDataSubsystem::GetRecipesOfChilds(TArray<UClass*> Childs, TArray<TSubclassOf<UFGRecipe>>& Out_Items,
												 bool UseNativeCheck)
{
	if (!bWasInit)
	{
		UE_LOG(AssetDataSubsystemLog, Error,
			   TEXT("Try to get classes without Init before the subsytem! Do ForceScan!"));
		DoScan();
	}

	for (UClass* FoundedClass : mAllFoundedRecipes)
	{
		if (TSubclassOf<UFGRecipe> Class = FoundedClass)
		{
			if (CheckChild(Class, Childs, UseNativeCheck))
			{
				Out_Items.Add(Class);
			}
		}
	}
}

void UKBFLAssetDataSubsystem::GetRecipesOfProducer(TArray<TSubclassOf<UObject>> Producers,
												   TArray<TSubclassOf<UFGRecipe>>& Out_Items)
{
	if (!bWasInit)
	{
		UE_LOG(AssetDataSubsystemLog, Error,
			   TEXT("Try to get classes without Init before the subsytem! Do ForceScan!"));
		DoScan();
	}

	for (UClass* FoundedClass : mAllFoundedRecipes)
	{
		if (TSubclassOf<UFGRecipe> Class = FoundedClass)
		{
			if (CheckHasRecipeProducer(Class, Producers))
			{
				Out_Items.Add(Class);
			}
		}
	}
}

TArray<TSubclassOf<UFGRecipe>> UKBFLAssetDataSubsystem::GetAllRecipes()
{
	return TArray<TSubclassOf<UFGRecipe>>(mAllFoundedRecipes.Array());
}
TArray<TSubclassOf<UFGResearchTree>> UKBFLAssetDataSubsystem::GetAllResearchTrees()
{
	return TArray<TSubclassOf<UFGResearchTree>>(mAllFoundResearchTrees.Array());
}

void UKBFLAssetDataSubsystem::GetBuildableOfChilds(TArray<UClass*> Childs, TArray<TSubclassOf<AFGBuildable>>& Out_Items,
												   bool UseNativeCheck)
{
	if (!bWasInit)
	{
		UE_LOG(AssetDataSubsystemLog, Error,
			   TEXT("Try to get classes without Init before the subsytem! Do ForceScan!"));
		DoScan();
	}

	for (UClass* FoundedClass : mAllFoundedBuildables)
	{
		if (TSubclassOf<AFGBuildable> Class = FoundedClass)
		{
			if (CheckChild(Class, Childs, UseNativeCheck))
			{
				Out_Items.Add(Class);
			}
		}
	}
}

TArray<TSubclassOf<AFGBuildable>> UKBFLAssetDataSubsystem::GetAllBuildable()
{
	return TArray<TSubclassOf<AFGBuildable>>(mAllFoundedBuildables.Array());
}

bool UKBFLAssetDataSubsystem::CheckChild(UClass* TestClass, TArray<UClass*> ClassesToTest, bool UseNativeCheck)
{
	if (TestClass->IsNative() && UseNativeCheck)
	{
		for (UClass* ClassToTest : ClassesToTest)
		{
			if (GetParentNativeClass(ClassToTest) == TestClass)
			{
				return true;
			}
		}
		return false;
	}

	for (UClass* ClassToTest : ClassesToTest)
	{
		if (TestClass->IsChildOf(ClassToTest))
		{
			return true;
		}
	}
	return false;
}

bool UKBFLAssetDataSubsystem::CheckHasRecipeProducer(TSubclassOf<UFGRecipe> TestClass,
													 TArray<TSubclassOf<UObject>> Producers)
{
	if (TestClass)
	{
		const TArray<TSubclassOf<UObject>> RecipeProducers = UFGRecipe::GetProducedIn(TestClass);
		for (TSubclassOf<UObject> Producer : Producers)
		{
			if (Producer)
			{
				if (RecipeProducers.Contains(Producer))
				{
					return true;
				}
			}
		}
	}
	return false;
}

TSubclassOf<UFGBuildingDescriptor>
UKBFLAssetDataSubsystem::GetDescForBuildable(TSubclassOf<AFGBuildable> BuildableClass)
{
	for (UClass* ItemClass : mAllFoundedItems)
	{
		if (ItemClass && ItemClass->IsChildOf(UFGBuildingDescriptor::StaticClass()))
		{
			TSubclassOf<AFGBuildable> TestBuildableClass = UFGBuildingDescriptor::GetBuildableClass(ItemClass);
			if (TestBuildableClass == BuildableClass)
			{
				return ItemClass;
			}
		}
	}
	return nullptr;
}

TSubclassOf<UFGVehicleDescriptor> UKBFLAssetDataSubsystem::GetDescForVehicle(TSubclassOf<AFGVehicle> Class)
{
	for (UClass* ItemClass : mAllFoundedItems)
	{
		if (ItemClass && ItemClass->IsChildOf(UFGVehicleDescriptor::StaticClass()))
		{
			TSubclassOf<AFGVehicle> TestBuildableClass = UFGVehicleDescriptor::GetVehicleClass(ItemClass);
			if (TestBuildableClass == Class)
			{
				return ItemClass;
			}
		}
	}
	return nullptr;
}

void UKBFLAssetDataSubsystem::GetAllBuildableDesc(TArray<TSubclassOf<UFGBuildingDescriptor>>& Out) {}

void UKBFLAssetDataSubsystem::GetObjectsOfChilds(TArray<UClass*> Childs, TArray<TSubclassOf<UObject>>& Out_Items,
												 bool UseNativeCheck)
{
	if (!bWasInit)
	{
		UE_LOG(AssetDataSubsystemLog, Error,
			   TEXT("Try to get classes without Init before the subsytem! Do ForceScan!"));
		DoScan();
	}

	for (UClass* FoundedClass : mAllFoundedObjects)
	{
		if (TSubclassOf<UObject> Class = FoundedClass)
		{
			if (CheckChild(Class, Childs, UseNativeCheck))
			{
				Out_Items.Add(Class);
			}
		}
	}
}

void UKBFLAssetDataSubsystem::GetDriveablePawnsOfChilds(TArray<UClass*> Childs,
														TArray<TSubclassOf<AFGDriveablePawn>>& Out_Items,
														bool UseNativeCheck)
{
	if (!bWasInit)
	{
		UE_LOG(AssetDataSubsystemLog, Error,
			   TEXT("Try to get classes without Init before the subsytem! Do ForceScan!"));
		DoScan();
	}

	for (UClass* FoundedClass : mAllFoundedDriveablePawns)
	{
		if (TSubclassOf<AFGDriveablePawn> Class = FoundedClass)
		{
			if (CheckChild(Class, Childs, UseNativeCheck))
			{
				Out_Items.Add(Class);
			}
		}
	}
}

void UKBFLAssetDataSubsystem::GetHologramsOfChilds(TArray<UClass*> Childs, TArray<TSubclassOf<AFGHologram>>& Out_Items,
												   bool UseNativeCheck)
{
	if (!bWasInit)
	{
		UE_LOG(AssetDataSubsystemLog, Error,
			   TEXT("Try to get classes without Init before the subsytem! Do ForceScan!"));
		DoScan();
	}

	for (UClass* FoundedClass : mAllFoundedHolograms)
	{
		if (TSubclassOf<AFGHologram> Class = FoundedClass)
		{
			if (CheckChild(Class, Childs, UseNativeCheck))
			{
				Out_Items.Add(Class);
			}
		}
	}
}

void UKBFLAssetDataSubsystem::GetModModulesOfChilds(TArray<UClass*> Childs, TArray<TSubclassOf<UModModule>>& Out_Items,
													bool UseNativeCheck)
{
	if (!bWasInit)
	{
		UE_LOG(AssetDataSubsystemLog, Error,
			   TEXT("Try to get classes without Init before the subsytem! Do ForceScan!"));
		DoScan();
	}

	for (UClass* FoundedClass : mAllFoundedModModules)
	{
		if (TSubclassOf<UModModule> Class = FoundedClass)
		{
			if (CheckChild(Class, Childs, UseNativeCheck))
			{
				Out_Items.Add(Class);
			}
		}
	}
}

void UKBFLAssetDataSubsystem::GetCDOHelpersOfChilds(TArray<UClass*> Childs,
													TArray<TSubclassOf<UKBFL_CDOHelperClass_Base>>& Out_Items,
													bool UseNativeCheck)
{
	if (!bWasInit)
	{
		UE_LOG(AssetDataSubsystemLog, Error,
			   TEXT("Try to get classes without Init before the subsytem! Do ForceScan!"));
		DoScan();
	}

	for (UClass* FoundedClass : mAllFoundedCDOHelpers)
	{
		if (TSubclassOf<UKBFL_CDOHelperClass_Base> Class = FoundedClass)
		{
			if (CheckChild(Class, Childs, UseNativeCheck))
			{
				Out_Items.Add(Class);
			}
		}
	}
}

FKBFLAssetData UKBFLAssetDataSubsystem::GetModRelatedData(UModModule* ModModule)
{
	if (!bWasInit)
	{
		UE_LOG(AssetDataSubsystemLog, Error,
			   TEXT("Try to get classes without Init before the subsytem! Do ForceScan!"));
		DoScan();
	}

	UE_LOG(AssetDataSubsystemLog, Warning, TEXT("GetModRelatedData: %s"),
		   *ModModule->GetOwnerModReference().ToString().ToLower());
	if (FKBFLAssetData* AssetData =
			mDirectoryMappings.Find(FName(ModModule->GetOwnerModReference().ToString().ToLower())))
	{
		return *AssetData;
	}
	return FKBFLAssetData();
}

void UKBFLAssetDataSubsystem::GetResourceDescriptorsOfChilds(
	TArray<UClass*> Childs, TArray<TSubclassOf<UKBFLActorSpawnDescriptorBase>>& Out_Items, bool UseNativeCheck)
{
	if (!bWasInit)
	{
		UE_LOG(AssetDataSubsystemLog, Error,
			   TEXT("Try to get classes without Init before the subsytem! Do ForceScan!"));
		DoScan();
	}

	for (UClass* FoundedClass : mAllFoundedResourceDescriptors)
	{
		if (TSubclassOf<UKBFLActorSpawnDescriptorBase> Class = FoundedClass)
		{
			if (CheckChild(Class, Childs, UseNativeCheck))
			{
				Out_Items.Add(Class);
			}
		}
	}
}
