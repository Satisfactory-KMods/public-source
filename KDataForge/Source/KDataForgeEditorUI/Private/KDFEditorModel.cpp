#include "KDFEditorModel.h"

#include "Algo/Sort.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Blueprint.h"
#include "Engine/DataAsset.h"
#include "Engine/World.h"
#include "Internationalization/Text.h"
#include "KDFNode.h"
#include "KDFYamlParser.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Reflection/KDFOpEngine.h"
#include "Reflection/KDFPropertyPath.h"
#include "Reflection/KDFValueCodec.h"
#include "Registry/ModContentRegistry.h"
#include "Subsystems/KDFSubsystem.h"
#include "UObject/Class.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/TextProperty.h"
#include "UObject/UObjectHash.h"
#include "UObject/UObjectIterator.h"
#include "UObject/UnrealType.h"

namespace
{
	const TCHAR* CategoryRecipes = TEXT("Recipes");
	const TCHAR* CategorySchematics = TEXT("Schematics");
	const TCHAR* CategoryResearch = TEXT("Research");
	const TCHAR* CategoryItems = TEXT("Items");
	const TCHAR* CategoryBuildings = TEXT("Buildings");
	const TCHAR* CategoryDataAssets = TEXT("Data Assets");
	const TCHAR* CategoryGenerated = TEXT("Generated");
	const TCHAR* CategoryAllClasses = TEXT("All Classes");

	const TCHAR* ModFilterAll = TEXT("(All)");
	const TCHAR* ModFilterBaseGame = TEXT("/Game");

	constexpr int32 MaxAutoChildren = 256; // containers larger than this show a summary + raw editing only

	/**
	 * Buckets a class/object path into the mod/content-mount it belongs to, for the browser's mod
	 * filter dropdown:
	 *   /Game/FactoryGame/...                                   -> "/Game"            (base game)
	 *   /RSS/Buildable/.../Foo.Foo_C                             -> "/RSS"             (a mod's own mount)
	 *   /Script/FactoryGame.FGSchematic                          -> "/Script/FactoryGame" (native module)
	 *   /KDataForge/Gen/KDFSample.Gen_KDFSample_Foo_C            -> "/KDataForge/Gen/KDFSample" (one pack)
	 *   /KDataForge/GenAssets/KDFSample.Gen_KDFSample_FooIcon    -> "/KDataForge/GenAssets/KDFSample"
	 * Generated content is grouped by its originating pack rather than the shared "/KDataForge"
	 * mount — the pack is the "mod" a user actually wants to filter by.
	 */
	FString ResolveModKeyFromPath(const FString& Path)
	{
		if (Path.IsEmpty())
		{
			return FString();
		}
		if (Path.StartsWith(TEXT("/Script/")))
		{
			FString Module = Path.RightChop(8); // strip "/Script/"
			int32 DotIndex;
			if (Module.FindChar(TEXT('.'), DotIndex))
			{
				Module.LeftInline(DotIndex);
			}
			return TEXT("/Script/") + Module;
		}

		TArray<FString> Segments;
		Path.ParseIntoArray(Segments, TEXT("/"), true);
		if (Segments.IsEmpty())
		{
			return Path;
		}
		int32 DotIndex;
		if (Segments[0].FindChar(TEXT('.'), DotIndex))
		{
			Segments[0].LeftInline(DotIndex);
		}

		if (Segments[0] == TEXT("KDataForge") && Segments.Num() >= 3 &&
			(Segments[1] == TEXT("Gen") || Segments[1] == TEXT("GenAssets")))
		{
			FString PackRef = Segments[2];
			if (PackRef.FindChar(TEXT('.'), DotIndex))
			{
				PackRef.LeftInline(DotIndex);
			}
			return FString::Printf(TEXT("/KDataForge/%s/%s"), *Segments[1], *PackRef);
		}

		return TEXT("/") + Segments[0];
	}

	bool IsColorStruct(const FProperty* Property)
	{
		const FStructProperty* StructProperty = CastField<FStructProperty>(Property);
		return StructProperty != nullptr &&
			(StructProperty->Struct == TBaseStructure<FLinearColor>::Get() ||
			 StructProperty->Struct == TBaseStructure<FColor>::Get());
	}

	EKDFRowKind ResolveRowKind(const FProperty* Property, TArray<TSharedPtr<FString>>& OutEnumOptions)
	{
		if (Property->IsA<FBoolProperty>())
		{
			return EKDFRowKind::Bool;
		}
		if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
		{
			const UEnum* Enum = EnumProperty->GetEnum();
			for (int32 Index = 0; Index < Enum->NumEnums() - 1; ++Index)
			{
				OutEnumOptions.Add(MakeShared<FString>(Enum->GetNameStringByIndex(Index)));
			}
			return EKDFRowKind::Enum;
		}
		if (const FNumericProperty* Numeric = CastField<FNumericProperty>(Property))
		{
			if (const UEnum* Enum = Numeric->GetIntPropertyEnum())
			{
				for (int32 Index = 0; Index < Enum->NumEnums() - 1; ++Index)
				{
					OutEnumOptions.Add(MakeShared<FString>(Enum->GetNameStringByIndex(Index)));
				}
				return EKDFRowKind::Enum;
			}
			return Numeric->IsFloatingPoint() ? EKDFRowKind::Float : EKDFRowKind::Integer;
		}
		if (Property->IsA<FStrProperty>() || Property->IsA<FNameProperty>() || Property->IsA<FTextProperty>())
		{
			return EKDFRowKind::Text;
		}
		if (Property->IsA<FClassProperty>() || Property->IsA<FSoftClassProperty>())
		{
			return EKDFRowKind::ClassRef;
		}
		if (Property->IsA<FObjectPropertyBase>())
		{
			return EKDFRowKind::ObjectRef;
		}
		if (IsColorStruct(Property))
		{
			return EKDFRowKind::Color;
		}
		if (Property->IsA<FStructProperty>())
		{
			return EKDFRowKind::Struct;
		}
		if (Property->IsA<FArrayProperty>())
		{
			return EKDFRowKind::Array;
		}
		if (Property->IsA<FSetProperty>())
		{
			return EKDFRowKind::Set;
		}
		if (Property->IsA<FMapProperty>())
		{
			return EKDFRowKind::Map;
		}
		return EKDFRowKind::Complex;
	}

	/** Short display for a reference value: object name instead of the full export path. */
	FString FriendlyRefText(const FString& ExportedPath)
	{
		if (ExportedPath.IsEmpty() || ExportedPath == TEXT("None"))
		{
			return TEXT("None");
		}
		FString Path = ExportedPath;
		// ExportText form: Class'/Pkg/Asset.Asset_C' — strip decoration.
		int32 QuoteIndex;
		if (Path.FindChar(TEXT('\''), QuoteIndex))
		{
			Path = Path.Mid(QuoteIndex + 1);
			Path.RemoveFromEnd(TEXT("'"));
		}
		FString Left;
		FString Name = Path;
		Path.Split(TEXT("."), &Left, &Name, ESearchCase::CaseSensitive, ESearchDir::FromEnd);
		return Name;
	}

