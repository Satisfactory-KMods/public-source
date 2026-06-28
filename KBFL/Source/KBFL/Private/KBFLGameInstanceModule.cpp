// Copyright Coffee Stain Studios. All Rights Reserved.

#include "KBFLGameInstanceModule.h"

#include "KBFLLogging.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Configuration/ModConfiguration.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "FGRemoteCallObject.h"
#include "Misc/PackageName.h"
#include "SessionSettings/SessionSetting.h"


UKBFLGameInstanceModule::UKBFLGameInstanceModule() { bRootModule = false; }

void UKBFLGameInstanceModule::DispatchLifecycleEvent(ELifecyclePhase Phase)
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

bool UKBFLGameInstanceModule::IsOwnerModObject(UObject* Object) const
{
	TArray<FString> DirectoryArray;
	Object->GetPathName().ParseIntoArray(DirectoryArray, TEXT("/"));
	FName ModName = FName(DirectoryArray[0]);
	return ModName == GetOwnerModReference();
}

void UKBFLGameInstanceModule::PostInitPhase_Implementation() {}

void UKBFLGameInstanceModule::InitPhase_Implementation() {}

void UKBFLGameInstanceModule::ConstructionPhase_Implementation() {}

void UKBFLGameInstanceModule::ScanModConfigurations()
{
	UE_LOG(LogTemp, Error, TEXT("[KBFL] ScanModConfigurations button pressed on '%s'"), *GetPathName());
	ScanModClassesInto<UModConfiguration>(ModConfigurations, TEXT("ModConfiguration"));
}

void UKBFLGameInstanceModule::ScanRemoteCallObjects()
{
	UE_LOG(LogTemp, Error, TEXT("[KBFL] ScanRemoteCallObjects button pressed on '%s'"), *GetPathName());
	ScanModClassesInto<UFGRemoteCallObject>(RemoteCallObjects, TEXT("RemoteCallObject"));
}

void UKBFLGameInstanceModule::ScanSessionSettings()
{
	UE_LOG(LogTemp, Error, TEXT("[KBFL] ScanSessionSettings button pressed on '%s'"), *GetPathName());
	ScanModInstancesInto<USMLSessionSetting>(SessionSettings, TEXT("SessionSetting"));
}

