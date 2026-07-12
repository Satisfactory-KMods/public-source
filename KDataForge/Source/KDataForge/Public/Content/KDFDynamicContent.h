#pragma once

#include "CoreMinimal.h"

class UKDFSubsystem;

/** One generated class/asset recorded in the persistent manifest. */
struct KDATAFORGE_API FKDFGeneratedRecord
{
	FString mPackRef;
	FString mId;
	FString mParentClassPath;
	bool bIsAsset = false;
};

/**
 * Registry for runtime-generated content with save-stable identity.
 *
 * Classes: SML FClassGenerator, package `/KDataForge/Gen/<PackRef>`, class `Gen_<PackRef>_<Id>` —
 * deterministic, id-based, created before any save loads so save class-path resolution hits the
 * in-memory class. Generated classes copy the parent layout; all variation lives in CDO defaults.
 *
 * Assets: PrimaryDataAsset instances in `/KDataForge/GenAssets/<PackRef>`, announced to the asset
 * registry (AssetCreated) so scanning subsystems (KAPI, RefinedRDApi, …) discover them.
 *
 * Persistence: every generated CLASS is recorded in `Saved/KDataForge/GeneratedClasses.json`.
 * Recorded classes missing from the current YAML are recreated as tombstones at the end of the
 * load, so old saves keep loading. Assets are recorded but never tombstoned — recreating a
 * config asset without its YAML values could trip consumer key-field checks (e.g. KAPI fgcheck).
 */
class KDATAFORGE_API FKDFDynamicContentRegistry
{
public:
	void Initialize(UKDFSubsystem* Owner);

	/** Idempotent: returns the existing class for (PackRef, Id) or generates it. */
	UClass* GetOrCreateClass(const FString& PackRef, const FString& Id, UClass* ParentClass, FString& OutError);

	/** Idempotent: returns the existing asset instance for (PackRef, Id) or creates + announces it. */
	UObject* GetOrCreateAsset(const FString& PackRef, const FString& Id, UClass* AssetClass, FString& OutError);

	/** Looks up an already-generated class by its exact (PackRef, Id) pair. Never generates. */
	UClass* FindGeneratedClass(const FString& PackRef, const FString& Id) const;

	/** Looks up an already-generated asset instance by its exact (PackRef, Id) pair. Never creates. */
	UObject* FindGeneratedAsset(const FString& PackRef, const FString& Id) const;

	/**
	 * Every live generated class whose Id matches (case-insensitive), across every pack — used to
	 * resolve a bare id (e.g. `MySuperIngot`) when the caller doesn't know (or didn't specify) which
	 * pack generated it. Sorted by full class path for deterministic "first match" behaviour, same
	 * as the bare class-name shortcut in FKDFValueCodec.
	 */
	void FindGeneratedClassesById(const FString& Id, TArray<UClass*>& OutMatches) const;

	/** Asset-instance counterpart of FindGeneratedClassesById. */
	void FindGeneratedAssetsById(const FString& Id, TArray<UObject*>& OutMatches) const;

	/** Registers FCoreRedirects so saves referencing OldId resolve to NewId (pack.yml `redirects:`). */
	void ApplyRedirects(const FString& PackRef, const TMap<FString, FString>& OldIdToNewId);

	/** Recreates manifest-recorded classes the current YAML no longer defines. Returns tombstone count. */
	int32 ReconstructTombstones(TArray<FString>& OutTombstoneKeys);

	bool IsTombstone(const FString& PackRef, const FString& Id) const;

	/** All generated classes (browser/editor use). */
	void GetAllGeneratedClasses(TArray<UClass*>& OutClasses) const
	{
		for (const TTuple<FString, UClass*>& Pair : mClassesByKey)
		{
			OutClasses.Add(Pair.Value);
		}
	}

	int32 NumGenerated() const { return mClassesByKey.Num() + mAssetsByKey.Num(); }

	static FString SanitizeToken(const FString& Token);
	static FString MakeClassName(const FString& PackRef, const FString& Id);
	static FString MakeClassPackageName(const FString& PackRef);
	static FString MakeAssetPackageName(const FString& PackRef);

private:
	static FString MakeKey(const FString& PackRef, const FString& Id);
	bool ValidateRuntimeIdentity(const FString& PackRef, const FString& Id, bool bIsAsset, FString& OutError) const;
	void LoadManifest();
	void SaveManifest() const;
	void RecordGenerated(const FString& PackRef, const FString& Id, const FString& ParentPath, bool bIsAsset);

	/** Generated classes are root-set (FClassGenerator); assets are retained by the owner subsystem. */
	TMap<FString, UClass*> mClassesByKey;
	TMap<FString, TWeakObjectPtr<UObject>> mAssetsByKey;
	TArray<FKDFGeneratedRecord> mManifest;
	TSet<FString> mTombstoneKeys;
	UKDFSubsystem* mOwner = nullptr;
};