	/** The picker meta class for a class/soft-class property (what candidates must derive from). */
	UClass* GetPickerMetaClass(const FProperty* Property)
	{
		if (const FClassProperty* ClassProperty = CastField<FClassProperty>(Property))
		{
			return ClassProperty->MetaClass;
		}
		if (const FSoftClassProperty* SoftClassProperty = CastField<FSoftClassProperty>(Property))
		{
			return SoftClassProperty->MetaClass;
		}
		return nullptr;
	}

	UClass* GetPickerObjectClass(const FProperty* Property)
	{
		if (const FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Property))
		{
			return ObjectProperty->PropertyClass;
		}
		return nullptr;
	}

	FString EscapePropertyPathKey(const FString& Key)
	{
		FString Escaped = Key;
		Escaped.ReplaceInline(TEXT("\\"), TEXT("\\\\"));
		Escaped.ReplaceInline(TEXT("\""), TEXT("\\\""));
		return Escaped;
	}
} // namespace

void UKDFEditorModel::Initialize(UWorld* World)
{
	mWorld = World;
	mSelectedObject.Reset();
	mUndoStack.Reset();
	mRedoStack.Reset();
	mCategories = {MakeShared<FString>(CategoryRecipes),   MakeShared<FString>(CategorySchematics),
				   MakeShared<FString>(CategoryResearch),  MakeShared<FString>(CategoryItems),
				   MakeShared<FString>(CategoryBuildings), MakeShared<FString>(CategoryDataAssets),
				   MakeShared<FString>(CategoryGenerated), MakeShared<FString>(CategoryAllClasses)};
	mCreatableKinds = {MakeShared<FString>(TEXT("item")),	   MakeShared<FString>(TEXT("resource")),
					   MakeShared<FString>(TEXT("building")),  MakeShared<FString>(TEXT("recipe")),
					   MakeShared<FString>(TEXT("schematic")), MakeShared<FString>(TEXT("research")),
					   MakeShared<FString>(TEXT("unlock")),	   MakeShared<FString>(TEXT("dataasset")),
					   MakeShared<FString>(TEXT("class"))};
	mRegisterAsOptions = {MakeShared<FString>(TEXT("(none)")), MakeShared<FString>(TEXT("recipe")),
						  MakeShared<FString>(TEXT("schematic")), MakeShared<FString>(TEXT("research"))};
	SetCategory(CategoryRecipes);
}

UKDFSubsystem* UKDFEditorModel::GetKDF() const { return UKDFSubsystem::Get(mWorld.Get()); }

void UKDFEditorModel::SetCategory(const FString& Category)
{
	mActiveCategory = Category;
	RebuildBrowserSource();
	ApplyFilter();
}

void UKDFEditorModel::SetSearchText(const FString& Search)
{
	mSearchText = Search;
	ApplyFilter();
}

void UKDFEditorModel::AddClassItems(const TArray<UClass*>& Classes)
{
	for (UClass* Class : Classes)
	{
		if (!IsValid(Class))
		{
			continue;
		}
		const TSharedRef<FKDFBrowserItem> Item = MakeShared<FKDFBrowserItem>();
		Item->mDisplayName = Class->GetName();
		Item->mSubText = Class->GetPathName();
		Item->mModKey = ResolveModKeyFromPath(Item->mSubText);
		Item->mObject = Class->GetDefaultObject();
		mSourceItems.Add(Item);
	}
}

void UKDFEditorModel::AddSoftAssetItems(const TArray<FAssetData>& Assets, bool bSubTextIsClassName)
{
	TSet<FString> ExistingPaths;
	for (const FKDFBrowserItemPtr& ExistingItem : mSourceItems)
	{
		if (ExistingItem.IsValid() && !ExistingItem->mUnloadedPath.IsEmpty())
		{
			ExistingPaths.Add(ExistingItem->mUnloadedPath);
		}
	}
	for (const FAssetData& AssetData : Assets)
	{
		const FString ObjectPath = AssetData.GetObjectPathString();
		FString LoadPath = ObjectPath;
		FString GeneratedClassExportPath;
		if (AssetData.GetTagValue(FBlueprintTags::GeneratedClassPath, GeneratedClassExportPath))
		{
			LoadPath = FPackageName::ExportTextPathToObjectPath(GeneratedClassExportPath);
		}
		if (LoadPath.IsEmpty() || ExistingPaths.Contains(LoadPath))
		{
			continue;
		}
		const TSharedRef<FKDFBrowserItem> Item = MakeShared<FKDFBrowserItem>();
		Item->mDisplayName = AssetData.AssetName.ToString();
		Item->mSubText =
			bSubTextIsClassName ? AssetData.AssetClassPath.GetAssetName().ToString() : AssetData.GetObjectPathString();
		Item->mModKey = ResolveModKeyFromPath(ObjectPath);
		Item->mUnloadedPath = LoadPath;
		mSourceItems.Add(Item);
		ExistingPaths.Add(LoadPath);
	}
}

