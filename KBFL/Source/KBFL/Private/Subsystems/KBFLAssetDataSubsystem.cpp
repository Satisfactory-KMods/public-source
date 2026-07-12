#include "Subsystems/KBFLAssetDataSubsystem.h"

#include "Engine/AssetManager.h"

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
	mAllFoundedResourceDescriptors.Empty();
	mAllFoundResearchTrees.Empty();
	mAllFoundAGS.Empty();
	mAllFoundedObjects.Empty();

	for (TTuple<FName, FKBFLAssetData> directoryMapping : mDirectoryMappings)
	{
		directoryMapping.Value.cleanup();
	}

	mDirectoryMappings.Empty();

	mScannedAssets.Empty();
	mResolvedTypes.Empty();
	mPendingAsyncLoads.Empty();
	bWasInit = false;

	Super::Deinitialize();
}

void UKBFLAssetDataSubsystem::ScanOnInitialize()
{
	// Init only scans registry metadata - no class is loaded here. Categories resolve lazily on first query.
	EnsureRegistryScanned();
}

void UKBFLAssetDataSubsystem::DoScan(bool Force)
{
	if (Force)
	{
		// Drop every cache and force a fresh registry scan. Categories will re-resolve lazily on next query.
		mResolvedTypes.Empty();
		mAllFoundedSchematics.Empty();
		mAllFoundedRecipes.Empty();
		mAllFoundedItems.Empty();
		mAllFoundedBuildables.Empty();
		mAllFoundedDriveablePawns.Empty();
		mAllFoundedHolograms.Empty();
		mAllFoundedModModules.Empty();
		mAllFoundedResourceDescriptors.Empty();
		mAllFoundResearchTrees.Empty();
		mAllFoundAGS.Empty();
		mAllFoundedObjects.Empty();

		for (TTuple<FName, FKBFLAssetData> DirectoryMapping : mDirectoryMappings)
		{
			DirectoryMapping.Value.cleanup();
		}
		mDirectoryMappings.Empty();

		mScannedAssets.Empty();
		mPendingAsyncLoads.Empty();
		bWasInit = false;
	}

	EnsureRegistryScanned();
}

