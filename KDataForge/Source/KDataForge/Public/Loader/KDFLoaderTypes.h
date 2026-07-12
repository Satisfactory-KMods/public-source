#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "KDFNode.h"

/** Content kinds that get registered into SML's UModContentRegistry per world. */
enum class EKDFContentRegKind : uint8
{
	None,
	Recipe,
	Schematic,
	ResearchTree
};

/**
 * One content pack: a `DataForge` root directory, optionally described by a `pack.yml` manifest.
 * Files without a manifest form an implicit pack named after the DataForge root's parent directory.
 */
struct KDATAFORGE_API FKDFPack
{
	/** Stable pack reference — the contract for save persistence and conflict reports. */
	FString mRef;

	FString mName;
	FString mVersion;

	/** Higher priority packs apply earlier within a stage. */
	int32 mPriority = 100;

	bool bEnabled = true;

	/** Debug logging for every document of this pack (pack.yml `debug:`). */
	bool bDebug = false;

	/** Absolute path of the DataForge root directory. */
	FString mDirectory;

	/**
	 * pack.yml `conditions:` block — same grammar as a document's `conditions:` (see
	 * FKDFConditionEvaluator). All conditions must pass (logical AND) or the entire pack — every
	 * document under it — is skipped. Null means "no conditions" → always applies.
	 */
	TSharedPtr<FKDFNode> mConditionsNode;

	/** Pack refs that must load before this pack. */
	TArray<FString> mDependencies;

	/** Generated-content id renames (old → new) — fed to FCoreRedirects for save compatibility. */
	TMap<FString, FString> mRedirects;
};

/** One parsed YAML document, ready for stage dispatch. */
struct KDATAFORGE_API FKDFDocument
{
	FString mAbsoluteFile;

	/** File path relative to the DataForge root (used in diagnostics and deterministic ordering). */
	FString mRelativeFile;

	FString mPackRef;
	int32 mPackPriority = 100;

	/** Root type detected from the document's `type:` key (or `name.<type>.yml` filename convention). */
	FName mRootType;

	TSharedPtr<FKDFNode> mRoot;

	/** Relative files (same pack) that must apply before this one (`include:`). */
	TArray<FString> mIncludeDependencies;

	/** Debug op logging for this document (document `debug:` or pack `debug:`). */
	bool bDebug = false;
};

/**
 * Runtime actor patch: property ops re-applied to every spawning actor instance whose class matches.
 * Registered by cdo documents with `applyToSpawnedActors: true`; consumed by UKDFActorPatchSubsystem.
 */
struct KDATAFORGE_API FKDFActorPatch
{
	TWeakObjectPtr<UClass> mTargetClass;

	bool bIncludeSubclasses = true;

	/** The patch's `properties:` sequence node (shared with the parsed document, kept alive by this ref). */
	TSharedPtr<FKDFNode> mPropertiesNode;

	FString mSourceFile;

	/** Pack the patch's document belongs to — scopes bare generated-content id resolution on re-apply. */
	FString mPackRef;
};

/**
 * Watches for classes that load after a cdo patch's initial apply pass — KBFL parity
 * (UKBFLContentCDOHelperSubsystem's lazy-load CDO re-apply). Blueprint-authored item/building/
 * recipe classes are frequently not loaded into memory until the game actually references them, so
 * `GetDerivedClasses` at apply time only ever sees a subset of the real subclass set. Registered by
 * `applyToSubclasses: true` and `matchTag` + `ofClass` patches (ancestor-matched, via mBaseClass), and
 * by a plain `target:` class path that failed to resolve at all — its owning mod's content may not
 * be ready yet this early in the lifecycle (exact-path-matched, via mExactTargetPath). Consumed by
 * `UKDFSubsystem::NotifyUObjectCreated`, which re-applies the same ops to any later-loaded class
 * that matches.
 */
struct KDATAFORGE_API FKDFLazyClassWatch
{
	/** Ancestor class — the patch's `target:` (with `applyToSubclasses`) or `ofClass:` scope root. */
	TWeakObjectPtr<UClass> mBaseClass;

	/**
	 * A `target:` class path that failed to resolve at apply time — the mod that owns it may not be
	 * fully ready yet this early in the lifecycle (its content pak not mounted, its own module Init
	 * still pending, …). Matched by exact path against a newly loaded class, not ancestry, since we
	 * never got a UClass* to compare against. Mutually exclusive with mBaseClass — set exactly one.
	 */
	FString mExactTargetPath;

	/** Ops to (re-)apply to a newly loaded matching class's CDO. */
	TSharedPtr<FKDFNode> mPropertiesNode;

	bool bPropagate = true;

	/** Empty: plain `applyToSubclasses` watch. Non-empty: the CDO must carry every tag (`matchTag`). */
	TArray<FGameplayTag> mMatchTags;

	FString mTagPropertyName;
	FString mSourceFile;
	FString mPackRef;
	bool bDebug = false;
};