void UKDFEditorModel::RebuildBrowserSource()
{
	mSourceItems.Reset();
	UWorld* World = mWorld.Get();
	UKDFSubsystem* KDF = GetKDF();
	if (World == nullptr || KDF == nullptr)
	{
		return;
	}
	UModContentRegistry* Registry = UModContentRegistry::Get(World);

	auto AddRegistrations = [this](const TArray<FGameObjectRegistration>& Registrations)
	{
		TArray<UClass*> Classes;
		for (const FGameObjectRegistration& Registration : Registrations)
		{
			if (UClass* Class = Cast<UClass>(Registration.RegisteredObject.Get()))
			{
				Classes.Add(Class);
			}
		}
		AddClassItems(Classes);
	};

	if (mActiveCategory == CategoryRecipes && Registry != nullptr)
	{
		AddRegistrations(Registry->GetRegisteredRecipes());
	}
	else if (mActiveCategory == CategorySchematics && Registry != nullptr)
	{
		AddRegistrations(Registry->GetRegisteredSchematics());
	}
	else if (mActiveCategory == CategoryResearch && Registry != nullptr)
	{
		AddRegistrations(Registry->GetRegisteredResearchTrees());
	}
	else if (mActiveCategory == CategoryItems || mActiveCategory == CategoryBuildings)
	{
		FString Error;
		UClass* ItemBase = KDF->FindOrLoadClassCached(TEXT("/Script/FactoryGame.FGItemDescriptor"), Error);
		UClass* BuildingBase = KDF->FindOrLoadClassCached(TEXT("/Script/FactoryGame.FGBuildingDescriptor"), Error);
		if (ItemBase != nullptr)
		{
			// Most items/buildings are Blueprint-only and never load until the game references them
			// (recipes, build menu, …) — go straight to the asset registry so the browser lists
			// everything that exists, not just what happens to be in memory this session. Buildings
			// are a subclass of items (FGBuildingDescriptor : FGItemDescriptor), so one Blueprint-asset
			// scan plus a class-hierarchy filter separates them without loading descriptor classes.
			const FAssetRegistryModule& AssetRegistryModule =
				FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
			FARFilter BlueprintFilter;
			BlueprintFilter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
			BlueprintFilter.bRecursiveClasses = true;
			TArray<FAssetData> BlueprintAssets;
			AssetRegistryModule.Get().GetAssets(BlueprintFilter, BlueprintAssets);

			auto GatherClassAssets =
				[&AssetRegistryModule, &BlueprintAssets](UClass* RootClass, TArray<FAssetData>& OutAssets)
			{
				TArray<FTopLevelAssetPath> RootPaths;
				RootPaths.Add(RootClass->GetClassPathName());
				TSet<FTopLevelAssetPath> DerivedClassPaths;
				AssetRegistryModule.Get().GetDerivedClassNames(RootPaths, TSet<FTopLevelAssetPath>(),
															   DerivedClassPaths);
				DerivedClassPaths.Add(RootClass->GetClassPathName());

				// Descriptor Blueprint assets are catalogued as UBlueprint, not as their generated descriptor
				// class. Querying GetAssetsByClass(FGItemDescriptor) therefore only returns native classes in
				// Shipping. The GeneratedClassPath tag maps each Blueprint asset back to the class hierarchy.
				for (const FAssetData& AssetData : BlueprintAssets)
				{
					FString GeneratedClassExportPath;
					if (!AssetData.GetTagValue(FBlueprintTags::GeneratedClassPath, GeneratedClassExportPath))
					{
						continue;
					}
					const FString GeneratedClassPath =
						FPackageName::ExportTextPathToObjectPath(GeneratedClassExportPath);
					if (!GeneratedClassPath.IsEmpty() &&
						DerivedClassPaths.Contains(FTopLevelAssetPath(GeneratedClassPath)))
					{
						OutAssets.Add(AssetData);
					}
				}
			};

			TArray<FAssetData> BuildingAssets;
			TSet<FSoftObjectPath> BuildingPaths;
			if (BuildingBase != nullptr)
			{
				GatherClassAssets(BuildingBase, BuildingAssets);
				for (const FAssetData& AssetData : BuildingAssets)
				{
					BuildingPaths.Add(AssetData.GetSoftObjectPath());
				}
			}
			if (mActiveCategory == CategoryBuildings)
			{
				AddSoftAssetItems(BuildingAssets, false);
			}
			else
			{
				TArray<FAssetData> ItemAssets;
				GatherClassAssets(ItemBase, ItemAssets);
				ItemAssets.RemoveAll([&BuildingPaths](const FAssetData& AssetData)
									 { return BuildingPaths.Contains(AssetData.GetSoftObjectPath()); });
				AddSoftAssetItems(ItemAssets, false);
			}

			// Native (non-Blueprint) subclasses have no asset-registry entry of their own — the
			// registry only catalogs on-disk Blueprint assets — so pick those up the old way. Skip
			// Blueprint classes here even if already loaded: they're already covered above.
			TArray<UClass*> Derived;
			GetDerivedClasses(ItemBase, Derived, true);
			TArray<UClass*> NativeWanted;
			for (UClass* Class : Derived)
			{
				const bool bIsBuilding = BuildingBase != nullptr && Class->IsChildOf(BuildingBase);
				if ((mActiveCategory == CategoryBuildings) == bIsBuilding &&
					!Class->HasAnyClassFlags(CLASS_Abstract | CLASS_CompiledFromBlueprint))
				{
					NativeWanted.Add(Class);
				}
			}
			AddClassItems(NativeWanted);
		}
	}
	else if (mActiveCategory == CategoryDataAssets)
	{
		// Every data asset in the registry (KAPI, RRDA, generated, mod content, …), loaded or not —
		// unlike items/buildings these are already concrete instances in the registry, so one query
		// covers everything; Select() force-loads on demand.
		const FAssetRegistryModule& AssetRegistryModule =
			FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		TArray<FAssetData> Assets;
		AssetRegistryModule.Get().GetAssetsByClass(UDataAsset::StaticClass()->GetClassPathName(), Assets, true);
		AddSoftAssetItems(Assets, true);
	}
	else if (mActiveCategory == CategoryGenerated)
	{
		TArray<UClass*> Generated;
		KDF->GetDynamicContent().GetAllGeneratedClasses(Generated);
		AddClassItems(Generated);
	}
	else if (mActiveCategory == CategoryAllClasses)
	{
		// Every loaded, non-abstract class — the "anything goes" escape hatch. Large (tens of
		// thousands) but the list view is virtualized and the search filter is instant.
		TArray<UClass*> AllClasses;
		for (TObjectIterator<UClass> It; It; ++It)
		{
			if (!It->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
			{
				AllClasses.Add(*It);
			}
		}
		AddClassItems(AllClasses);
	}

	mSourceItems.Sort([](const FKDFBrowserItemPtr& A, const FKDFBrowserItemPtr& B)
					  { return A->mDisplayName < B->mDisplayName; });

	RebuildModOptions();
}

void UKDFEditorModel::RebuildModOptions()
{
	TSet<FString> DistinctMods;
	for (const FKDFBrowserItemPtr& Item : mSourceItems)
	{
		if (!Item->mModKey.IsEmpty())
		{
			DistinctMods.Add(Item->mModKey);
		}
	}
	DistinctMods.Add(ModFilterBaseGame); // always offered, even if this category has no base-game entries yet

	TArray<FString> SortedMods = DistinctMods.Array();
	SortedMods.Sort();

	mModOptions.Reset();
	mModOptions.Add(MakeShared<FString>(ModFilterAll));
	for (const FString& Mod : SortedMods)
	{
		mModOptions.Add(MakeShared<FString>(Mod));
	}

	// Keep the active selection if it still applies to this category; otherwise fall back to "(All)"
	// rather than silently showing an empty list for a mod filter that no longer matches anything.
	const bool bStillValid = mModOptions.ContainsByPredicate([this](const TSharedPtr<FString>& Option)
															 { return *Option == mActiveModFilter; });
	if (!bStillValid)
	{
		mActiveModFilter = ModFilterAll;
	}
}

void UKDFEditorModel::SetModFilter(const FString& Mod)
{
	mActiveModFilter = Mod;
	ApplyFilter();
}

void UKDFEditorModel::ApplyFilter()
{
	mFilteredItems.Reset();
	const bool bModFilterActive = mActiveModFilter != ModFilterAll && !mActiveModFilter.IsEmpty();
	for (const FKDFBrowserItemPtr& Item : mSourceItems)
	{
		if (bModFilterActive && Item->mModKey != mActiveModFilter)
		{
			continue;
		}
		if (mSearchText.IsEmpty() || Item->mDisplayName.Contains(mSearchText) || Item->mSubText.Contains(mSearchText))
		{
			mFilteredItems.Add(Item);
		}
	}
}

void UKDFEditorModel::Select(const FKDFBrowserItemPtr& Item)
{
	if (!Item.IsValid())
	{
		mSelectedObject = nullptr;
	}
	else if (Item->mObject.IsValid())
	{
		mSelectedObject = Item->mObject;
	}
	else if (!Item->mUnloadedPath.IsEmpty())
	{
		// First selection of an asset-registry-only entry: force-load and cache so re-selecting it
		// (or another item that resolves to the same object) doesn't reload.
		UObject* Loaded = FSoftObjectPath(Item->mUnloadedPath).TryLoad();
		UClass* LoadedClass = Cast<UClass>(Loaded);
		if (LoadedClass == nullptr && Loaded == nullptr)
		{
			LoadedClass = FSoftClassPath(Item->mUnloadedPath).TryLoadClass<UObject>();
		}
		if (LoadedClass != nullptr)
		{
			Loaded = LoadedClass->GetDefaultObject();
		}
		Item->mObject = Loaded;
		mSelectedObject = Loaded;
	}
	else
	{
		mSelectedObject = nullptr;
	}
	RefreshPropertyRows();
}

FString UKDFEditorModel::GetSelectedTitle() const
{
	const UObject* Selected = mSelectedObject.Get();
	return Selected != nullptr ? Selected->GetName() : TEXT("Nothing selected");
}

FString UKDFEditorModel::GetSelectedSubtitle() const
{
	const UObject* Selected = mSelectedObject.Get();
	if (Selected == nullptr)
	{
		return FString();
	}
	FString Chain;
	for (const UClass* Class = Selected->GetClass(); Class != nullptr; Class = Class->GetSuperClass())
	{
		Chain += Chain.IsEmpty() ? Class->GetName() : TEXT(" ← ") + Class->GetName();
		if (Class == UObject::StaticClass())
		{
			break;
		}
	}
	return Chain;
}

void UKDFEditorModel::BuildChildren(const FKDFPropertyRowPtr& Row, const FProperty* Property, const void* ValuePtr)
{
	const int32 ChildDepth = Row->mDepth + 1;

	if (const FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Property))
	{
		// Instanced subobjects (e.g. mUnlocks[0] -> a UFGUnlock instance) drill into the
		// instance's own properties, same as a struct's members. Non-instanced object refs
		// (asset/class pickers) never reach here — gated by CPF_PersistentInstance in BuildRow.
		UObject* Instance = ObjectProperty->GetObjectPropertyValue(ValuePtr);
		if (Instance == nullptr)
		{
			return;
		}
		for (TFieldIterator<FProperty> It(Instance->GetClass()); It; ++It)
		{
			Row->mChildren.Add(BuildRow(It->GetAuthoredName(), Row->mPath + TEXT(".") + It->GetName(), *It,
										It->ContainerPtrToValuePtr<void>(Instance), ChildDepth, Row));
		}
		return;
	}
	if (const FStructProperty* StructProperty = CastField<FStructProperty>(Property))
	{
		Row->mSummary = StructProperty->Struct->GetName();
		for (TFieldIterator<FProperty> It(StructProperty->Struct); It; ++It)
		{
			Row->mChildren.Add(BuildRow(It->GetAuthoredName(), Row->mPath + TEXT(".") + It->GetName(), *It,
										It->ContainerPtrToValuePtr<void>(ValuePtr), ChildDepth, Row));
		}
		return;
	}
	if (const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
	{
		FScriptArrayHelper Helper(ArrayProperty, ValuePtr);
		Row->mSummary = FString::Printf(TEXT("%d element(s)"), Helper.Num());
		if (Helper.Num() > MaxAutoChildren)
		{
			return;
		}
		for (int32 Index = 0; Index < Helper.Num(); ++Index)
		{
			const FKDFPropertyRowPtr Child =
				BuildRow(FString::Printf(TEXT("[%d]"), Index), FString::Printf(TEXT("%s[%d]"), *Row->mPath, Index),
						 ArrayProperty->Inner, Helper.GetRawPtr(Index), ChildDepth, Row);
			Child->mElementIndex = Index;
			Child->bIsArrayElement = true;
			Row->mChildren.Add(Child);
		}
		return;
	}
	if (const FSetProperty* SetProperty = CastField<FSetProperty>(Property))
	{
		FScriptSetHelper Helper(SetProperty, ValuePtr);
		Row->mSummary = FString::Printf(TEXT("%d element(s)"), Helper.Num());
		if (Helper.Num() > MaxAutoChildren)
		{
			return;
		}
		int32 LogicalIndex = 0;
		for (int32 Sparse = 0; Sparse < Helper.GetMaxIndex(); ++Sparse)
		{
			if (!Helper.IsValidIndex(Sparse))
			{
				continue;
			}
			Row->mChildren.Add(BuildRow(FString::Printf(TEXT("[%d]"), LogicalIndex),
										FString::Printf(TEXT("%s[%d]"), *Row->mPath, LogicalIndex),
										SetProperty->ElementProp, Helper.GetElementPtr(Sparse), ChildDepth, Row));
			++LogicalIndex;
		}
		return;
	}
	if (const FMapProperty* MapProperty = CastField<FMapProperty>(Property))
	{
		FScriptMapHelper Helper(MapProperty, ValuePtr);
		Row->mSummary = FString::Printf(TEXT("%d pair(s)"), Helper.Num());
		if (Helper.Num() > MaxAutoChildren)
		{
			return;
		}
		for (int32 Sparse = 0; Sparse < Helper.GetMaxIndex(); ++Sparse)
		{
			if (!Helper.IsValidIndex(Sparse))
			{
				continue;
			}
			const FString KeyText = FKDFValueCodec::ExportText(MapProperty->KeyProp, Helper.GetKeyPtr(Sparse));
			const FString EscapedKey = EscapePropertyPathKey(KeyText);
			Row->mChildren.Add(BuildRow(FString::Printf(TEXT("[\"%s\"]"), *EscapedKey),
										FString::Printf(TEXT("%s[\"%s\"]"), *Row->mPath, *EscapedKey),
										MapProperty->ValueProp, Helper.GetValuePtr(Sparse), ChildDepth, Row));
		}
	}
}

FKDFPropertyRowPtr UKDFEditorModel::BuildRow(const FString& Name, const FString& Path, const FProperty* Property,
											 const void* ValuePtr, int32 Depth,
											 const TSharedPtr<FKDFPropertyRowData>& Parent)
{
	UKDFSubsystem* KDF = GetKDF();
	UObject* Selected = mSelectedObject.Get();

	const TSharedRef<FKDFPropertyRowData> Row = MakeShared<FKDFPropertyRowData>();
	Row->mName = Name;
	Row->mPath = Path;
	Row->mTypeName = Property->GetCPPType();
	Row->mDepth = Depth;
	Row->mParent = Parent;
	Row->mKind = ResolveRowKind(Property, Row->mEnumOptions);
	Row->mValueText = FKDFValueCodec::ExportText(Property, ValuePtr);
	if (KDF != nullptr && Selected != nullptr)
	{
		Row->bModified = KDF->GetVanillaCache().FindSnapshot(Selected, Path) != nullptr;
	}

	switch (Row->mKind)
	{
	case EKDFRowKind::Color:
		{
			const FStructProperty* StructProperty = CastField<FStructProperty>(Property);
			if (StructProperty->Struct == TBaseStructure<FLinearColor>::Get())
			{
				Row->mColorValue = *static_cast<const FLinearColor*>(ValuePtr);
			}
			else
			{
				Row->mColorValue = FLinearColor(*static_cast<const FColor*>(ValuePtr));
			}
			break;
		}
	case EKDFRowKind::ClassRef:
		Row->mSummary = FriendlyRefText(Row->mValueText);
		break;
	case EKDFRowKind::Text:
		if (const FTextProperty* TextProperty = CastField<FTextProperty>(Property))
		{
			const FText& Text = TextProperty->GetPropertyValue(ValuePtr);
			if (const TOptional<FString> Key = FTextInspector::GetKey(Text))
			{
				Row->mTextNamespace = FTextInspector::GetNamespace(Text).Get(FString());
				Row->mTextKey = Key.GetValue();
				if (const FString* Source = FTextInspector::GetSourceString(Text))
				{
					Row->mTextSource = *Source;
				}
			}
			else
			{
				Row->mTextSource = Text.ToString();
			}
		}
		break;
	case EKDFRowKind::ObjectRef:
		{
			Row->mSummary = FriendlyRefText(Row->mValueText);
			const FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Property);
			if (ObjectProperty != nullptr && ObjectProperty->HasAnyPropertyFlags(CPF_PersistentInstance))
			{
				BuildChildren(Row, Property, ValuePtr);
			}
			break;
		}
	case EKDFRowKind::Struct:
	case EKDFRowKind::Array:
	case EKDFRowKind::Set:
	case EKDFRowKind::Map:
		BuildChildren(Row, Property, ValuePtr);
		break;
	default:
		break;
	}
	return Row;
}

