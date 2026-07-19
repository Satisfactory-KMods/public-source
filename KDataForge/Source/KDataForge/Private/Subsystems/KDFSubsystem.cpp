#include "Subsystems/KDFSubsystem.h"

#include "Async/Async.h"
#include "Async/ParallelFor.h"
#include "Containers/Ticker.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "FGResourceSinkSubsystem.h"
#include "Handlers/KDFAssetHandler.h"
#include "Handlers/KDFCdoHandler.h"
#include "Handlers/KDFContentClassHandler.h"
#include "Handlers/KDFCurveHandler.h"
#include "Handlers/KDFDataAssetHandler.h"
#include "Handlers/KDFGameTagHandler.h"
#include "Handlers/KDFLocalizationHandler.h"
#include "Handlers/KDFSinkPointsHandler.h"
#include "KDFLogging.h"
#include "KDFYamlParser.h"
#include "Loader/KDFConditionEvaluator.h"
#include "Loader/KDFDocumentType.h"
#include "Loader/KDFPackScanner.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Reflection/KDFPatchUtil.h"
#include "Reflection/KDFPropertyPath.h"
#include "Reflection/KDFValueCodec.h"
#include "Registry/ModContentRegistry.h"

namespace
{
	/**
	 * @param bAllowFilenameFallback The `<name>.<type>.yml` filename convention only makes sense for
	 *        single-document files — a multi-document file's name can't encode more than one type,
	 *        so every document in a `---`-separated stream must declare its own `type:` key.
	 */
	FName DetectRootType(const FKDFNode& Root, const FString& RelativeFile, bool bAllowFilenameFallback)
	{
		if (const FKDFNode* TypeNode = Root.Find(TEXT("type")))
		{
			const FName RootType = KDFDocumentType::Normalize(TypeNode->GetString());
			if (!RootType.IsNone())
			{
				return RootType;
			}
		}
		if (!bAllowFilenameFallback)
		{
			return NAME_None;
		}
		// Filename convention fallback: `anything.<type>.yml`.
		TArray<FString> Tokens;
		FPaths::GetCleanFilename(RelativeFile).ParseIntoArray(Tokens, TEXT("."));
		if (Tokens.Num() >= 3)
		{
			return KDFDocumentType::Normalize(Tokens[Tokens.Num() - 2]);
		}
		return NAME_None;
	}

	/**
	 * Matches an `include:` entry against a document's (possibly multi-doc-suffixed) relative file.
	 * A bare `include: file.yml` also matches every document split out of a multi-document
	 * `file.yml` (suffixed `file.yml[0]`, `file.yml[1]`, ...), so referencing the whole file waits
	 * for all of its documents, not just whichever one happens to sort first.
	 */
	bool DocumentMatchesInclude(const FString& RelativeFile, const FString& Include)
	{
		return RelativeFile == Include || RelativeFile.StartsWith(Include + TEXT("["));
	}

	bool IsReadyForLazyPatch(const UObject* Object)
	{
		constexpr EObjectFlags PendingLoadFlags =
			RF_NeedInitialization | RF_NeedLoad | RF_NeedPostLoad | RF_NeedPostLoadSubobjects;
		constexpr EInternalObjectFlags PendingInternalFlags =
			EInternalObjectFlags::Async | EInternalObjectFlags::AsyncLoadingPhase1 |
			EInternalObjectFlags::AsyncLoadingPhase2 | EInternalObjectFlags::PendingConstruction;
		return IsValid(Object) && !Object->HasAnyFlags(PendingLoadFlags) &&
			!Object->HasAnyInternalFlags(PendingInternalFlags);
	}
} // namespace

UKDFSubsystem* UKDFSubsystem::Get(const UObject* WorldContextObject)
{
	if (!IsValid(WorldContextObject))
	{
		return nullptr;
	}
	if (const UGameInstance* GameInstance = Cast<UGameInstance>(WorldContextObject))
	{
		return GameInstance->GetSubsystem<UKDFSubsystem>();
	}
	const UWorld* World = WorldContextObject->GetWorld();
	if (World == nullptr || World->GetGameInstance() == nullptr)
	{
		return nullptr;
	}
	return World->GetGameInstance()->GetSubsystem<UKDFSubsystem>();
}

void UKDFSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	mDynamicContent.Initialize(this);
	FKDFValueCodec::SetDynamicContentRegistry(&mDynamicContent);

	RegisterHandler(NewObject<UKDFCdoHandler>(this));
	RegisterHandler(NewObject<UKDFGameTagHandler>(this));
	RegisterHandler(NewObject<UKDFAssetHandler>(this));
	RegisterHandler(NewObject<UKDFCurveHandler>(this));
	RegisterHandler(NewObject<UKDFDataAssetHandler>(this));
	RegisterHandler(NewObject<UKDFLocalizationHandler>(this));
	RegisterHandler(NewObject<UKDFClassHandler>(this));
	RegisterHandler(NewObject<UKDFItemHandler>(this));
	RegisterHandler(NewObject<UKDFResourceHandler>(this));
	RegisterHandler(NewObject<UKDFBuildingHandler>(this));
	RegisterHandler(NewObject<UKDFUnlockHandler>(this));
	RegisterHandler(NewObject<UKDFRecipeHandler>(this));
	RegisterHandler(NewObject<UKDFSchematicHandler>(this));
	RegisterHandler(NewObject<UKDFResearchHandler>(this));
	RegisterHandler(NewObject<UKDFSinkPointsHandler>(this));

	// Lazy-load CDO re-apply (KBFL parity): fires on every UObject construction engine-wide, so
	// NotifyUObjectCreated must stay cheap (atomic no-watch early-out) when unused.
	GUObjectArray.AddUObjectCreateListener(this);
}

