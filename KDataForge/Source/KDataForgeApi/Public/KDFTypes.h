#pragma once

#include "CoreMinimal.h"

#include "KDFTypes.generated.h"

class UGameInstance;

/**
 * Fixed loader stages. The pipeline iterates these in enum order — handler priority never reorders stages.
 * Keep in sync with Docs/YamlSchemaSpec.md and the canonical-op checksum version.
 */
UENUM(BlueprintType)
enum class EKDFStage : uint8
{
	GameTags,
	Assets,
	Localization,
	CDOChanges,
	Items,
	Recipes,
	Buildings,
	Schematics,
	Research,
	Unlocks,
	SubsystemMods,
	RuntimePatches,
	Validation
};

/** Property operations supported by the op engine. Which ops are valid per property kind is validated at parse time. */
UENUM(BlueprintType)
enum class EKDFOp : uint8
{
	Set,
	Add,
	Subtract,
	Multiply,
	Divide,
	Min,
	Max,
	Clamp,
	Append,
	Prepend,
	Insert,
	Remove,
	RemoveAt,
	Clear,
	Swap,
	Replace,
	Copy,
	Move,
	Duplicate,
	Sort,
	Unique,
	Reverse
};

/** Severity of a load/validation diagnostic. Errors skip the offending document but never halt the load sequence. */
UENUM(BlueprintType)
enum class EKDFSeverity : uint8
{
	Info,
	Warning,
	Error
};

/** Single diagnostic emitted while loading, validating, or applying documents. */
USTRUCT(BlueprintType)
struct KDATAFORGEAPI_API FKDFDiagnostic
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "KDataForge")
	EKDFSeverity mSeverity = EKDFSeverity::Info;

	UPROPERTY(BlueprintReadOnly, Category = "KDataForge")
	FString mMessage;

	/** Source .yml file relative to its DataForge root, empty for framework-level diagnostics. */
	UPROPERTY(BlueprintReadOnly, Category = "KDataForge")
	FString mFile;

	/** 1-based source line, INDEX_NONE when unknown. */
	UPROPERTY(BlueprintReadOnly, Category = "KDataForge")
	int32 mLine = INDEX_NONE;

	FString ToString() const;
};

/** One applied property operation, recorded in canonical form for revert, diffing, and (Phase 3) MP checksums. */
USTRUCT()
struct KDATAFORGEAPI_API FKDFOpRecord
{
	GENERATED_BODY()

	/** Full path of the target object (usually a CDO). */
	UPROPERTY()
	FString mTargetObjectPath;

	UPROPERTY()
	FString mPropertyPath;

	UPROPERTY()
	EKDFOp mOp = EKDFOp::Set;

	/** Canonical text form of the applied value (exported through the value codec). */
	UPROPERTY()
	FString mValueText;
};

/** Everything one document application did — the unit of revert and patch history. */
USTRUCT()
struct KDATAFORGEAPI_API FKDFPatchRecord
{
	GENERATED_BODY()

	UPROPERTY()
	FString mSourceFile;

	UPROPERTY()
	FName mRootType;

	UPROPERTY()
	FString mPackRef;

	UPROPERTY()
	TArray<FKDFOpRecord> mOps;
};

/** Shared context passed to handlers. Never cached by handlers — valid only for the duration of one call. */
struct KDATAFORGEAPI_API FKDFContextBase
{
	/** Owning game instance; always valid during handler calls. */
	UGameInstance* mGameInstance = nullptr;

	/** Source file of the document being processed (relative to its DataForge root). */
	FString mSourceFile;

	/** Ref of the pack the document belongs to (stable id — the save-persistence contract). */
	FString mPackRef;

	/** Debug logging for this document (document `debug:`, pack `debug:`, or the KDF.Debug CVar). */
	bool bDebug = false;

	/** Diagnostic sink owned by the framework; never null during handler calls. */
	TArray<FKDFDiagnostic>* mDiagnostics = nullptr;

	void AddDiagnostic(EKDFSeverity Severity, const FString& Message, int32 Line = INDEX_NONE) const;
	void AddInfo(const FString& Message, int32 Line = INDEX_NONE) const;
	void AddWarning(const FString& Message, int32 Line = INDEX_NONE) const;
	void AddError(const FString& Message, int32 Line = INDEX_NONE) const;
};

/** Context for IKDFDataEditorHandler::ValidateDocument. */
struct KDATAFORGEAPI_API FKDFValidationContext : FKDFContextBase
{
};

/** Context for IKDFDataEditorHandler::ApplyDocument / RevertDocument. */
struct KDATAFORGEAPI_API FKDFApplyContext : FKDFContextBase
{
	/** When true, resolve and validate everything but do not write any property. */
	bool bDryRun = false;

	/** True when applying during a live session reload (registry-frozen stages must refuse). */
	bool bLiveReload = false;

	/** True for stages whose writes are reverted and reapplied by a live reload. */
	bool bLiveReloadSafeStage = false;

	/** Record of what this document changed; the framework stores it for revert/report. */
	FKDFPatchRecord* mPatchRecord = nullptr;

	/** Running count of applied ops (for the load report). */
	int32 mAppliedOpCount = 0;
};

/** Context for IKDFDataEditorHandler::ExportObject. */
struct KDATAFORGEAPI_API FKDFExportContext : FKDFContextBase
{
	/** Export only properties that differ from the vanilla snapshot when true. */
	bool bDiffOnly = false;
};