void UKDFEditorModel::RefreshPropertyRows()
{
	mRootRows.Reset();
	UObject* Selected = mSelectedObject.Get();
	if (Selected == nullptr)
	{
		return;
	}

	TMap<FString, FKDFPropertyRowPtr> CategoryRows;
	for (TFieldIterator<FProperty> It(Selected->GetClass()); It; ++It)
	{
		const FProperty* Property = *It;
		if (Property->HasAnyPropertyFlags(CPF_Deprecated | CPF_EditorOnly | CPF_Transient))
		{
			continue;
		}

		const bool bIsEditableProperty = Property->HasAnyPropertyFlags(CPF_Edit);
		FString Category;
#if WITH_METADATA
		Category = bIsEditableProperty ? Property->GetMetaData(TEXT("Category")) : FString();
#endif
		if (Category.IsEmpty())
		{
			Category = bIsEditableProperty ? TEXT("Default") : TEXT("Non-UPROPERTY / Internal");
		}

		TArray<FString> CategorySegments;
		Category.ParseIntoArray(CategorySegments, TEXT("|"), true);
		if (CategorySegments.IsEmpty())
		{
			CategorySegments.Add(Category);
		}

		FKDFPropertyRowPtr ParentCategory;
		FString CategoryPath;
		for (const FString& Segment : CategorySegments)
		{
			if (!CategoryPath.IsEmpty())
			{
				CategoryPath += TEXT("|");
			}
			CategoryPath += Segment;

			FKDFPropertyRowPtr& CategoryRow = CategoryRows.FindOrAdd(CategoryPath);
			if (!CategoryRow.IsValid())
			{
				CategoryRow = MakeShared<FKDFPropertyRowData>();
				CategoryRow->mName = Segment;
				CategoryRow->mPath = FString::Printf(TEXT("<category>/%s"), *CategoryPath);
				CategoryRow->mTypeName = TEXT("Category");
				CategoryRow->bIsCategory = true;
				if (ParentCategory.IsValid())
				{
					ParentCategory->mChildren.Add(CategoryRow);
				}
				else
				{
					mRootRows.Add(CategoryRow);
				}
			}
			ParentCategory = CategoryRow;
		}

		ParentCategory->mChildren.Add(BuildRow(Property->GetAuthoredName(), Property->GetName(), Property,
											   Property->ContainerPtrToValuePtr<void>(Selected), 0, ParentCategory));
	}
}