void UKDFSubsystem::Deinitialize()
{
	bHasLazyClassWatches.store(false, std::memory_order_release);
	GUObjectArray.RemoveUObjectCreateListener(this);
	if (mLazyClassTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(mLazyClassTickerHandle);
		mLazyClassTickerHandle.Reset();
	}
	FKDFValueCodec::SetDynamicContentRegistry(nullptr); // mDynamicContent is about to go away with us
	mActorPatches.Reset();
	mLazyClassWatches.Reset();
	mPendingLazyClasses.Reset();
	mHandlerObjects.Reset();
	mRetainedObjects.Reset();
	mClassCache.Reset();
	mVanillaCache.Reset();
	Super::Deinitialize();
}

void UKDFSubsystem::RegisterHandler(UObject* HandlerObject)
{
	if (!IsValid(HandlerObject) || Cast<IKDFDataEditorHandler>(HandlerObject) == nullptr)
	{
		UE_LOG(LogKDataForge, Warning, TEXT("RegisterHandler: %s does not implement IKDFDataEditorHandler"),
			   *GetNameSafe(HandlerObject));
		return;
	}
	if (bLoadedOnce)
	{
		UE_LOG(LogKDataForge, Warning,
			   TEXT("RegisterHandler(%s): registered after the initial load — its documents apply next session"),
			   *HandlerObject->GetName());
	}
	mHandlerObjects.AddUnique(HandlerObject);
}

void UKDFSubsystem::RunInitialLoad()
{
	if (bLoadedOnce)
	{
		return;
	}
	const double StartTime = FPlatformTime::Seconds();
	ScanAndParse();
	ApplyStages();

	// Persistence: recreate manifest-recorded classes the current YAML no longer defines, so any
	// save referencing them keeps loading (tombstones).
	TArray<FString> TombstoneKeys;
	const int32 TombstoneCount = mDynamicContent.ReconstructTombstones(TombstoneKeys);
	if (TombstoneCount > 0)
	{
		FKDFDiagnostic& Diagnostic = mDiagnostics.AddDefaulted_GetRef();
		Diagnostic.mSeverity = EKDFSeverity::Warning;
		Diagnostic.mMessage =
			FString::Printf(TEXT("%d generated class(es) have no YAML definition anymore and were recreated as "
								 "tombstones: %s"),
							TombstoneCount, *FString::Join(TombstoneKeys, TEXT(", ")));
	}

	NotifyDataAssetConsumers();
	bLoadedOnce = true;
	UE_LOG(LogKDataForge, Log, TEXT("Initial load finished in %.2f ms — %s"),
		   (FPlatformTime::Seconds() - StartTime) * 1000.0, *BuildReportString());
}