void UKBFLAssetDataSubsystem::EnsureRegistryScanned()
{
	if (bWasInit)
	{
		return;
	}

	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(FName("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	// Safety guard: do not scan while AssetRegistry is still loading assets,
	// as this can cause a crash in FPluginModuleLoader::FindRootModulesOfType
	if (AssetRegistry.IsLoadingAssets())
	{
		UE_LOG(AssetDataSubsystemLog, Warning,
			   TEXT("EnsureRegistryScanned: AssetRegistry is still loading - deferring until OnFilesLoaded"));
		AssetRegistry.OnFilesLoaded().AddUObject(this, &UKBFLAssetDataSubsystem::ScanOnInitialize);
		return;
	}

	InitAssetFinder();
}

void UKBFLAssetDataSubsystem::InitAssetFinder()
{
	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

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

	// Gather ONLY Blueprint + DataAsset registry metadata. This does NOT load any class object -
	// FAssetData is registry metadata. Class objects are loaded lazily, per category, in
	// EnsureCategoryResolved when a getter for that category is first called.
	FARFilter Filter;
	Filter.PackagePaths = SearchPaths;
	Filter.bRecursivePaths = true;
	Filter.bRecursiveClasses = true;
	Filter.ClassPaths.Add(FTopLevelAssetPath(UBlueprint::StaticClass()));
	Filter.ClassPaths.Add(FTopLevelAssetPath(UBlueprintGeneratedClass::StaticClass()));
	Filter.ClassPaths.Add(FTopLevelAssetPath(UDataAsset::StaticClass()));

	mScannedAssets.Reset();
	if (AssetRegistry.GetAssets(Filter, mScannedAssets))
	{
		bWasInit = true;
		UE_LOG(AssetDataSubsystemLog, Log,
			   TEXT("Registry scan complete: %d candidate assets found (no classes loaded yet)"), mScannedAssets.Num());
	}
	else
	{
		UE_LOG(AssetDataSubsystemLog, Error, TEXT("FAIL TO FIND ASSET PATH"));
	}
}

void UKBFLAssetDataSubsystem::EnsureCategoryResolved(int32 Type)
{
	if (mResolvedTypes.Contains(Type))
	{
		return;
	}

	EnsureRegistryScanned();
	if (!bWasInit)
	{
		// Registry still streaming — register a one-shot callback to retry after OnFilesLoaded.
		if (!mDeferredCategoryTypes.Contains(Type))
		{
			mDeferredCategoryTypes.Add(Type);
			FAssetRegistryModule& AssetRegistryModule =
				FModuleManager::LoadModuleChecked<FAssetRegistryModule>(FName("AssetRegistry"));
			AssetRegistryModule.Get().OnFilesLoaded().AddWeakLambda(this,
																	[this, Type]()
																	{
																		mDeferredCategoryTypes.Remove(Type);
																		EnsureCategoryResolved(Type);
																	});
		}
		return;
	}

	// GetDerivedClasses finds all currently in-memory subclasses with no disk I/O.
	// Using LoadObject here corrupts CSS's replication graph init (causes MP crash).
	auto AddDerivedToSet = [](UClass* Parent, auto& OutSet)
	{
		TArray<UClass*> Derived;
		GetDerivedClasses(Parent, Derived, true);
		for (UClass* C : Derived)
		{
			using SC = typename TDecay<decltype(OutSet)>::Type::ElementType;
			if (SC Sub = C)
			{
				OutSet.Add(Sub);
			}
		}
	};

	switch (Type)
	{
	case 0:
		AddDerivedToSet(UFGSchematic::StaticClass(), mAllFoundedSchematics);
		break;
	case 1:
		AddDerivedToSet(UFGRecipe::StaticClass(), mAllFoundedRecipes);
		break;
	case 2:
		AddDerivedToSet(UFGItemDescriptor::StaticClass(), mAllFoundedItems);
		break;
	case 3:
		AddDerivedToSet(AFGBuildable::StaticClass(), mAllFoundedBuildables);
		break;
	case 4:
		AddDerivedToSet(AFGDriveablePawn::StaticClass(), mAllFoundedDriveablePawns);
		break;
	case 5:
		AddDerivedToSet(AFGHologram::StaticClass(), mAllFoundedHolograms);
		break;
	case 6:
		AddDerivedToSet(UModModule::StaticClass(), mAllFoundedModModules);
		break;
	case 8:
		AddDerivedToSet(UFGResourceDescriptor::StaticClass(), mAllFoundedResourceDescriptors);
		break;
	case 10:
		AddDerivedToSet(UFGResearchTree::StaticClass(), mAllFoundResearchTrees);
		break;
	case 11:
		FindAllDataAssetsOfClass(mAllFoundAGS);
		break;
	default:
		return;
	}

	mResolvedTypes.Add(Type);
	IndexResolvedCategory(Type);
	TriggerAsyncLoadsForCategory(Type);
}

void UKBFLAssetDataSubsystem::EnsureAllResolved()
{
	static const int32 AllTypes[] = {0, 1, 2, 3, 4, 5, 6, 8, 10, 11};
	for (int32 Type : AllTypes)
	{
		EnsureCategoryResolved(Type);
	}
}

void UKBFLAssetDataSubsystem::setMapClass(UClass* Class, int32 Type)
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

void UKBFLAssetDataSubsystem::setMapClass(UObject* Object, int32 Type)
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

void UKBFLAssetDataSubsystem::IndexResolvedCategory(int32 Type)
{
	switch (Type)
	{
	case 0:
		for (UClass* Data : mAllFoundedSchematics)
		{
			mAllFoundedObjects.Add(Data);
			setMapClass(Data, 0);
		}
		break;
	case 1:
		for (UClass* Data : mAllFoundedRecipes)
		{
			mAllFoundedObjects.Add(Data);
			setMapClass(Data, 1);
		}
		break;
	case 2:
		for (UClass* Data : mAllFoundedItems)
		{
			mAllFoundedObjects.Add(Data);
			setMapClass(Data, 2);
		}
		break;
	case 3:
		for (UClass* Data : mAllFoundedBuildables)
		{
			mAllFoundedObjects.Add(Data);
			setMapClass(Data, 3);
		}
		break;
	case 4:
		for (UClass* Data : mAllFoundedDriveablePawns)
		{
			mAllFoundedObjects.Add(Data);
			setMapClass(Data, 4);
		}
		break;
	case 5:
		for (UClass* Data : mAllFoundedHolograms)
		{
			mAllFoundedObjects.Add(Data);
			setMapClass(Data, 5);
		}
		break;
	case 6:
		for (UClass* Data : mAllFoundedModModules)
		{
			mAllFoundedObjects.Add(Data);
			setMapClass(Data, 6);
		}
		break;
	case 8:
		for (UClass* Data : mAllFoundedResourceDescriptors)
		{
			mAllFoundedObjects.Add(Data);
			setMapClass(Data, 8);
		}
		break;
	case 10:
		for (UClass* Data : mAllFoundResearchTrees)
		{
			mAllFoundedObjects.Add(Data);
			setMapClass(Data, 10);
		}
		break;
	case 11:
		for (UObject* Data : mAllFoundAGS)
		{
			setMapClass(Data, 11);
		}
		break;
	default:
		break;
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
	UE_LOG(AssetDataSubsystemLog, Log, TEXT("mAllFoundedResourceDescriptors: %d"),
		   mAllFoundedResourceDescriptors.Num());
	PrintArray(mAllFoundedResourceDescriptors);
	UE_LOG(AssetDataSubsystemLog, Log, TEXT("mAllFoundedObjects: %d"), mAllFoundedObjects.Num());
	PrintArray(mAllFoundedObjects);
	UE_LOG(AssetDataSubsystemLog, Log, TEXT("--------------------------------------------"));
}

bool UKBFLAssetDataSubsystem::FilterAsset(const FAssetData& AssetData) { return true; }

bool UKBFLAssetDataSubsystem::Local_FilterAsset(const FAssetData& AssetData) const
{
	for (const auto& TagValue : AssetData.TagsAndValues)
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
	EnsureCategoryResolved(2);

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
	EnsureCategoryResolved(2);

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
	EnsureCategoryResolved(2);

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
	EnsureCategoryResolved(2);

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
	EnsureCategoryResolved(2);
	return TArray<TSubclassOf<UFGItemDescriptor>>(mAllFoundedItems.Array());
}

void UKBFLAssetDataSubsystem::GetSchematicsOfTypes(TArray<ESchematicType> Types,
												   TArray<TSubclassOf<UFGSchematic>>& Out_Schematics)
{
	EnsureCategoryResolved(0);

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
	EnsureCategoryResolved(0);

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

TArray<TSubclassOf<UFGSchematic>> UKBFLAssetDataSubsystem::GetAllSchematics()
{
	EnsureCategoryResolved(0);
	return mAllFoundedSchematics.Array();
}

void UKBFLAssetDataSubsystem::GetRecipesOfChilds(TArray<UClass*> Childs, TArray<TSubclassOf<UFGRecipe>>& Out_Items,
												 bool UseNativeCheck)
{
	EnsureCategoryResolved(1);

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
	EnsureCategoryResolved(1);

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
	EnsureCategoryResolved(1);
	return TArray<TSubclassOf<UFGRecipe>>(mAllFoundedRecipes.Array());
}
TArray<TSubclassOf<UFGResearchTree>> UKBFLAssetDataSubsystem::GetAllResearchTrees()
{
	EnsureCategoryResolved(10);
	return TArray<TSubclassOf<UFGResearchTree>>(mAllFoundResearchTrees.Array());
}

void UKBFLAssetDataSubsystem::GetBuildableOfChilds(TArray<UClass*> Childs, TArray<TSubclassOf<AFGBuildable>>& Out_Items,
												   bool UseNativeCheck)
{
	EnsureCategoryResolved(3);

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
	EnsureCategoryResolved(3);
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
	EnsureCategoryResolved(2);
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
	EnsureCategoryResolved(2);
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

void UKBFLAssetDataSubsystem::GetObjectsOfChilds(TArray<UClass*> Childs, TArray<TSubclassOf<UObject>>& Out_Items,
												 bool UseNativeCheck)
{
	// Dedupe via a TSet (O(1)) instead of TArray::AddUnique (O(n)) — Derived can be thousands of classes,
	// and this is called multiple times per CDO overwrite, so AddUnique made it O(n^2).
	TSet<UClass*> Seen;
	Seen.Reserve(Out_Items.Num());
	for (const TSubclassOf<UObject>& Existing : Out_Items)
	{
		if (UClass* ExistingClass = Existing.Get())
		{
			Seen.Add(ExistingClass);
		}
	}

	for (UClass* Parent : Childs)
	{
		if (!IsValid(Parent))
		{
			continue;
		}

		TArray<UClass*> Derived;
		GetDerivedClasses(Parent, Derived, true);

		for (UClass* DerivedClass : Derived)
		{
			if (!DerivedClass || Seen.Contains(DerivedClass))
			{
				continue;
			}

			if (CheckChild(DerivedClass, Childs, UseNativeCheck))
			{
				Seen.Add(DerivedClass);
				Out_Items.Add(DerivedClass);
			}
		}
	}
}

void UKBFLAssetDataSubsystem::GetDriveablePawnsOfChilds(TArray<UClass*> Childs,
														TArray<TSubclassOf<AFGDriveablePawn>>& Out_Items,
														bool UseNativeCheck)
{
	EnsureCategoryResolved(4);

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
	EnsureCategoryResolved(5);

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
	EnsureCategoryResolved(6);

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


FKBFLAssetData UKBFLAssetDataSubsystem::GetModRelatedData(UModModule* ModModule)
{
	EnsureAllResolved();

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
	EnsureCategoryResolved(8);

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

static UClass* GetParentClassForType(int32 Type)
{
	switch (Type)
	{
	case 0:
		return UFGSchematic::StaticClass();
	case 1:
		return UFGRecipe::StaticClass();
	case 2:
		return UFGItemDescriptor::StaticClass();
	case 3:
		return AFGBuildable::StaticClass();
	case 4:
		return AFGDriveablePawn::StaticClass();
	case 5:
		return AFGHologram::StaticClass();
	case 6:
		return UModModule::StaticClass();
	case 8:
		return UFGResourceDescriptor::StaticClass();
	case 10:
		return UFGResearchTree::StaticClass();
	default:
		return nullptr;
	}
}

void UKBFLAssetDataSubsystem::TriggerAsyncLoadsForCategory(int32 Type)
{
	UClass* ParentClass = GetParentClassForType(Type);
	if (!IsValid(ParentClass) || mScannedAssets.IsEmpty())
	{
		return;
	}

	if (!UAssetManager::IsInitialized())
	{
		return;
	}

	TArray<FSoftObjectPath> ToLoad;

	for (const FAssetData& AssetData : mScannedAssets)
	{
		if (!Local_FilterAsset(AssetData))
		{
			continue;
		}

		// Skip assets where the native parent is definitely not related to ParentClass.
		FString NativeParentClassPath;
		if (AssetData.GetTagValue(FBlueprintTags::NativeParentClassPath, NativeParentClassPath))
		{
			FString NativeClassName, NativeClassObjectPath;
			if (FPackageName::ParseExportTextPath(NativeParentClassPath, &NativeClassName, &NativeClassObjectPath))
			{
				if (UClass* NativeClass = FindObject<UClass>(nullptr, *NativeClassObjectPath))
				{
					if (!NativeClass->IsChildOf(ParentClass))
					{
						continue;
					}
				}
			}
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

		// Already in memory — the sync GetDerivedClasses pass already handled it.
		if (FindObject<UClass>(nullptr, *GeneratedClassPath))
		{
			continue;
		}

		ToLoad.Emplace(GeneratedClassPath);
	}

	if (ToLoad.IsEmpty())
	{
		return;
	}

	mPendingAsyncLoads.FindOrAdd(Type).Append(ToLoad);

	UAssetManager::Get().GetStreamableManager().RequestAsyncLoad(
		ToLoad, FStreamableDelegate::CreateUObject(this, &UKBFLAssetDataSubsystem::OnCategoryAsyncLoaded, Type));
}

void UKBFLAssetDataSubsystem::OnCategoryAsyncLoaded(int32 Type)
{
	TArray<FSoftObjectPath>* Paths = mPendingAsyncLoads.Find(Type);
	if (!Paths)
	{
		return;
	}

	for (const FSoftObjectPath& Path : *Paths)
	{
		UClass* Class = FindObject<UClass>(nullptr, *Path.ToString());
		if (!IsValid(Class))
		{
			continue;
		}
		AddAsyncLoadedClassToCategory(Class, Type);
	}

	mPendingAsyncLoads.Remove(Type);
}

void UKBFLAssetDataSubsystem::NotifyClassLoaded(UClass* Class)
{
	if (!IsValid(Class))
	{
		return;
	}

	// Classify into a single category and route. AddAsyncLoadedClassToCategory only adds + broadcasts
	// when the class is genuinely new. Order matters: more-derived bases first (UFGResourceDescriptor
	// derives from UFGItemDescriptor), so check it before the item category.
	int32 Type = -1;
	if (Class->IsChildOf(UFGSchematic::StaticClass()))
	{
		Type = 0;
	}
	else if (Class->IsChildOf(UFGRecipe::StaticClass()))
	{
		Type = 1;
	}
	else if (Class->IsChildOf(UFGResourceDescriptor::StaticClass()))
	{
		Type = 8;
	}
	else if (Class->IsChildOf(UFGItemDescriptor::StaticClass()))
	{
		Type = 2;
	}
	else if (Class->IsChildOf(AFGDriveablePawn::StaticClass()))
	{
		Type = 4;
	}
	else if (Class->IsChildOf(AFGHologram::StaticClass()))
	{
		Type = 5;
	}
	else if (Class->IsChildOf(AFGBuildable::StaticClass()))
	{
		Type = 3;
	}
	else if (Class->IsChildOf(UModModule::StaticClass()))
	{
		Type = 6;
	}
	else if (Class->IsChildOf(UFGResearchTree::StaticClass()))
	{
		Type = 10;
	}

	if (Type != -1)
	{
		AddAsyncLoadedClassToCategory(Class, Type);
	}
}

void UKBFLAssetDataSubsystem::AddAsyncLoadedClassToCategory(UClass* Class, int32 Type)
{
	if (!IsValid(Class))
	{
		return;
	}

	// Default true — only set false by TSet::Add when element is genuinely new.
	bool bWasAlreadyPresent = true;

	switch (Type)
	{
	case 0:
		if (TSubclassOf<UFGSchematic> Sub = Class)
		{
			mAllFoundedSchematics.Add(Sub, &bWasAlreadyPresent);
		}
		break;
	case 1:
		if (TSubclassOf<UFGRecipe> Sub = Class)
		{
			mAllFoundedRecipes.Add(Sub, &bWasAlreadyPresent);
		}
		break;
	case 2:
		if (TSubclassOf<UFGItemDescriptor> Sub = Class)
		{
			mAllFoundedItems.Add(Sub, &bWasAlreadyPresent);
		}
		break;
	case 3:
		if (TSubclassOf<AFGBuildable> Sub = Class)
		{
			mAllFoundedBuildables.Add(Sub, &bWasAlreadyPresent);
		}
		break;
	case 4:
		if (TSubclassOf<AFGDriveablePawn> Sub = Class)
		{
			mAllFoundedDriveablePawns.Add(Sub, &bWasAlreadyPresent);
		}
		break;
	case 5:
		if (TSubclassOf<AFGHologram> Sub = Class)
		{
			mAllFoundedHolograms.Add(Sub, &bWasAlreadyPresent);
		}
		break;
	case 6:
		if (TSubclassOf<UModModule> Sub = Class)
		{
			mAllFoundedModModules.Add(Sub, &bWasAlreadyPresent);
		}
		break;
	case 8:
		if (TSubclassOf<UFGResourceDescriptor> Sub = Class)
		{
			mAllFoundedResourceDescriptors.Add(Sub, &bWasAlreadyPresent);
		}
		break;
	case 10:
		if (TSubclassOf<UFGResearchTree> Sub = Class)
		{
			mAllFoundResearchTrees.Add(Sub, &bWasAlreadyPresent);
		}
		break;
	default:
		return;
	}

	if (!bWasAlreadyPresent)
	{
		mAllFoundedObjects.Add(Class);
		setMapClass(Class, Type);
		BroadcastCategoryEvent(Class, Type);
	}
}

void UKBFLAssetDataSubsystem::BroadcastCategoryEvent(UClass* Class, int32 Type)
{
	switch (Type)
	{
	case 0:
		OnSchematicAdded.Broadcast(Class);
		break;
	case 1:
		OnRecipeAdded.Broadcast(Class);
		break;
	case 2:
		OnItemAdded.Broadcast(Class);
		break;
	case 3:
		OnBuildableAdded.Broadcast(Class);
		break;
	case 4:
		OnDriveablePawnAdded.Broadcast(Class);
		break;
	case 5:
		OnHologramAdded.Broadcast(Class);
		break;
	case 6:
		OnModModuleAdded.Broadcast(Class);
		break;
	case 8:
		OnResourceDescriptorAdded.Broadcast(Class);
		break;
	case 10:
		OnResearchTreeAdded.Broadcast(Class);
		break;
	default:
		break;
	}
}

void UKBFLAssetDataSubsystem::BindOnSchematicAdded(FKBFLOnSchematicAddedEvent Delegate, bool bEnsureLoaded)
{
	if (bEnsureLoaded)
	{
		EnsureCategoryResolved(0);
	}
	for (TSubclassOf<UFGSchematic> Item : mAllFoundedSchematics)
	{
		Delegate.ExecuteIfBound(Item);
	}
	OnSchematicAdded.Add(Delegate);
}

void UKBFLAssetDataSubsystem::BindOnRecipeAdded(FKBFLOnRecipeAddedEvent Delegate, bool bEnsureLoaded)
{
	if (bEnsureLoaded)
	{
		EnsureCategoryResolved(1);
	}
	for (TSubclassOf<UFGRecipe> Item : mAllFoundedRecipes)
	{
		Delegate.ExecuteIfBound(Item);
	}
	OnRecipeAdded.Add(Delegate);
}

void UKBFLAssetDataSubsystem::BindOnItemAdded(FKBFLOnItemAddedEvent Delegate, bool bEnsureLoaded)
{
	if (bEnsureLoaded)
	{
		EnsureCategoryResolved(2);
	}
	for (TSubclassOf<UFGItemDescriptor> Item : mAllFoundedItems)
	{
		Delegate.ExecuteIfBound(Item);
	}
	OnItemAdded.Add(Delegate);
}

void UKBFLAssetDataSubsystem::BindOnBuildableAdded(FKBFLOnBuildableAddedEvent Delegate, bool bEnsureLoaded)
{
	if (bEnsureLoaded)
	{
		EnsureCategoryResolved(3);
	}
	for (TSubclassOf<AFGBuildable> Item : mAllFoundedBuildables)
	{
		Delegate.ExecuteIfBound(Item);
	}
	OnBuildableAdded.Add(Delegate);
}

void UKBFLAssetDataSubsystem::BindOnDriveablePawnAdded(FKBFLOnDriveablePawnAddedEvent Delegate, bool bEnsureLoaded)
{
	if (bEnsureLoaded)
	{
		EnsureCategoryResolved(4);
	}
	for (TSubclassOf<AFGDriveablePawn> Item : mAllFoundedDriveablePawns)
	{
		Delegate.ExecuteIfBound(Item);
	}
	OnDriveablePawnAdded.Add(Delegate);
}

void UKBFLAssetDataSubsystem::BindOnHologramAdded(FKBFLOnHologramAddedEvent Delegate, bool bEnsureLoaded)
{
	if (bEnsureLoaded)
	{
		EnsureCategoryResolved(5);
	}
	for (TSubclassOf<AFGHologram> Item : mAllFoundedHolograms)
	{
		Delegate.ExecuteIfBound(Item);
	}
	OnHologramAdded.Add(Delegate);
}

void UKBFLAssetDataSubsystem::BindOnModModuleAdded(FKBFLOnModModuleAddedEvent Delegate, bool bEnsureLoaded)
{
	if (bEnsureLoaded)
	{
		EnsureCategoryResolved(6);
	}
	for (TSubclassOf<UModModule> Item : mAllFoundedModModules)
	{
		Delegate.ExecuteIfBound(Item);
	}
	OnModModuleAdded.Add(Delegate);
}

void UKBFLAssetDataSubsystem::BindOnResourceDescriptorAdded(FKBFLOnResourceDescriptorAddedEvent Delegate,
															bool bEnsureLoaded)
{
	if (bEnsureLoaded)
	{
		EnsureCategoryResolved(8);
	}
	for (TSubclassOf<UFGResourceDescriptor> Item : mAllFoundedResourceDescriptors)
	{
		Delegate.ExecuteIfBound(Item);
	}
	OnResourceDescriptorAdded.Add(Delegate);
}

void UKBFLAssetDataSubsystem::BindOnResearchTreeAdded(FKBFLOnResearchTreeAddedEvent Delegate, bool bEnsureLoaded)
{
	if (bEnsureLoaded)
	{
		EnsureCategoryResolved(10);
	}
	for (TSubclassOf<UFGResearchTree> Item : mAllFoundResearchTrees)
	{
		Delegate.ExecuteIfBound(Item);
	}
	OnResearchTreeAdded.Add(Delegate);
}

void UKBFLAssetDataSubsystem::BindOnSessionSettingAdded(FKBFLOnSessionSettingAddedEvent Delegate, bool bEnsureLoaded)
{
	if (bEnsureLoaded)
	{
		EnsureCategoryResolved(11);
	}
	for (USMLSessionSetting* Item : mAllFoundAGS)
	{
		Delegate.ExecuteIfBound(Item);
	}
	OnSessionSettingAdded.Add(Delegate);
}