const TArray<FKDFPickCandidatePtr>& UKDFEditorModel::GetPickCandidates(const FKDFPropertyRowPtr& Row)
{
	if (Row->bPickCandidatesBuilt)
	{
		return Row->mPickCandidates;
	}
	Row->bPickCandidatesBuilt = true;

	UObject* Selected = mSelectedObject.Get();
	if (Selected == nullptr)
	{
		return Row->mPickCandidates;
	}
	// Re-resolve the property to get its meta/object class (rows don't hold FProperty pointers).
	FString Error;
	FKDFPropertyPath Path;
	FKDFResolvedProperty Resolved;
	if (!FKDFPropertyPath::Parse(Row->mPath, Path, Error) ||
		!FKDFPropertyResolver::Resolve(Selected, Path, Resolved, Error))
	{
		return Row->mPickCandidates;
	}

	Row->mPickCandidates.Add(MakeShared<FKDFPickCandidate>(FKDFPickCandidate{TEXT("None (clear)"), TEXT("None")}));

	const auto AddClassCandidates = [&Row](UClass* BaseClass)
	{
		if (BaseClass == nullptr)
		{
			return;
		}
		TArray<UClass*> CandidateClasses;
		GetDerivedClasses(BaseClass, CandidateClasses, true);
		CandidateClasses.Add(BaseClass);
		for (UClass* Class : CandidateClasses)
		{
			if (!Class->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
			{
				Row->mPickCandidates.Add(
					MakeShared<FKDFPickCandidate>(FKDFPickCandidate{Class->GetName(), Class->GetPathName()}));
			}
		}
	};

	if (Row->mKind == EKDFRowKind::ClassRef)
	{
		AddClassCandidates(GetPickerMetaClass(Resolved.mProperty));
	}
	else if (Row->mKind == EKDFRowKind::ObjectRef)
	{
		UClass* ObjectClass = GetPickerObjectClass(Resolved.mProperty);
		if (ObjectClass != nullptr && ObjectClass != UObject::StaticClass())
		{
			if (Resolved.mProperty->HasAnyPropertyFlags(CPF_InstancedReference | CPF_PersistentInstance))
			{
				AddClassCandidates(ObjectClass);
			}
			else
			{
				// Asset registry candidates — soft paths, nothing is loaded until the pick commits.
				const FAssetRegistryModule& AssetRegistryModule =
					FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
				TArray<FAssetData> Assets;
				AssetRegistryModule.Get().GetAssetsByClass(ObjectClass->GetClassPathName(), Assets, true);
				for (const FAssetData& AssetData : Assets)
				{
					Row->mPickCandidates.Add(MakeShared<FKDFPickCandidate>(
						FKDFPickCandidate{AssetData.AssetName.ToString(), AssetData.GetObjectPathString()}));
				}
			}
		}
	}

	if (Row->mPickCandidates.Num() > 1)
	{
		TArrayView<FKDFPickCandidatePtr> Sortable(Row->mPickCandidates.GetData() + 1, Row->mPickCandidates.Num() - 1);
		Algo::Sort(Sortable, [](const FKDFPickCandidatePtr& A, const FKDFPickCandidatePtr& B)
				   { return A->mDisplayName < B->mDisplayName; });
	}
	return Row->mPickCandidates;
}

bool UKDFEditorModel::CommitRowValue(const FKDFPropertyRowPtr& Row, const FString& NewValueText, FString& OutMessage)
{
	const TSharedRef<FKDFNode> ValueNode = FKDFNode::MakeScalar(NewValueText, true);
	return CommitRowNode(Row, ValueNode, OutMessage);
}

bool UKDFEditorModel::CommitLocalizedText(const FKDFPropertyRowPtr& Row, const FString& Namespace, const FString& Key,
										  const FString& Source, FString& OutMessage)
{
	if (Key.IsEmpty())
	{
		OutMessage = TEXT("Translation key cannot be empty");
		return false;
	}
	const TSharedRef<FKDFNode> ValueNode = FKDFNode::MakeMap();
	ValueNode->SetChild(TEXT("key"), FKDFNode::MakeScalar(Key, true));
	ValueNode->SetChild(TEXT("ns"), FKDFNode::MakeScalar(Namespace, true));
	ValueNode->SetChild(TEXT("source"), FKDFNode::MakeScalar(Source, true));
	return CommitRowNode(Row, ValueNode, OutMessage);
}

bool UKDFEditorModel::CommitRowNode(const FKDFPropertyRowPtr& Row, const TSharedRef<FKDFNode>& ValueNode,
									FString& OutMessage)
{
	UObject* Selected = mSelectedObject.Get();
	UKDFSubsystem* KDF = GetKDF();
	if (!Row.IsValid() || Selected == nullptr || KDF == nullptr)
	{
		OutMessage = TEXT("Nothing selected");
		return false;
	}

	FKDFOpArgs Args;
	Args.mValue = &ValueNode.Get();

	FString PreValue;
	FString PostValue;
	if (!KDF->ApplyEditorOp(Selected, Row->mPath, EKDFOp::Set, Args, PreValue, PostValue, OutMessage))
	{
		return false;
	}
	FKDFUndoEntry& Entry = mUndoStack.AddDefaulted_GetRef();
	Entry.mTarget = Selected;
	Entry.mPath = Row->mPath;
	Entry.mPreValue = PreValue;
	Entry.mPostValue = PostValue;
	mRedoStack.Reset();

	Row->mValueText = PostValue;
	Row->bModified = true;
	OutMessage = FString::Printf(TEXT("%s applied"), *Row->mName);
	return true;
}

bool UKDFEditorModel::ArrayAddElement(const FKDFPropertyRowPtr& ArrayRow, FString& OutMessage)
{
	UObject* Selected = mSelectedObject.Get();
	UKDFSubsystem* KDF = GetKDF();
	if (!ArrayRow.IsValid() || ArrayRow->mKind != EKDFRowKind::Array || Selected == nullptr || KDF == nullptr)
	{
		OutMessage = TEXT("Not an array");
		return false;
	}
	// Compute the array's canonical text with one appended default element, then apply it as a
	// single Set through the standard pipeline (one op record, one undo entry).
	FString Error;
	FKDFPropertyPath Path;
	FKDFResolvedProperty Resolved;
	if (!FKDFPropertyPath::Parse(ArrayRow->mPath, Path, Error) ||
		!FKDFPropertyResolver::Resolve(Selected, Path, Resolved, Error))
	{
		OutMessage = Error;
		return false;
	}
	const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Resolved.mProperty);
	if (ArrayProperty == nullptr)
	{
		OutMessage = TEXT("Not an array property");
		return false;
	}
	FScriptArrayHelper Helper(ArrayProperty, Resolved.mValuePtr);
	const int32 NewIndex = Helper.AddValue();
	const FString PostValue = FKDFValueCodec::ExportText(ArrayProperty, Resolved.mValuePtr);
	Helper.RemoveValues(NewIndex, 1); // restore — the write must go through ApplyEditorOp

	const TSharedRef<FKDFNode> ValueNode = FKDFNode::MakeScalar(PostValue, true);
	FKDFOpArgs Args;
	Args.mValue = &ValueNode.Get();
	FString AppliedPre;
	FString AppliedPost;
	if (!KDF->ApplyEditorOp(Selected, ArrayRow->mPath, EKDFOp::Set, Args, AppliedPre, AppliedPost, OutMessage))
	{
		return false;
	}
	FKDFUndoEntry& Entry = mUndoStack.AddDefaulted_GetRef();
	Entry.mTarget = Selected;
	Entry.mPath = ArrayRow->mPath;
	Entry.mPreValue = AppliedPre;
	Entry.mPostValue = AppliedPost;
	mRedoStack.Reset();

	RefreshPropertyRows();
	OutMessage = FString::Printf(TEXT("Element added to %s"), *ArrayRow->mName);
	return true;
}