void UKDFSubsystem::ScanAndParse()
{
	mPacks.Reset();
	mDocuments.Reset();

	TMap<FString, TArray<FString>> FilesByPack;
	FKDFPackScanner::ScanPacks(mPacks, FilesByPack, mDiagnostics);

	// pack.yml `conditions:` gates the whole pack (logical AND, same grammar as a document's
	// `conditions:`) — e.g. `conditions: { hasMod: [KAPI, KLib] }` only applies this pack's
	// documents when every listed mod is loaded. A failing pack is dropped entirely, before any of
	// its files are read.
	UGameInstance* ConditionsGameInstance = GetGameInstance();
	mPacks.RemoveAll(
		[this, ConditionsGameInstance, &FilesByPack](const FKDFPack& Pack)
		{
			FString FailReason;
			if (FKDFConditionEvaluator::Evaluate(Pack.mConditionsNode.Get(), ConditionsGameInstance, FailReason))
			{
				return false;
			}
			FKDFDiagnostic& Diagnostic = mDiagnostics.AddDefaulted_GetRef();
			Diagnostic.mSeverity = EKDFSeverity::Info;
			Diagnostic.mFile = Pack.mDirectory / TEXT("pack.yml");
			Diagnostic.mMessage = FString::Printf(TEXT("Pack '%s' skipped by conditions: %s"), *Pack.mRef, *FailReason);
			FilesByPack.Remove(Pack.mRef);
			return true;
		});

	// A pack removed by `conditions:` also invalidates every dependant. ScanPacks validated and
	// topologically sorted the original graph; condition filtering is the only operation that can
	// make a previously valid dependency disappear here.
	TSet<FString> ActivePackRefs;
	for (const FKDFPack& Pack : mPacks)
	{
		ActivePackRefs.Add(Pack.mRef);
	}
	TSet<FString> InvalidPackRefs;
	bool bInvalidatedAny = true;
	while (bInvalidatedAny)
	{
		bInvalidatedAny = false;
		for (const FKDFPack& Pack : mPacks)
		{
			if (InvalidPackRefs.Contains(Pack.mRef))
			{
				continue;
			}
			for (const FString& Dependency : Pack.mDependencies)
			{
				if (!ActivePackRefs.Contains(Dependency) || InvalidPackRefs.Contains(Dependency))
				{
					FKDFDiagnostic& Diagnostic = mDiagnostics.AddDefaulted_GetRef();
					Diagnostic.mSeverity = EKDFSeverity::Warning;
					Diagnostic.mFile = Pack.mDirectory / TEXT("pack.yml");
					Diagnostic.mMessage = FString::Printf(
						TEXT("Pack '%s' skipped: dependency '%s' was skipped by conditions"), *Pack.mRef, *Dependency);
					InvalidPackRefs.Add(Pack.mRef);
					bInvalidatedAny = true;
					break;
				}
			}
		}
	}
	for (const FString& InvalidPackRef : InvalidPackRefs)
	{
		FilesByPack.Remove(InvalidPackRef);
	}
	mPacks.RemoveAll([&InvalidPackRefs](const FKDFPack& Pack) { return InvalidPackRefs.Contains(Pack.mRef); });

	struct FPendingFile
	{
		FString mAbsoluteFile;
		FString mRelativeFile;
		FString mPackRef;
		int32 mPackPriority = 100;
		int32 mPackOrder = INDEX_NONE;
		bool bPackDebug = false;
	};
	TArray<FPendingFile> PendingFiles;
	for (int32 PackOrder = 0; PackOrder < mPacks.Num(); ++PackOrder)
	{
		const FKDFPack& Pack = mPacks[PackOrder];
		const TArray<FString>* Files = FilesByPack.Find(Pack.mRef);
		if (Files == nullptr)
		{
			continue;
		}
		for (const FString& File : *Files)
		{
			FPendingFile& Pending = PendingFiles.AddDefaulted_GetRef();
			Pending.mAbsoluteFile = File;
			Pending.mRelativeFile = File;
			FPaths::MakePathRelativeTo(Pending.mRelativeFile, *(Pack.mDirectory + TEXT("/")));
			Pending.mPackRef = Pack.mRef;
			Pending.mPackPriority = Pack.mPriority;
			Pending.mPackOrder = PackOrder;
			Pending.bPackDebug = Pack.bDebug;
		}
	}

	struct FParseResult
	{
		/** One entry per `---`-separated document; a plain single-document file still has exactly one. */
		TArray<TSharedPtr<FKDFNode>> mRoots;
		FString mError;
	};
	TArray<FParseResult> Results;
	Results.SetNum(PendingFiles.Num());

	// File IO + YAML parsing is pure CPU/disk work — parallelize. All UObject work stays on the game thread.
	ParallelFor(PendingFiles.Num(),
				[&PendingFiles, &Results](int32 Index)
				{
					FString FileText;
					if (!FFileHelper::LoadFileToString(FileText, *PendingFiles[Index].mAbsoluteFile))
					{
						Results[Index].mError = TEXT("Could not read file");
						return;
					}
					Results[Index].mRoots = FKDFYamlParser::ParseStringMulti(FileText, Results[Index].mError);
				});

	for (int32 Index = 0; Index < PendingFiles.Num(); ++Index)
	{
		const FPendingFile& Pending = PendingFiles[Index];
		FParseResult& Result = Results[Index];
		if (Result.mRoots.IsEmpty())
		{
			FKDFDiagnostic& Diagnostic = mDiagnostics.AddDefaulted_GetRef();
			Diagnostic.mSeverity = EKDFSeverity::Error;
			Diagnostic.mFile = Pending.mRelativeFile;
			Diagnostic.mMessage = FString::Printf(TEXT("YAML parse failed: %s"), *Result.mError);
			continue;
		}

		// Multiple `---`-separated documents share one file (Kubernetes-manifest style): each becomes
		// its own FKDFDocument tagged "<file>[<index>]" so diagnostics, `include:`, and actor-patch
		// identity all stay unambiguous. A plain single-document file keeps its bare path unchanged —
		// fully backward compatible with every existing pack.
		const bool bMultiDoc = Result.mRoots.Num() > 1;
		for (int32 DocIndex = 0; DocIndex < Result.mRoots.Num(); ++DocIndex)
		{
			const TSharedPtr<FKDFNode>& DocRoot = Result.mRoots[DocIndex];
			if (DocRoot->IsNull())
			{
				continue; // empty document between separators (e.g. a stray trailing `---`) — silently skipped
			}

			const FString DocSuffix = bMultiDoc ? FString::Printf(TEXT("[%d]"), DocIndex) : FString();
			const FString DocAbsoluteFile = Pending.mAbsoluteFile + DocSuffix;
			const FString DocRelativeFile = Pending.mRelativeFile + DocSuffix;

			if (!DocRoot->IsMap())
			{
				FKDFDiagnostic& Diagnostic = mDiagnostics.AddDefaulted_GetRef();
				Diagnostic.mSeverity = EKDFSeverity::Warning;
				Diagnostic.mFile = DocRelativeFile;
				Diagnostic.mMessage = TEXT("Document root must be a map — document skipped");
				continue;
			}

			FKDFDocument& Document = mDocuments.AddDefaulted_GetRef();
			Document.mAbsoluteFile = DocAbsoluteFile;
			Document.mRelativeFile = DocRelativeFile;
			Document.mPackRef = Pending.mPackRef;
			Document.mPackPriority = Pending.mPackPriority;
			Document.mPackOrder = Pending.mPackOrder;
			Document.bDebug = Pending.bPackDebug || DocRoot->GetBool(TEXT("debug"), false);
			Document.mRoot = DocRoot;
			Document.mRootType = DetectRootType(*DocRoot, Pending.mRelativeFile, /*bAllowFilenameFallback=*/!bMultiDoc);
			if (const FKDFNode* IncludeNode = DocRoot->Find(TEXT("include")))
			{
				if (IncludeNode->IsSequence())
				{
					for (const TSharedRef<FKDFNode>& Include : IncludeNode->Sequence)
					{
						Document.mIncludeDependencies.Add(Include->GetString());
					}
				}
				else if (IncludeNode->IsScalar())
				{
					Document.mIncludeDependencies.Add(IncludeNode->Scalar);
				}
			}

			if (Document.mRootType.IsNone())
			{
				FKDFDiagnostic& Diagnostic = mDiagnostics.AddDefaulted_GetRef();
				Diagnostic.mSeverity = EKDFSeverity::Warning;
				Diagnostic.mFile = DocRelativeFile;
				Diagnostic.mMessage = bMultiDoc
					? TEXT("No 'type:' key — the '<name>.<type>.yml' filename convention only applies to "
						   "single-document files, so every document in a multi-document file needs its "
						   "own 'type:' key — document skipped")
					: TEXT("No 'type:' key and no '<name>.<type>.yml' filename convention — document skipped");
				mDocuments.Pop();
			}
		}
	}
}

