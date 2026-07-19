#pragma once

#include "Containers/Ticker.h"
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UObject/UObjectArray.h"

#include <atomic>

#include "Content/KDFDynamicContent.h"
#include "KDFDataEditorHandler.h"
#include "KDFTypes.h"
#include "Loader/KDFLoaderTypes.h"
#include "Reflection/KDFOpEngine.h"
#include "Reflection/KDFVanillaCache.h"

#include "KDFSubsystem.generated.h"

class APlayerController;
class UDataTable;
class UWorld;

/** Broadcast when something (chat command, console) asks the in-game editor to toggle. */
DECLARE_MULTICAST_DELEGATE_OneParam(FKDFOnToggleEditor, APlayerController* /*Requester — may be null*/);

/** Queued per-world registration of a generated/registered content class. */
USTRUCT()
struct FKDFContentRegRequest
{
	GENERATED_BODY()

	UPROPERTY()
	uint8 mKind = 0; // EKDFContentRegKind

	UPROPERTY()
	TObjectPtr<UClass> mClass;

	UPROPERTY()
	FString mSourceFile;
};

/** Queued per-world registration of an AWESOME Sink point table. */
USTRUCT()
struct FKDFSinkTableRequest
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UDataTable> mTable;

	UPROPERTY()
	uint8 mTrack = 0; // EResourceSinkTrack

	UPROPERTY()
	FString mSourceFile;
};

/**
 * Central KDataForge subsystem: owns the loader pipeline, handler registry, reflection caches,
 * vanilla snapshots, patch records, and registered runtime actor patches.
 *
 * Lifecycle (driven by UKDFGameInstanceModule):
 *  - CONSTRUCTION: built-in handlers registered (subsystem Initialize); third-party mods register theirs.
 *  - INITIALIZATION: RunInitialLoad() — scan, parse (parallel), validate, apply stages in order.
 *  - POST_INITIALIZATION: load report logged.
 */
UCLASS()
class KDATAFORGE_API UKDFSubsystem : public UGameInstanceSubsystem, public FUObjectArray::FUObjectCreateListener
{
	GENERATED_BODY()

public:
	static UKDFSubsystem* Get(const UObject* WorldContextObject);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/**
	 * Registers a handler object (must implement IKDFDataEditorHandler).
	 * Call during the CONSTRUCTION lifecycle phase of your game instance module.
	 * The subsystem retains the object against GC.
	 */
	UFUNCTION(BlueprintCallable, Category = "KDataForge")
	void RegisterHandler(UObject* HandlerObject);

	/** Full scan + parse + apply. Called once from the INITIALIZATION lifecycle phase. */
	void RunInitialLoad();

	/** Human-readable load report (packs, documents, ops, diagnostics). */
	UFUNCTION(BlueprintCallable, Category = "KDataForge")
	FString BuildReportString() const;

	const TArray<FKDFDiagnostic>& GetDiagnostics() const { return mDiagnostics; }

	FKDFVanillaCache& GetVanillaCache() { return mVanillaCache; }

	const TArray<FKDFPatchRecord>& GetPatchRecords() const { return mPatchRecords; }

	bool HasLoadedOnce() const { return bLoadedOnce; }

	/** Class lookup with GC-safe caching (paths per Docs/YamlSchemaSpec.md). */
	UClass* FindOrLoadClassCached(const FString& Path, FString& OutError);

	/** Returns the CDO of a class and retains it (modified CDOs must never be garbage collected). */
	UObject* GetAndRetainCDO(UClass* Class);

	/** Retains an arbitrary patched object (data asset instances). */
	void RetainObject(UObject* Object);

	// --- Runtime actor patches ---

	/** Registers a spawn-time actor patch (see FKDFActorPatch). Replaces earlier patches for the same class+file. */
	void RegisterActorPatch(const FKDFActorPatch& Patch);

	const TArray<FKDFActorPatch>& GetActorPatches() const { return mActorPatches; }

	void ClearActorPatches() { mActorPatches.Reset(); }

	/**
	 * Registers a lazy class watch (see FKDFLazyClassWatch) — replaces any existing watch from the
	 * same source file targeting the same base class. Called by cdo patches with
	 * `applyToSubclasses: true` or `matchTag` + `ofClass`.
	 */
	void RegisterLazyClassWatch(const FKDFLazyClassWatch& Watch);

	// FUObjectArray::FUObjectCreateListener — re-applies lazy class watches to classes that load
	// after their registering document's initial apply pass (KBFL parity).
	virtual void NotifyUObjectCreated(const UObjectBase* Object, int32 Index) override;
	virtual void OnUObjectArrayShutdown() override;

	// --- Phase 2: dynamic content, registration queues, consumer notification ---

	FKDFDynamicContentRegistry& GetDynamicContent() { return mDynamicContent; }

	const TArray<FKDFPack>& GetPacks() const { return mPacks; }