bool UKDFEditorModel::ArrayRemoveElement(const FKDFPropertyRowPtr& ElementRow, FString& OutMessage)
{
	UObject* Selected = mSelectedObject.Get();
	UKDFSubsystem* KDF = GetKDF();
	const FKDFPropertyRowPtr Parent = ElementRow.IsValid() ? ElementRow->mParent.Pin() : nullptr;
	if (!ElementRow.IsValid() || !ElementRow->bIsArrayElement || !Parent.IsValid() || Selected == nullptr ||
		KDF == nullptr)
	{
		OutMessage = TEXT("Not an array element");
		return false;
	}

	FString PreValue;
	FString PostValue;
	FKDFOpArgs Args;
	Args.mIndexA = ElementRow->mElementIndex;
	if (!KDF->ApplyEditorOp(Selected, Parent->mPath, EKDFOp::RemoveAt, Args, PreValue, PostValue, OutMessage))
	{
		return false;
	}
	FKDFUndoEntry& Entry = mUndoStack.AddDefaulted_GetRef();
	Entry.mTarget = Selected;
	Entry.mPath = Parent->mPath;
	Entry.mPreValue = PreValue;
	Entry.mPostValue = PostValue;
	mRedoStack.Reset();

	RefreshPropertyRows();
	OutMessage = FString::Printf(TEXT("Removed %s from %s"), *ElementRow->mName, *Parent->mName);
	return true;
}

bool UKDFEditorModel::Undo(FString& OutMessage)
{
	if (mUndoStack.IsEmpty())
	{
		OutMessage = TEXT("Nothing to undo");
		return false;
	}
	const FKDFUndoEntry& Entry = mUndoStack.Last();
	UKDFSubsystem* KDF = GetKDF();
	UObject* Target = Entry.mTarget.Get();
	if (KDF == nullptr || Target == nullptr ||
		!KDF->RestorePropertyText(Target, Entry.mPath, Entry.mPreValue, OutMessage))
	{
		return false;
	}
	const FKDFUndoEntry CompletedEntry = mUndoStack.Pop();
	mRedoStack.Add(CompletedEntry);
	RefreshPropertyRows();
	OutMessage = FString::Printf(TEXT("Undid %s"), *CompletedEntry.mPath);
	return true;
}