void UKDFSubsystem::SortStageDocuments(TArray<int32>& DocumentIndices) const
{
	// Deterministic base order: the pack scanner's topological order (dependencies first, then
	// priority desc/ref asc among ready packs), followed by relative path.
	DocumentIndices.StableSort(
		[this](int32 IndexA, int32 IndexB)
		{
			const FKDFDocument& A = mDocuments[IndexA];
			const FKDFDocument& B = mDocuments[IndexB];
			if (A.mPackOrder != B.mPackOrder)
			{
				return A.mPackOrder < B.mPackOrder;
			}
			if (A.mPackRef != B.mPackRef)
			{
				return A.mPackRef < B.mPackRef;
			}
			return A.mRelativeFile < B.mRelativeFile;
		});

	// Include-dependency fix-up: move dependents after their includes (bounded passes; cycles keep base order).
	for (int32 Pass = 0; Pass < DocumentIndices.Num(); ++Pass)
	{
		bool bChanged = false;
		for (int32 Position = 0; Position < DocumentIndices.Num(); ++Position)
		{
			const FKDFDocument& Document = mDocuments[DocumentIndices[Position]];
			for (const FString& Include : Document.mIncludeDependencies)
			{
				for (int32 Later = Position + 1; Later < DocumentIndices.Num(); ++Later)
				{
					const FKDFDocument& Candidate = mDocuments[DocumentIndices[Later]];
					if (Candidate.mPackRef == Document.mPackRef &&
						DocumentMatchesInclude(Candidate.mRelativeFile, Include))
					{
						const int32 Moved = DocumentIndices[Position];
						DocumentIndices.RemoveAt(Position);
						DocumentIndices.Insert(Moved, Later); // after the include target's new position
						bChanged = true;
						break;
					}
				}
			}
		}
		if (!bChanged)
		{
			break;
		}
	}
}

void UKDFSubsystem::ApplyStages()
{
	UGameInstance* GameInstance = GetGameInstance();

	// Save-compat id redirects must exist before any generated class is referenced (once per pack).
	for (const FKDFPack& Pack : mPacks)
	{
		if (!Pack.mRedirects.IsEmpty() && !mRedirectedPacks.Contains(Pack.mRef))
		{
			mRedirectedPacks.Add(Pack.mRef);
			mDynamicContent.ApplyRedirects(Pack.mRef, Pack.mRedirects);
		}
	}

	// Handler lookup: root type → handlers (priority desc).
	TMultiMap<FName, IKDFDataEditorHandler*> HandlersByType;
	TArray<IKDFDataEditorHandler*> AllHandlers;
	for (const TObjectPtr<UObject>& HandlerObject : mHandlerObjects)
	{
		if (IKDFDataEditorHandler* Handler = Cast<IKDFDataEditorHandler>(HandlerObject.Get()))
		{
			HandlersByType.Add(FName(*Handler->GetRootType().ToString().ToLower()), Handler);
			AllHandlers.Add(Handler);
		}
	}

	// Pre-evaluate conditions per document (game thread — may load classes).
	TArray<bool> ConditionsPassed;
	ConditionsPassed.SetNum(mDocuments.Num());
	for (int32 Index = 0; Index < mDocuments.Num(); ++Index)
	{
		FString FailReason;
		ConditionsPassed[Index] = FKDFConditionEvaluator::Evaluate(mDocuments[Index].mRoot->Find(TEXT("conditions")),
																   GameInstance, FailReason);
		if (!ConditionsPassed[Index])
		{
			FKDFDiagnostic& Diagnostic = mDiagnostics.AddDefaulted_GetRef();
			Diagnostic.mSeverity = EKDFSeverity::Info;
			Diagnostic.mFile = mDocuments[Index].mRelativeFile;
			Diagnostic.mMessage = FString::Printf(TEXT("Skipped by conditions: %s"), *FailReason);
		}
	}

	TSet<FName> UnhandledTypes;
	const UEnum* StageEnum = StaticEnum<EKDFStage>();
	for (int64 StageValue = 0; StageValue <= static_cast<int64>(EKDFStage::Validation); ++StageValue)
	{
		const EKDFStage Stage = static_cast<EKDFStage>(StageValue);

		// Handlers of this stage, deterministic order: priority desc, root type asc.
		TArray<IKDFDataEditorHandler*> StageHandlers = AllHandlers.FilterByPredicate(
			[Stage](const IKDFDataEditorHandler* Handler) { return Handler->GetStage() == Stage; });
		StageHandlers.StableSort(
			[](const IKDFDataEditorHandler& A, const IKDFDataEditorHandler& B)
			{
				if (A.GetPriority() != B.GetPriority())
				{
					return A.GetPriority() > B.GetPriority();
				}
				return A.GetRootType().LexicalLess(B.GetRootType());
			});

		for (IKDFDataEditorHandler* Handler : StageHandlers)
		{
			const FName HandlerType = FName(*Handler->GetRootType().ToString().ToLower());
			TArray<int32> StageDocuments;
			for (int32 Index = 0; Index < mDocuments.Num(); ++Index)
			{
				if (mDocuments[Index].mRootType == HandlerType && ConditionsPassed[Index])
				{
					StageDocuments.Add(Index);
				}
			}
			SortStageDocuments(StageDocuments);

			for (const int32 DocumentIndex : StageDocuments)
			{
				const FKDFDocument& Document = mDocuments[DocumentIndex];

				FKDFValidationContext ValidationContext;
				ValidationContext.mGameInstance = GameInstance;
				ValidationContext.mSourceFile = Document.mRelativeFile;
				ValidationContext.mPackRef = Document.mPackRef;
				ValidationContext.mDiagnostics = &mDiagnostics;
				if (!Handler->ValidateDocument(*Document.mRoot, ValidationContext))
				{
					continue; // handler reported diagnostics; never fatal
				}

				FKDFPatchRecord PatchRecord;
				PatchRecord.mSourceFile = Document.mRelativeFile;
				PatchRecord.mRootType = Document.mRootType;
				PatchRecord.mPackRef = Document.mPackRef;

				FKDFApplyContext ApplyContext;
				ApplyContext.mGameInstance = GameInstance;
				ApplyContext.mSourceFile = Document.mRelativeFile;
				ApplyContext.mPackRef = Document.mPackRef;
				ApplyContext.bDebug = Document.bDebug;
				ApplyContext.mDiagnostics = &mDiagnostics;
				ApplyContext.mPatchRecord = &PatchRecord;

				// Bare generated-content ids (e.g. "MySuperIngot" instead of the full
				// /KDataForge/Gen/... path) resolve against this document's own pack first.
				const FKDFValueCodec::FPackScope PackScope(Document.mPackRef);
				if (Handler->ApplyDocument(*Document.mRoot, ApplyContext))
				{
					mPatchRecords.Add(MoveTemp(PatchRecord));
				}
			}
		}
	}

	// Warn once per root type that nobody handles.
	for (int32 Index = 0; Index < mDocuments.Num(); ++Index)
	{
		const FName Type = mDocuments[Index].mRootType;
		if (!HandlersByType.Contains(Type) && !UnhandledTypes.Contains(Type))
		{
			UnhandledTypes.Add(Type);
			FKDFDiagnostic& Diagnostic = mDiagnostics.AddDefaulted_GetRef();
			Diagnostic.mSeverity = EKDFSeverity::Warning;
			Diagnostic.mFile = mDocuments[Index].mRelativeFile;
			Diagnostic.mMessage = FString::Printf(
				TEXT("No handler registered for document type '%s' (planned for a later phase?)"), *Type.ToString());
		}
	}
}