	/** Queues a recipe/schematic/research class for per-world UModContentRegistry registration. */
	void QueueContentRegistration(EKDFContentRegKind Kind, UClass* ContentClass, const FString& SourceFile);

	/** Queues an AWESOME Sink point table for per-world registration. */
	void QueueSinkTableRegistration(UDataTable* Table, uint8 TrackValue, const FString& SourceFile);

	/** Registers all queued content into the world's UModContentRegistry (before the registry freeze). */
	void RegisterQueuedContentForWorld(UWorld* World);

	/** Flags that data assets were created/modified — consumers get a rescan after the load. */
	void MarkDataAssetsChanged() { bDataAssetsChanged = true; }

	/**
	 * Triggers data-asset consumer rescans when assets changed:
	 * KAPI and RefinedRDApi are both optional — resolved and invoked via reflection, no link dependency needed.
	 */
	void NotifyDataAssetConsumers();

	// --- Phase 3: in-game editor bridge (the editor UI module lives above this one) ---

	/** Toggle requests from `/kdf editor` / console; the editor UI subsystem binds here. */
	FKDFOnToggleEditor& OnToggleEditorRequested() { return mOnToggleEditorRequested; }

	void RequestToggleEditor(APlayerController* Requester) { mOnToggleEditorRequested.Broadcast(Requester); }

	/**
	 * Applies one editor-originated op through the standard pipeline: vanilla snapshot before the
	 * first write, op record into the editor session patch record, data-asset change tracking.
	 * @param OutPreValue / OutPostValue  Canonical text before/after — the undo stack stores these.
	 */
	bool ApplyEditorOp(UObject* Target, const FString& PropertyPath, EKDFOp Op, const FKDFOpArgs& Args,
					   FString& OutPreValue, FString& OutPostValue, FString& OutError);

	/** Writes a canonical text value back to a property (undo/redo path). */
	bool RestorePropertyText(UObject* Target, const FString& PropertyPath, const FString& ValueText, FString& OutError);

	/**
	 * Exports Target as a YAML document via the first capable handler (cdo handler for generic
	 * objects). bDiffOnly limits output to properties touched this session (vanilla cache).
	 */
	bool ExportObjectToYaml(UObject* Target, bool bDiffOnly, FString& OutYaml, FString& OutError);

	/** Ops applied through the in-game editor this session (report/export). */
	const FKDFPatchRecord& GetEditorPatchRecord() const { return mEditorPatchRecord; }

private:
	void ScanAndParse();
	void ApplyStages();
	void SortStageDocuments(TArray<int32>& DocumentIndices) const;

	/** Enqueues a newly allocated class for a post-load-safe lazy-watch pass. Game-thread only. */
	void QueueLazyClassForProcessing(UClass* NewClass);

	/** Core ticker callback: waits for class/CDO loading to finish before applying queued watches. */
	bool ProcessPendingLazyClasses(float DeltaTime);

	/** Game-thread only: matches NewClass against mLazyClassWatches and applies any that match. */
	void ApplyLazyWatchesToClass(UClass* NewClass, UObject* CDO);

	/** Handler objects retained against GC; each implements IKDFDataEditorHandler. */
	UPROPERTY()
	TArray<TObjectPtr<UObject>> mHandlerObjects;

	/** Class path → resolved class (GC-safe). */
	UPROPERTY()
	TMap<FString, TObjectPtr<UClass>> mClassCache;

	/** Patched CDOs / data assets, retained against GC. */
	UPROPERTY()
	TSet<TObjectPtr<UObject>> mRetainedObjects;

	UPROPERTY()
	TArray<FKDFContentRegRequest> mContentRegistrations;

	UPROPERTY()
	TArray<FKDFSinkTableRequest> mSinkTableRequests;

	/** Sink registrations already applied per world; avoids duplicate registry rows. */
	TMap<TWeakObjectPtr<UWorld>, TSet<FString>> mAppliedSinkRegistrations;

	TArray<FKDFPack> mPacks;
	TArray<FKDFDocument> mDocuments;
	TArray<FKDFDiagnostic> mDiagnostics;
	TArray<FKDFPatchRecord> mPatchRecords;
	TArray<FKDFActorPatch> mActorPatches;
	TArray<FKDFLazyClassWatch> mLazyClassWatches;

	/** Classes observed by FUObjectArray but not yet safe to inspect or patch. Game-thread only. */
	TSet<TWeakObjectPtr<UClass>> mPendingLazyClasses;
	FTSTicker::FDelegateHandle mLazyClassTickerHandle;

	/** NotifyUObjectCreated can run off-thread; never read mLazyClassWatches there. */
	std::atomic<bool> bHasLazyClassWatches{false};

	FKDFVanillaCache mVanillaCache;
	FKDFDynamicContentRegistry mDynamicContent;
	TSet<FString> mRedirectedPacks;
	FKDFOnToggleEditor mOnToggleEditorRequested;
	FKDFPatchRecord mEditorPatchRecord;
	bool bLoadedOnce = false;
	bool bDataAssetsChanged = false;
};