bool UKDFEditorModel::Redo(FString& OutMessage)
{
	if (mRedoStack.IsEmpty())
	{
		OutMessage = TEXT("Nothing to redo");
		return false;
	}
	const FKDFUndoEntry& Entry = mRedoStack.Last();
	UKDFSubsystem* KDF = GetKDF();
	UObject* Target = Entry.mTarget.Get();
	if (KDF == nullptr || Target == nullptr ||
		!KDF->RestorePropertyText(Target, Entry.mPath, Entry.mPostValue, OutMessage))
	{
		return false;
	}
	const FKDFUndoEntry CompletedEntry = mRedoStack.Pop();
	mUndoStack.Add(CompletedEntry);
	RefreshPropertyRows();
	OutMessage = FString::Printf(TEXT("Redid %s"), *CompletedEntry.mPath);
	return true;
}

FString UKDFEditorModel::GetSelectedYamlPreview(bool bDiffOnly) const
{
	UObject* Selected = mSelectedObject.Get();
	UKDFSubsystem* KDF = GetKDF();
	if (Selected == nullptr || KDF == nullptr)
	{
		return TEXT("# select an object to preview its YAML");
	}
	FString Yaml;
	FString Error;
	if (!KDF->ExportObjectToYaml(Selected, bDiffOnly, Yaml, Error))
	{
		return FString::Printf(TEXT("# %s"), *Error);
	}
	return Yaml;
}

FString UKDFEditorModel::GetSelectedDiffText() const
{
	UObject* Selected = mSelectedObject.Get();
	UKDFSubsystem* KDF = GetKDF();
	if (Selected == nullptr || KDF == nullptr)
	{
		return TEXT("select an object to see its changes");
	}
	const TMap<FString, FString>* Snapshots = KDF->GetVanillaCache().FindObjectSnapshots(Selected);
	if (Snapshots == nullptr || Snapshots->IsEmpty())
	{
		return TEXT("no changes this session");
	}

	TArray<FString> Paths;
	Snapshots->GenerateKeyArray(Paths);
	Paths.Sort();

	FString Result;
	for (const FString& PathString : Paths)
	{
		FString Current = TEXT("<unresolvable>");
		FKDFPropertyPath Path;
		FKDFResolvedProperty Resolved;
		FString Error;
		if (FKDFPropertyPath::Parse(PathString, Path, Error) &&
			FKDFPropertyResolver::Resolve(Selected, Path, Resolved, Error))
		{
			Current = FKDFValueCodec::ExportText(Resolved.mProperty, Resolved.mValuePtr);
		}
		const FString& Vanilla = (*Snapshots)[PathString];
		if (Vanilla == Current)
		{
			continue; // snapshotted but currently back at vanilla (undo)
		}
		Result += FString::Printf(TEXT("%s\n  vanilla: %s\n  current: %s\n\n"), *PathString, *Vanilla, *Current);
	}
	return Result.IsEmpty() ? TEXT("no changes this session") : Result;
}

FString UKDFEditorModel::GetSelectedReferencesText() const
{
	UObject* Selected = mSelectedObject.Get();
	UWorld* World = mWorld.Get();
	if (Selected == nullptr || World == nullptr)
	{
		return TEXT("select an object to see its references");
	}
	UModContentRegistry* Registry = UModContentRegistry::Get(World);
	if (Registry == nullptr)
	{
		return TEXT("content registry unavailable");
	}
	// The selection is a CDO or asset; references track its CLASS (recipes reference classes).
	const UClass* SelectedClass = Selected->GetClass();
	const FString ClassPath = SelectedClass->GetPathName();

	TArray<FString> IngredientIn;
	TArray<FString> ProductIn;
	TArray<FString> ProducedInThis;
	for (const FGameObjectRegistration& Registration : Registry->GetRegisteredRecipes())
	{
		UClass* RecipeClass = Cast<UClass>(Registration.RegisteredObject.Get());
		if (RecipeClass == nullptr)
		{
			continue;
		}
		// Reflection-only scan: recipes hold FItemAmount arrays + a producer class array; matching
		// on the exported text keeps this independent of FGRecipe's exact layout.
		const UObject* RecipeCDO = RecipeClass->GetDefaultObject();
		auto PropertyContains = [&](const TCHAR* PropertyName)
		{
			const FProperty* Property = RecipeClass->FindPropertyByName(PropertyName);
			if (Property == nullptr)
			{
				return false;
			}
			FString Exported;
			Property->ExportTextItem_Direct(Exported, Property->ContainerPtrToValuePtr<void>(RecipeCDO), nullptr,
											nullptr, PPF_None);
			return Exported.Contains(ClassPath);
		};
		if (PropertyContains(TEXT("mIngredients")))
		{
			IngredientIn.Add(RecipeClass->GetName());
		}
		if (PropertyContains(TEXT("mProduct")))
		{
			ProductIn.Add(RecipeClass->GetName());
		}
		if (PropertyContains(TEXT("mProducedIn")))
		{
			ProducedInThis.Add(RecipeClass->GetName());
		}
	}

	TArray<FString> UnlockedBy;
	for (const FGameObjectRegistration& Registration : Registry->GetRegisteredSchematics())
	{
		UClass* SchematicClass = Cast<UClass>(Registration.RegisteredObject.Get());
		if (SchematicClass == nullptr)
		{
			continue;
		}
		const UObject* SchematicCDO = SchematicClass->GetDefaultObject();
		const FProperty* UnlocksProperty = SchematicClass->FindPropertyByName(TEXT("mUnlocks"));
		if (UnlocksProperty == nullptr)
		{
			continue;
		}
		FString Exported;
		UnlocksProperty->ExportTextItem_Direct(Exported, UnlocksProperty->ContainerPtrToValuePtr<void>(SchematicCDO),
											   nullptr, nullptr, PPF_None);
		if (Exported.Contains(ClassPath) ||
			(IngredientIn.Num() + ProductIn.Num() > 0 &&
			 [&Exported, &IngredientIn, &ProductIn]()
			 {
				 for (const FString& Recipe : IngredientIn)
				 {
					 if (Exported.Contains(Recipe))
					 {
						 return true;
					 }
				 }
				 for (const FString& Recipe : ProductIn)
				 {
					 if (Exported.Contains(Recipe))
					 {
						 return true;
					 }
				 }
				 return false;
			 }()))
		{
			UnlockedBy.Add(SchematicClass->GetName());
		}
	}

	auto Section = [](const TCHAR* Title, const TArray<FString>& Names)
	{
		if (Names.IsEmpty())
		{
			return FString();
		}
		return FString::Printf(TEXT("%s\n  %s\n\n"), Title, *FString::Join(Names, TEXT("\n  ")));
	};
	const FString Result = Section(TEXT("Ingredient in:"), IngredientIn) + Section(TEXT("Product of:"), ProductIn) +
		Section(TEXT("Recipes produced in this:"), ProducedInThis) +
		Section(TEXT("Unlocked / related schematics:"), UnlockedBy);
	return Result.IsEmpty() ? TEXT("no references found in registered content") : Result;
}