FString UKDFSubsystem::BuildReportString() const
{
	int32 ErrorCount = 0;
	int32 WarningCount = 0;
	for (const FKDFDiagnostic& Diagnostic : mDiagnostics)
	{
		ErrorCount += Diagnostic.mSeverity == EKDFSeverity::Error ? 1 : 0;
		WarningCount += Diagnostic.mSeverity == EKDFSeverity::Warning ? 1 : 0;
	}
	int32 OpCount = 0;
	for (const FKDFPatchRecord& Record : mPatchRecords)
	{
		OpCount += Record.mOps.Num();
	}

	FString Report = FString::Printf(
		TEXT(
			"KDataForge: %d pack(s), %d document(s) applied, %d op(s), %d actor patch(es), %d error(s), %d warning(s)"),
		mPacks.Num(), mPatchRecords.Num(), OpCount, mActorPatches.Num(), ErrorCount, WarningCount);
	constexpr int32 MaxReportedDiagnostics = 100;
	int32 Reported = 0;
	for (const FKDFDiagnostic& Diagnostic : mDiagnostics)
	{
		if (Diagnostic.mSeverity == EKDFSeverity::Info)
		{
			continue;
		}
		if (++Reported > MaxReportedDiagnostics)
		{
			Report += FString::Printf(TEXT("\n… and %d more"), mDiagnostics.Num() - Reported + 1);
			break;
		}
		Report += TEXT("\n") + Diagnostic.ToString();
	}
	return Report;
}

UClass* UKDFSubsystem::FindOrLoadClassCached(const FString& Path, FString& OutError)
{
	// Bare-name paths (generated-content ids, the class-name shortcut) resolve differently depending
	// on the active FKDFValueCodec::FPackScope, so they must be cached per scope. Full paths never
	// depend on the scope, so they keep the plain, scope-independent key for maximum cache reuse.
	const bool bNameOnly = !Path.Contains(TEXT("/")) && !Path.Contains(TEXT("."));
	const FString CacheKey = bNameOnly ? FKDFValueCodec::GetCurrentPackScope() + TEXT("|") + Path : Path;
	if (const TObjectPtr<UClass>* Cached = mClassCache.Find(CacheKey))
	{
		return Cached->Get();
	}
	UClass* Class = FKDFValueCodec::ResolveClass(Path, OutError);
	if (Class != nullptr)
	{
		mClassCache.Add(CacheKey, Class);
	}
	return Class;
}

UObject* UKDFSubsystem::GetAndRetainCDO(UClass* Class)
{
	if (!IsValid(Class))
	{
		return nullptr;
	}
	UObject* DefaultObject = Class->GetDefaultObject();
	RetainObject(DefaultObject);
	return DefaultObject;
}

