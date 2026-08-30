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

DECLARE_MULTICAST_DELEGATE_OneParam(FKDFOnToggleEditor, APlayerController*  );

USTRUCT()
struct FKDFContentRegRequest
{
	GENERATED_BODY()

	UPROPERTY()
	uint8 mKind = 0;

	UPROPERTY()
	TObjectPtr<UClass> mClass;

	UPROPERTY()
	FString mSourceFile;
};

USTRUCT()
struct FKDFSinkTableRequest
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UDataTable> mTable;

	UPROPERTY()
	uint8 mTrack = 0;

	UPROPERTY()
	FString mSourceFile;
};

UCLASS()
class KDATAFORGE_API UKDFSubsystem : public UGameInstanceSubsystem, public FUObjectArray::FUObjectCreateListener
{
	GENERATED_BODY()

public:
	static UKDFSubsystem* Get(const UObject* WorldContextObject);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "KDataForge")
	void RegisterHandler(UObject* HandlerObject);

	void RunInitialLoad();

	UFUNCTION(BlueprintCallable, Category = "KDataForge")
	FString BuildReportString() const;

	const TArray<FKDFDiagnostic>& GetDiagnostics() const { return mDiagnostics; }

	FKDFVanillaCache& GetVanillaCache() { return mVanillaCache; }

	const TArray<FKDFPatchRecord>& GetPatchRecords() const { return mPatchRecords; }

	bool HasLoadedOnce() const { return bLoadedOnce; }

	UClass* FindOrLoadClassCached(const FString& Path, FString& OutError);

	UObject* GetAndRetainCDO(UClass* Class);

	void RetainObject(UObject* Object);

	void RegisterActorPatch(const FKDFActorPatch& Patch);

	const TArray<FKDFActorPatch>& GetActorPatches() const { return mActorPatches; }

	void ClearActorPatches() { mActorPatches.Reset(); }

	void RegisterNodePurge(const FKDFNodePurge& Purge);

	const TArray<FKDFNodePurge>& GetNodePurges() const { return mNodePurges; }

	void ClearNodePurges() { mNodePurges.Reset(); }

	void RegisterContentRemoval(const FKDFContentRemoval& Removal);

	const TArray<FKDFContentRemoval>& GetContentRemovals() const { return mContentRemovals; }

	void ClearContentRemovals() { mContentRemovals.Reset(); }

	void RegisterLazyClassWatch(const FKDFLazyClassWatch& Watch);

	virtual void NotifyUObjectCreated(const UObjectBase* Object, int32 Index) override;
	virtual void OnUObjectArrayShutdown() override;

	FKDFDynamicContentRegistry& GetDynamicContent() { return mDynamicContent; }

	const TArray<FKDFPack>& GetPacks() const { return mPacks; }

	void QueueContentRegistration(EKDFContentRegKind Kind, UClass* ContentClass, const FString& SourceFile);

	void QueueSinkTableRegistration(UDataTable* Table, uint8 TrackValue, const FString& SourceFile);

	void RegisterQueuedContentForWorld(UWorld* World);

	void MarkDataAssetsChanged() { bDataAssetsChanged = true; }

	void NotifyDataAssetConsumers();

	FKDFOnToggleEditor& OnToggleEditorRequested() { return mOnToggleEditorRequested; }

	void RequestToggleEditor(APlayerController* Requester) { mOnToggleEditorRequested.Broadcast(Requester); }

	bool ApplyEditorOp(UObject* Target, const FString& PropertyPath, EKDFOp Op, const FKDFOpArgs& Args,
					   FString& OutPreValue, FString& OutPostValue, FString& OutError);

	bool RestorePropertyText(UObject* Target, const FString& PropertyPath, const FString& ValueText, FString& OutError);

	bool ExportObjectToYaml(UObject* Target, bool bDiffOnly, FString& OutYaml, FString& OutError);

	const FKDFPatchRecord& GetEditorPatchRecord() const { return mEditorPatchRecord; }

private:
	void ScanAndParse();
	void ApplyStages();
	void SortStageDocuments(TArray<int32>& DocumentIndices) const;

	void QueueLazyClassForProcessing(UClass* NewClass);

	bool ProcessPendingLazyClasses(float DeltaTime);

	void ApplyLazyWatchesToClass(UClass* NewClass, UObject* CDO);

	UPROPERTY()
	TArray<TObjectPtr<UObject>> mHandlerObjects;

	UPROPERTY()
	TMap<FString, TObjectPtr<UClass>> mClassCache;

	UPROPERTY()
	TSet<TObjectPtr<UObject>> mRetainedObjects;

	UPROPERTY()
	TArray<FKDFContentRegRequest> mContentRegistrations;

	UPROPERTY()
	TArray<FKDFSinkTableRequest> mSinkTableRequests;

	TMap<TWeakObjectPtr<UWorld>, TSet<FString>> mAppliedSinkRegistrations;

	TArray<FKDFPack> mPacks;
	TArray<FKDFDocument> mDocuments;
	TArray<FKDFDiagnostic> mDiagnostics;
	TArray<FKDFPatchRecord> mPatchRecords;
	TArray<FKDFActorPatch> mActorPatches;
	TArray<FKDFNodePurge> mNodePurges;
	TArray<FKDFContentRemoval> mContentRemovals;
	TArray<FKDFLazyClassWatch> mLazyClassWatches;

	TSet<TWeakObjectPtr<UClass>> mPendingLazyClasses;
	FTSTicker::FDelegateHandle mLazyClassTickerHandle;

	std::atomic<bool> bHasLazyClassWatches{false};

	FKDFVanillaCache mVanillaCache;
	FKDFDynamicContentRegistry mDynamicContent;
	TSet<FString> mRedirectedPacks;
	FKDFOnToggleEditor mOnToggleEditorRequested;
	FKDFPatchRecord mEditorPatchRecord;
	bool bLoadedOnce = false;
	bool bDataAssetsChanged = false;
};