bool UKDFEditorModel::ExportSelected(bool bDiffOnly, FString& OutMessage)
{
	UObject* Selected = mSelectedObject.Get();
	UKDFSubsystem* KDF = GetKDF();
	if (Selected == nullptr || KDF == nullptr)
	{
		OutMessage = TEXT("Nothing selected");
		return false;
	}
	FString Yaml;
	if (!KDF->ExportObjectToYaml(Selected, bDiffOnly, Yaml, OutMessage))
	{
		return false;
	}
	const FString Directory =
		FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()) / TEXT("DataForge") / TEXT("exported");
	const FString FilePath = Directory / (Selected->GetName() + TEXT(".cdo.yml"));
	if (!FFileHelper::SaveStringToFile(Yaml, *FilePath))
	{
		OutMessage = FString::Printf(TEXT("Could not write %s"), *FilePath);
		return false;
	}
	OutMessage = FString::Printf(TEXT("Exported to %s"), *FilePath);
	return true;
}

bool UKDFEditorModel::CreateContentDraft(const FString& Kind, const FString& Id, const FString& ParentOrClassPath,
										 const FString& RegisterAs, FString& OutMessage)
{
	UKDFSubsystem* KDF = GetKDF();
	if (KDF == nullptr)
	{
		OutMessage = TEXT("Subsystem unavailable");
		return false;
	}
	if (Id.IsEmpty() ||
		!mCreatableKinds.ContainsByPredicate([&Kind](const TSharedPtr<FString>& Entry) { return *Entry == Kind; }))
	{
		OutMessage = TEXT("Pick a kind and enter an id");
		return false;
	}

	const bool bIsDataAsset = Kind == TEXT("dataasset");
	const bool bIsGenericClass = Kind == TEXT("class");
	if ((bIsDataAsset || bIsGenericClass || Kind == TEXT("unlock")) && ParentOrClassPath.IsEmpty())
	{
		// These kinds have no default parent in their handlers — the draft would fail validation.
		OutMessage = FString::Printf(TEXT("Kind '%s' requires a %s"), *Kind,
									 bIsDataAsset ? TEXT("class") : TEXT("parent class"));
		return false;
	}

	const TSharedRef<FKDFNode> Document = FKDFNode::MakeMap();
	Document->SetChild(TEXT("type"), FKDFNode::MakeScalar(Kind, false));
	const TSharedRef<FKDFNode> Entry = FKDFNode::MakeMap();
	Entry->SetChild(TEXT("id"), FKDFNode::MakeScalar(Id, false));
	if (!ParentOrClassPath.IsEmpty())
	{
		Entry->SetChild(bIsDataAsset ? TEXT("class") : TEXT("parent"), FKDFNode::MakeScalar(ParentOrClassPath, true));
	}
	if (bIsGenericClass && !RegisterAs.IsEmpty() && RegisterAs != TEXT("(none)"))
	{
		Entry->SetChild(TEXT("registerAs"), FKDFNode::MakeScalar(RegisterAs, false));
	}
	const TSharedRef<FKDFNode> Entries = FKDFNode::MakeSequence();
	Entries->AddChild(Entry);
	// Plural document keys per schema: items/resources/buildings/recipes/schematics/research/
	// unlocks/assets/classes. Bare `+ "s"` covers most; irregulars are special-cased.
	FString EntriesKey = Kind + TEXT("s");
	if (Kind == TEXT("research"))
	{
		EntriesKey = TEXT("research");
	}
	else if (bIsDataAsset)
	{
		EntriesKey = TEXT("assets");
	}
	else if (bIsGenericClass)
	{
		EntriesKey = TEXT("classes");
	}
	Document->SetChild(EntriesKey, Entries);

	const FString Directory =
		FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()) / TEXT("DataForge") / TEXT("created");
	const FString FilePath = Directory / FString::Printf(TEXT("%s.%s.yml"), *Id, *Kind);
	if (!FFileHelper::SaveStringToFile(FKDFYamlParser::EmitString(Document.Get()), *FilePath))
	{
		OutMessage = FString::Printf(TEXT("Could not write %s"), *FilePath);
		return false;
	}

	OutMessage = FString::Printf(TEXT("Created %s — restart the session to load the new %s content"), *FilePath, *Kind);
	return true;
}

const TArray<FKDFPickCandidatePtr>& UKDFEditorModel::GetCreateParentCandidates(const FString& Kind)
{
	if (const TArray<FKDFPickCandidatePtr>* Cached = mCreateParentCandidates.Find(Kind))
	{
		return *Cached;
	}

	// Mirrors the base-class restriction each content handler enforces at apply time, so the
	// picker only offers parents the draft would actually validate against.
	static const TMap<FString, FString> KindBases = {
		{TEXT("item"), TEXT("/Script/FactoryGame.FGItemDescriptor")},
		{TEXT("resource"), TEXT("/Script/FactoryGame.FGResourceDescriptor")},
		{TEXT("building"), TEXT("/Script/FactoryGame.FGBuildingDescriptor")},
		{TEXT("recipe"), TEXT("/Script/FactoryGame.FGRecipe")},
		{TEXT("schematic"), TEXT("/Script/FactoryGame.FGSchematic")},
		{TEXT("research"), TEXT("/Script/FactoryGame.FGResearchTree")},
		{TEXT("unlock"), TEXT("/Script/FactoryGame.FGUnlock")},
		{TEXT("dataasset"), TEXT("/Script/Engine.DataAsset")},
	};

	UClass* BaseClass = nullptr;
	if (const FString* BasePath = KindBases.Find(Kind))
	{
		BaseClass = FindObject<UClass>(nullptr, **BasePath);
	}

	TArray<FKDFPickCandidatePtr>& Candidates = mCreateParentCandidates.Add(Kind);
	for (TObjectIterator<UClass> It; It; ++It)
	{
		UClass* Class = *It;
		if (Class->HasAnyClassFlags(CLASS_Deprecated | CLASS_NewerVersionExists) ||
			Class->GetName().StartsWith(TEXT("SKEL_")) || Class->GetName().StartsWith(TEXT("REINST_")))
		{
			continue;
		}
		// Data assets are instantiated directly — abstract classes cannot be a `class:`; as a
		// generation PARENT abstract is fine (the generated child is concrete).
		if (Kind == TEXT("dataasset") && Class->HasAnyClassFlags(CLASS_Abstract))
		{
			continue;
		}
		if (BaseClass != nullptr && !Class->IsChildOf(BaseClass))
		{
			continue;
		}
		const FKDFPickCandidatePtr Candidate = MakeShared<FKDFPickCandidate>();
		Candidate->mDisplayName = Class->GetName();
		Candidate->mPath = Class->GetPathName();
		Candidates.Add(Candidate);
	}
	Candidates.Sort([](const FKDFPickCandidatePtr& A, const FKDFPickCandidatePtr& B)
					{ return A->mDisplayName < B->mDisplayName; });
	return Candidates;
}

int32 UKDFEditorModel::GetSessionEditCount() const
{
	const UKDFSubsystem* KDF = GetKDF();
	return KDF != nullptr ? KDF->GetEditorPatchRecord().mOps.Num() : 0;
}