void UKDFSubsystem::RetainObject(UObject* Object)
{
	if (IsValid(Object))
	{
		mRetainedObjects.Add(Object);
	}
}

void UKDFSubsystem::QueueContentRegistration(EKDFContentRegKind Kind, UClass* ContentClass, const FString& SourceFile)
{
	if (!IsValid(ContentClass) || Kind == EKDFContentRegKind::None)
	{
		return;
	}
	for (const FKDFContentRegRequest& Existing : mContentRegistrations)
	{
		if (Existing.mClass == ContentClass)
		{
			return; // reloads re-run handlers — never queue a class twice
		}
	}
	FKDFContentRegRequest& Request = mContentRegistrations.AddDefaulted_GetRef();
	Request.mKind = static_cast<uint8>(Kind);
	Request.mClass = ContentClass;
	Request.mSourceFile = SourceFile;
}

void UKDFSubsystem::QueueSinkTableRegistration(UDataTable* Table, uint8 TrackValue, const FString& SourceFile)
{
	if (!IsValid(Table))
	{
		return;
	}
	for (FKDFSinkTableRequest& Existing : mSinkTableRequests)
	{
		if (Existing.mTable == Table && Existing.mTrack == TrackValue)
		{
			Existing.mSourceFile = SourceFile;
			return;
		}
	}
	FKDFSinkTableRequest& Request = mSinkTableRequests.AddDefaulted_GetRef();
	Request.mTable = Table;
	Request.mTrack = TrackValue;
	Request.mSourceFile = SourceFile;
}

void UKDFSubsystem::RegisterQueuedContentForWorld(UWorld* World)
{
	if (World == nullptr || (mContentRegistrations.IsEmpty() && mSinkTableRequests.IsEmpty()))
	{
		return;
	}
	UModContentRegistry* ContentRegistry = UModContentRegistry::Get(World);
	if (ContentRegistry == nullptr)
	{
		UE_LOG(LogKDataForge, Warning, TEXT("Mod content registry unavailable — queued content not registered"));
		return;
	}
	const FName ModReference(TEXT("KDataForge"));
	mAppliedSinkRegistrations.Remove(nullptr);
	TSet<FString>& AppliedSinkKeys = mAppliedSinkRegistrations.FindOrAdd(World);

	int32 RegisteredCount = 0;
	for (const FKDFContentRegRequest& Request : mContentRegistrations)
	{
		UClass* ContentClass = Request.mClass.Get();
		if (!IsValid(ContentClass))
		{
			continue;
		}
		switch (static_cast<EKDFContentRegKind>(Request.mKind))
		{
		case EKDFContentRegKind::Recipe:
			ContentRegistry->RegisterRecipe(ModReference, ContentClass);
			break;
		case EKDFContentRegKind::Schematic:
			ContentRegistry->RegisterSchematic(ModReference, ContentClass);
			break;
		case EKDFContentRegKind::ResearchTree:
			ContentRegistry->RegisterResearchTree(ModReference, ContentClass);
			break;
		default:
			continue;
		}
		++RegisteredCount;
	}
	for (const FKDFSinkTableRequest& Request : mSinkTableRequests)
	{
		if (IsValid(Request.mTable))
		{
			const FString RegistrationKey =
				FString::Printf(TEXT("%s|%u"), *Request.mTable->GetPathName(), Request.mTrack);
			if (AppliedSinkKeys.Contains(RegistrationKey))
			{
				continue;
			}
			ContentRegistry->RegisterResourceSinkItemPointTable(ModReference, Request.mTable,
																static_cast<EResourceSinkTrack>(Request.mTrack));
			AppliedSinkKeys.Add(RegistrationKey);
			++RegisteredCount;
		}
	}
	UE_LOG(LogKDataForge, Log, TEXT("Registered %d queued content entr%s into the world's content registry"),
		   RegisteredCount, RegisteredCount == 1 ? TEXT("y") : TEXT("ies"));
}

void UKDFSubsystem::NotifyDataAssetConsumers()
{
	if (!bDataAssetsChanged)
	{
		return;
	}
	bDataAssetsChanged = false;
	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance == nullptr)
	{
		return;
	}

	// KAPI is optional — resolve and invoke via reflection so no link dependency exists.
	if (UClass* KAPIClass = FindObject<UClass>(nullptr, TEXT("/Script/KAPI.KAPIDataAssetSubsystem")))
	{
		if (UGameInstanceSubsystem* KAPISubsystem = GameInstance->GetSubsystemBase(KAPIClass))
		{
			if (UFunction* ScanFunction = KAPISubsystem->FindFunction(FName(TEXT("StartScanForDataAssets"))))
			{
				KAPISubsystem->ProcessEvent(ScanFunction, nullptr);
				UE_LOG(LogKDataForge, Log, TEXT("KAPI data asset subsystem rescanned"));
			}
		}
	}

	// RefinedRDApi is optional — resolve and invoke via reflection so no link dependency exists.
	if (UClass* RRDAClass = FindObject<UClass>(nullptr, TEXT("/Script/RefinedRDApi.RRDADataAssetSubsystem")))
	{
		if (UGameInstanceSubsystem* RRDASubsystem = GameInstance->GetSubsystemBase(RRDAClass))
		{
			if (UFunction* ScanFunction = RRDASubsystem->FindFunction(FName(TEXT("StartScanForDataAssets"))))
			{
				RRDASubsystem->ProcessEvent(ScanFunction, nullptr);
				UE_LOG(LogKDataForge, Log, TEXT("RefinedRDApi data asset subsystem rescanned"));
			}
		}
	}
}