template <typename T>
void UKBFLGameInstanceModule::ScanModClassesInto(TArray<TSubclassOf<T>>& OutArray, const TCHAR* Label)
{
	// Derive this asset's mod plugin root (e.g. "/KLib") from its package path.
	const FString PackageName = GetPackage()->GetName();
	TArray<FString> Segments;
	PackageName.ParseIntoArray(Segments, TEXT("/"), true);
	if (Segments.Num() == 0)
	{
		UE_LOG(KBFLGameInstanceModuleLog, Warning, TEXT("ScanModClassesInto<%s>: could not derive mod root from '%s'"),
			   Label, *PackageName);
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

	UE_LOG(KBFLGameInstanceModuleLog, Warning, TEXT("ScanModClassesInto<%s>: package '%s' -> mod root '%s'"), Label,
		   *PackageName, *MountRoot.ToString());

	TArray<FAssetData> Assets;
	AssetRegistry.GetAssets(Filter, Assets);

	UE_LOG(KBFLGameInstanceModuleLog, Warning,
		   TEXT("ScanModClassesInto<%s>: scanning '%s' found %d blueprint asset(s)"), Label, *MountRoot.ToString(),
		   Assets.Num());

	Modify();

	int32 Added = 0;
	int32 Skipped = 0;
	for (const FAssetData& Asset : Assets)
	{
		FString GeneratedClassExportedPath;
		if (!Asset.GetTagValue(FBlueprintTags::GeneratedClassPath, GeneratedClassExportedPath))
		{
			UE_LOG(KBFLGameInstanceModuleLog, Warning,
				   TEXT("ScanModClassesInto<%s>: skip '%s' (no GeneratedClassPath tag)"), Label,
				   *Asset.AssetName.ToString());
			Skipped++;
			continue;
		}

		// The tag is export-text wrapped (e.g. BlueprintGeneratedClass'/Game/...X.X_C') — unwrap it.
		FString GeneratedClassPath;
		if (!FPackageName::ParseExportTextPath(GeneratedClassExportedPath, nullptr, &GeneratedClassPath))
		{
			UE_LOG(KBFLGameInstanceModuleLog, Warning,
				   TEXT("ScanModClassesInto<%s>: skip '%s' (could not parse class path '%s')"), Label,
				   *Asset.AssetName.ToString(), *GeneratedClassExportedPath);
			Skipped++;
			continue;
		}

		UClass* LoadedClass = FSoftClassPath(GeneratedClassPath).TryLoadClass<T>();
		if (!IsValid(LoadedClass) || !LoadedClass->IsChildOf(T::StaticClass()) ||
			LoadedClass->HasAnyClassFlags(CLASS_Abstract))
		{
			UE_LOG(KBFLGameInstanceModuleLog, Warning,
				   TEXT("ScanModClassesInto<%s>: skip '%s' (not a concrete %s subclass: %s)"), Label,
				   *Asset.AssetName.ToString(), Label, *GeneratedClassPath);
			Skipped++;
			continue;
		}

		const TSubclassOf<UObject> AsObject = LoadedClass;
		if (mBlacklistedClasses.Contains(AsObject))
		{
			UE_LOG(KBFLGameInstanceModuleLog, Warning, TEXT("ScanModClassesInto<%s>: skip '%s' (blacklisted)"), Label,
				   *LoadedClass->GetName());
			Skipped++;
			continue;
		}

		const TSubclassOf<T> AsTarget = LoadedClass;
		if (OutArray.Contains(AsTarget))
		{
			UE_LOG(KBFLGameInstanceModuleLog, Warning, TEXT("ScanModClassesInto<%s>: already present '%s'"), Label,
				   *LoadedClass->GetName());
			continue;
		}

		OutArray.Add(AsTarget);
		Added++;
		UE_LOG(KBFLGameInstanceModuleLog, Warning, TEXT("ScanModClassesInto<%s>: + added '%s'"), Label,
			   *LoadedClass->GetName());
	}

#if WITH_EDITOR
	PostEditChange();
#endif
	MarkPackageDirty();

	UE_LOG(KBFLGameInstanceModuleLog, Warning,
		   TEXT("ScanModClassesInto<%s>: done '%s' — added %d, skipped %d, total now %d"), Label, *MountRoot.ToString(),
		   Added, Skipped, OutArray.Num());
}

template <typename T>
void UKBFLGameInstanceModule::ScanModInstancesInto(TArray<TObjectPtr<T>>& OutArray, const TCHAR* Label)
{
	// Derive this asset's mod plugin root (e.g. "/KLib") from its package path.
	const FString PackageName = GetPackage()->GetName();
	TArray<FString> Segments;
	PackageName.ParseIntoArray(Segments, TEXT("/"), true);
	if (Segments.Num() == 0)
	{
		UE_LOG(KBFLGameInstanceModuleLog, Warning,
			   TEXT("ScanModInstancesInto<%s>: could not derive mod root from '%s'"), Label, *PackageName);
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
	Filter.ClassPaths.Add(T::StaticClass()->GetClassPathName());

	UE_LOG(KBFLGameInstanceModuleLog, Warning, TEXT("ScanModInstancesInto<%s>: package '%s' -> mod root '%s'"), Label,
		   *PackageName, *MountRoot.ToString());

	TArray<FAssetData> Assets;
	AssetRegistry.GetAssets(Filter, Assets);

	UE_LOG(KBFLGameInstanceModuleLog, Warning, TEXT("ScanModInstancesInto<%s>: scanning '%s' found %d asset(s)"), Label,
		   *MountRoot.ToString(), Assets.Num());

	Modify();

	int32 Added = 0;
	int32 Skipped = 0;
	for (const FAssetData& Asset : Assets)
	{
		T* Instance = Cast<T>(Asset.GetAsset());
		if (!IsValid(Instance))
		{
			UE_LOG(KBFLGameInstanceModuleLog, Warning,
				   TEXT("ScanModInstancesInto<%s>: skip '%s' (failed to load as %s)"), Label,
				   *Asset.AssetName.ToString(), Label);
			Skipped++;
			continue;
		}

		const TSubclassOf<UObject> AsObject = Instance->GetClass();
		if (mBlacklistedClasses.Contains(AsObject))
		{
			UE_LOG(KBFLGameInstanceModuleLog, Warning, TEXT("ScanModInstancesInto<%s>: skip '%s' (class blacklisted)"),
				   Label, *Instance->GetName());
			Skipped++;
			continue;
		}

		if (OutArray.Contains(Instance))
		{
			UE_LOG(KBFLGameInstanceModuleLog, Warning, TEXT("ScanModInstancesInto<%s>: already present '%s'"), Label,
				   *Instance->GetName());
			continue;
		}

		OutArray.Add(Instance);
		Added++;
		UE_LOG(KBFLGameInstanceModuleLog, Warning, TEXT("ScanModInstancesInto<%s>: + added '%s'"), Label,
			   *Instance->GetName());
	}

#if WITH_EDITOR
	PostEditChange();
#endif
	MarkPackageDirty();

	UE_LOG(KBFLGameInstanceModuleLog, Warning,
		   TEXT("ScanModInstancesInto<%s>: done '%s' — added %d, skipped %d, total now %d"), Label,
		   *MountRoot.ToString(), Added, Skipped, OutArray.Num());
}
