// Fill out your copyright notice in the Description page of Project Settings.


#include "KBFLWorldModule.h"

#include "KBFLLogging.h"
#include "Subsystems/KBFLCustomizerSubsystem.h"
#include "Subsystems/KBFLResourceNodeSubsystem.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Command/ChatCommandInstance.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "FGResearchTree.h"
#include "FGSchematic.h"
#include "Misc/PackageName.h"

UKBFLWorldModule::UKBFLWorldModule() { bRootModule = false; }

void UKBFLWorldModule::DispatchLifecycleEvent(ELifecyclePhase Phase)
{
	if (Phase == ELifecyclePhase::CONSTRUCTION)
	{
		ConstructionPhase();
	}

	if (Phase == ELifecyclePhase::INITIALIZATION)
	{
		InitPhase();
	}

	if (Phase == ELifecyclePhase::POST_INITIALIZATION)
	{
		PostInitPhase();
	}

	Super::DispatchLifecycleEvent(Phase);
}

void UKBFLWorldModule::InitPhase_Implementation() {}

void UKBFLWorldModule::ConstructionPhase_Implementation() {}

void UKBFLWorldModule::PostInitPhase_Implementation() {}

void UKBFLWorldModule::ScanSchematics()
{
	UE_LOG(LogTemp, Error, TEXT("[KBFL] ScanSchematics button pressed on '%s'"), *GetPathName());
	ScanModAssetsInto<UFGSchematic>(mSchematics, TEXT("Schematic"));
}

void UKBFLWorldModule::ScanResearchTrees()
{
	UE_LOG(LogTemp, Error, TEXT("[KBFL] ScanResearchTrees button pressed on '%s'"), *GetPathName());
	ScanModAssetsInto<UFGResearchTree>(mResearchTrees, TEXT("ResearchTree"));
}

void UKBFLWorldModule::ScanChatCommands()
{
	UE_LOG(LogTemp, Error, TEXT("[KBFL] ScanChatCommands button pressed on '%s'"), *GetPathName());
	ScanModAssetsInto<AChatCommandInstance>(mChatCommands, TEXT("ChatCommand"));
}

template <typename T>
void UKBFLWorldModule::ScanModAssetsInto(TArray<TSubclassOf<T>>& OutArray, const TCHAR* Label)
{
	// Derive this asset's mod plugin root (e.g. "/KLib") from its package path.
	const FString PackageName = GetPackage()->GetName();
	TArray<FString> Segments;
	PackageName.ParseIntoArray(Segments, TEXT("/"), true);
	if (Segments.Num() == 0)
	{
		UE_LOG(KBFLWorldModuleLog, Warning, TEXT("ScanModAssetsInto<%s>: could not derive mod root from '%s'"), Label,
			   *PackageName);
		return;
	}
	const FName MountRoot(*(FString(TEXT("/")) + Segments[0]));

	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(FName("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.bRecursiveClasses = true;
	Filter.PackagePaths.Add(MountRoot);
	Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
	Filter.ClassPaths.Add(UBlueprintGeneratedClass::StaticClass()->GetClassPathName());

	UE_LOG(KBFLWorldModuleLog, Warning, TEXT("ScanModAssetsInto<%s>: package '%s' -> mod root '%s'"), Label,
		   *PackageName, *MountRoot.ToString());

	TArray<FAssetData> Assets;
	AssetRegistry.GetAssets(Filter, Assets);

	UE_LOG(KBFLWorldModuleLog, Warning, TEXT("ScanModAssetsInto<%s>: scanning '%s' found %d blueprint asset(s)"), Label,
		   *MountRoot.ToString(), Assets.Num());

	Modify();

	int32 Added = 0;
	int32 Skipped = 0;
	for (const FAssetData& Asset : Assets)
	{
		FString GeneratedClassExportedPath;
		if (!Asset.GetTagValue(FBlueprintTags::GeneratedClassPath, GeneratedClassExportedPath))
		{
			UE_LOG(KBFLWorldModuleLog, Warning, TEXT("ScanModAssetsInto<%s>: skip '%s' (no GeneratedClassPath tag)"),
				   Label, *Asset.AssetName.ToString());
			Skipped++;
			continue;
		}

		// The tag is export-text wrapped (e.g. BlueprintGeneratedClass'/Game/...X.X_C') — unwrap it.
		FString GeneratedClassPath;
		if (!FPackageName::ParseExportTextPath(GeneratedClassExportedPath, nullptr, &GeneratedClassPath))
		{
			UE_LOG(KBFLWorldModuleLog, Warning,
				   TEXT("ScanModAssetsInto<%s>: skip '%s' (could not parse class path '%s')"), Label,
				   *Asset.AssetName.ToString(), *GeneratedClassExportedPath);
			Skipped++;
			continue;
		}

		UClass* LoadedClass = FSoftClassPath(GeneratedClassPath).TryLoadClass<T>();
		if (!IsValid(LoadedClass) || !LoadedClass->IsChildOf(T::StaticClass()) ||
			LoadedClass->HasAnyClassFlags(CLASS_Abstract))
		{
			UE_LOG(KBFLWorldModuleLog, Warning,
				   TEXT("ScanModAssetsInto<%s>: skip '%s' (not a concrete %s subclass: %s)"), Label,
				   *Asset.AssetName.ToString(), Label, *GeneratedClassPath);
			Skipped++;
			continue;
		}

		const TSubclassOf<UObject> AsObject = LoadedClass;
		if (mBlacklistedClasses.Contains(AsObject))
		{
			UE_LOG(KBFLWorldModuleLog, Warning, TEXT("ScanModAssetsInto<%s>: skip '%s' (blacklisted)"), Label,
				   *LoadedClass->GetName());
			Skipped++;
			continue;
		}

		const TSubclassOf<T> AsTarget = LoadedClass;
		if (OutArray.Contains(AsTarget))
		{
			UE_LOG(KBFLWorldModuleLog, Warning, TEXT("ScanModAssetsInto<%s>: already present '%s'"), Label,
				   *LoadedClass->GetName());
			continue;
		}

		OutArray.Add(AsTarget);
		Added++;
		UE_LOG(KBFLWorldModuleLog, Warning, TEXT("ScanModAssetsInto<%s>: + added '%s'"), Label,
			   *LoadedClass->GetName());
	}

#if WITH_EDITOR
	PostEditChange();
#endif
	MarkPackageDirty();

	UE_LOG(KBFLWorldModuleLog, Warning, TEXT("ScanModAssetsInto<%s>: done '%s' — added %d, skipped %d, total now %d"),
		   Label, *MountRoot.ToString(), Added, Skipped, OutArray.Num());
}