bool UKDFSubsystem::ApplyEditorOp(UObject* Target, const FString& PropertyPath, EKDFOp Op, const FKDFOpArgs& Args,
								  FString& OutPreValue, FString& OutPostValue, FString& OutError)
{
	if (!IsValid(Target))
	{
		OutError = TEXT("Invalid target");
		return false;
	}
	FKDFPropertyPath Path;
	if (!FKDFPropertyPath::Parse(PropertyPath, Path, OutError))
	{
		return false;
	}

	FKDFResolvedProperty PreResolved;
	if (!FKDFPropertyResolver::Resolve(Target, Path, PreResolved, OutError))
	{
		return false;
	}
	OutPreValue = FKDFValueCodec::ExportText(PreResolved.mProperty, PreResolved.mValuePtr);
	mVanillaCache.RecordSnapshot(Target, Path.ToString(), OutPreValue);

	if (!FKDFOpEngine::ApplyOp(Target, Path, Op, Args, OutError))
	{
		return false;
	}

	FKDFResolvedProperty PostResolved;
	if (FKDFPropertyResolver::Resolve(Target, Path, PostResolved, OutError))
	{
		OutPostValue = FKDFValueCodec::ExportText(PostResolved.mProperty, PostResolved.mValuePtr);
	}
	FKDFOpRecord& OpRecord = mEditorPatchRecord.mOps.AddDefaulted_GetRef();
	OpRecord.mTargetObjectPath = Target->GetPathName();
	OpRecord.mPropertyPath = Path.ToString();
	OpRecord.mOp = Op;
	OpRecord.mValueText = OutPostValue;

	FKDFPatchUtil::PostWriteFixups(Target, Path.ToString());
	if (FKDFPatchUtil::IsDebugEnabled(false))
	{
		FKDFPatchUtil::LogAppliedOp(FString(), Target, Path.ToString(), FKDFOpEngine::OpToString(Op), OutPreValue,
									OutPostValue);
	}

	RetainObject(Target->HasAnyFlags(RF_ClassDefaultObject) ? Target : Target);
	if (Target->IsA<UDataAsset>())
	{
		MarkDataAssetsChanged();
		NotifyDataAssetConsumers(); // editor edits should reflect immediately
	}
	return true;
}

bool UKDFSubsystem::RestorePropertyText(UObject* Target, const FString& PropertyPath, const FString& ValueText,
										FString& OutError)
{
	if (!IsValid(Target))
	{
		OutError = TEXT("Invalid target");
		return false;
	}
	FKDFPropertyPath Path;
	FKDFResolvedProperty Resolved;
	if (!FKDFPropertyPath::Parse(PropertyPath, Path, OutError) ||
		!FKDFPropertyResolver::Resolve(Target, Path, Resolved, OutError))
	{
		return false;
	}
	if (!FKDFValueCodec::ImportText(ValueText, Resolved.mProperty, Resolved.mValuePtr))
	{
		OutError = FString::Printf(TEXT("Could not restore '%s'"), *PropertyPath);
		return false;
	}
	if (Target->IsA<UDataAsset>())
	{
		MarkDataAssetsChanged();
		NotifyDataAssetConsumers();
	}
	return true;
}

bool UKDFSubsystem::ExportObjectToYaml(UObject* Target, bool bDiffOnly, FString& OutYaml, FString& OutError)
{
	if (!IsValid(Target))
	{
		OutError = TEXT("Invalid target");
		return false;
	}
	FKDFExportContext Context;
	Context.mGameInstance = GetGameInstance();
	Context.mDiagnostics = &mDiagnostics;
	Context.bDiffOnly = bDiffOnly;

	for (const TObjectPtr<UObject>& HandlerObject : mHandlerObjects)
	{
		IKDFDataEditorHandler* Handler = Cast<IKDFDataEditorHandler>(HandlerObject.Get());
		if (Handler == nullptr)
		{
			continue;
		}
		FKDFNode Document;
		if (Handler->ExportObject(Target, Document, Context))
		{
			OutYaml = FKDFYamlParser::EmitString(Document);
			return true;
		}
	}
	OutError = TEXT("No handler can export this object");
	return false;
}

void UKDFSubsystem::RegisterActorPatch(const FKDFActorPatch& Patch)
{
	mActorPatches.RemoveAll(
		[&Patch](const FKDFActorPatch& Existing)
		{ return Existing.mTargetClass == Patch.mTargetClass && Existing.mSourceFile == Patch.mSourceFile; });
	mActorPatches.Add(Patch);
}

void UKDFSubsystem::RegisterLazyClassWatch(const FKDFLazyClassWatch& Watch)
{
	mLazyClassWatches.RemoveAll(
		[&Watch](const FKDFLazyClassWatch& Existing)
		{
			return Existing.mBaseClass == Watch.mBaseClass && Existing.mExactTargetPath == Watch.mExactTargetPath &&
				Existing.mSourceFile == Watch.mSourceFile;
		});
	mLazyClassWatches.Add(Watch);
	bHasLazyClassWatches.store(true, std::memory_order_release);
}

void UKDFSubsystem::NotifyUObjectCreated(const UObjectBase* Object, int32 /*Index*/)
{
	// Fires for every UObject constructed engine-wide — stay cheap when no watch cares.
	if (Object == nullptr || !bHasLazyClassWatches.load(std::memory_order_acquire))
	{
		return;
	}
	// Only the creation of a new CLASS matters here (its meta-class is UBlueprintGeneratedClass);
	// instances of that class report their generated class as GetClass(), not the meta-class, so
	// they never pass this filter. Native classes are already loaded at process start — nothing to
	// watch for them.
	const UClass* MetaClass = Object->GetClass();
	if (MetaClass == nullptr || !MetaClass->IsChildOf(UBlueprintGeneratedClass::StaticClass()))
	{
		return;
	}
	UClass* NewClass = const_cast<UClass*>(static_cast<const UClass*>(Object));

	// FUObjectArray announces allocation, not completed construction or package loading. Always
	// leave this callback before touching class/CDO state, even when allocation happened on the game thread.
	TWeakObjectPtr<UKDFSubsystem> WeakThis(this);
	TWeakObjectPtr<UClass> WeakClass(NewClass);
	AsyncTask(ENamedThreads::GameThread,
			  [WeakThis, WeakClass]()
			  {
				  UKDFSubsystem* Subsystem = WeakThis.Get();
				  UClass* Class = WeakClass.Get();
				  if (Subsystem != nullptr && Class != nullptr)
				  {
					  Subsystem->QueueLazyClassForProcessing(Class);
				  }
			  });
}

void UKDFSubsystem::OnUObjectArrayShutdown() { GUObjectArray.RemoveUObjectCreateListener(this); }

void UKDFSubsystem::QueueLazyClassForProcessing(UClass* NewClass)
{
	check(IsInGameThread());
	if (!bHasLazyClassWatches.load(std::memory_order_acquire) || !IsValid(NewClass))
	{
		return;
	}
	mPendingLazyClasses.Add(NewClass);
	if (!mLazyClassTickerHandle.IsValid())
	{
		mLazyClassTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateUObject(this, &UKDFSubsystem::ProcessPendingLazyClasses));
	}
}

bool UKDFSubsystem::ProcessPendingLazyClasses(float /*DeltaTime*/)
{
	check(IsInGameThread());
	if (!bLoadedOnce)
	{
		return true;
	}

	for (auto It = mPendingLazyClasses.CreateIterator(); It; ++It)
	{
		UClass* NewClass = It->Get();
		if (!IsValid(NewClass))
		{
			It.RemoveCurrent();
			continue;
		}
		if (!IsReadyForLazyPatch(NewClass) || NewClass->ClassWithin == nullptr ||
			NewClass->ClassConstructor == nullptr)
		{
			continue;
		}

		// Never force CDO creation here. Cooked Blueprint CDOs are separate exports and may still be
		// loading after their UClass allocation; creating one early either asserts or gets overwritten.
		UObject* CDO = NewClass->GetDefaultObject(false);
		if (!IsReadyForLazyPatch(CDO))
		{
			continue;
		}

		It.RemoveCurrent();
		ApplyLazyWatchesToClass(NewClass, CDO);
	}

	if (mPendingLazyClasses.IsEmpty())
	{
		mLazyClassTickerHandle.Reset();
		return false;
	}
	return true;
}

void UKDFSubsystem::ApplyLazyWatchesToClass(UClass* NewClass, UObject* CDO)
{
	check(IsInGameThread());
	check(IsValid(NewClass) && IsValid(CDO));

	for (FKDFLazyClassWatch& Watch : mLazyClassWatches)
	{
		if (!Watch.mPropertiesNode.IsValid() || Watch.mAppliedClasses.Contains(NewClass))
		{
			continue;
		}

		if (!Watch.mExactTargetPath.IsEmpty())
		{
			// Direct `target:` that failed to resolve at apply time — the owning mod's content may
			// not have been ready yet. Matched by exact path, not ancestry: we never had a UClass* to
			// compare against, only the string the document wrote.
			if (!NewClass->GetPathName().Equals(Watch.mExactTargetPath, ESearchCase::IgnoreCase))
			{
				continue;
			}
		}
		else
		{
			UClass* BaseClass = Watch.mBaseClass.Get();
			// NewClass == BaseClass was already handled by the watch-registering document's own
			// initial apply pass — only subclasses that load later are this listener's job.
			if (BaseClass == nullptr || NewClass == BaseClass || !NewClass->IsChildOf(BaseClass))
			{
				continue;
			}
			if (!Watch.mMatchTags.IsEmpty() &&
				!FKDFPatchUtil::ObjectHasAllTags(CDO, Watch.mMatchTags, Watch.mTagPropertyName))
			{
				continue;
			}
		}

		// Record before applying. A partially failing op sequence must not repeat successful additive
		// operations if duplicate class-created notifications arrive.
		Watch.mAppliedClasses.Add(NewClass);
		RetainObject(CDO);
		FKDFPatchRecord Record;
		Record.mSourceFile = Watch.mSourceFile;
		Record.mPackRef = Watch.mPackRef;
		Record.mRootType = FName(TEXT("cdo"));

		FKDFApplyContext Context;
		Context.mGameInstance = GetGameInstance();
		Context.mSourceFile = Watch.mSourceFile;
		Context.mPackRef = Watch.mPackRef;
		Context.bDebug = Watch.bDebug;
		Context.mDiagnostics = &mDiagnostics;
		Context.mPatchRecord = &Record;
		if (FKDFPatchUtil::ApplyOpsToObject(CDO, *Watch.mPropertiesNode, Context) && !Record.mOps.IsEmpty())
		{
			UE_LOG(LogKDataForge, Log, TEXT("Lazy-loaded class '%s' matched a watch from %s — CDO patched"),
				   *NewClass->GetName(), *Watch.mSourceFile);
			mPatchRecords.Add(MoveTemp(Record));
		}
	}
}
