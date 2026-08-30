#pragma once

#include "CoreMinimal.h"

#include "KDFTypes.generated.h"

class UGameInstance;

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

UENUM(BlueprintType)
enum class EKDFSeverity : uint8
{
	Info,
	Warning,
	Error
};

USTRUCT(BlueprintType)
struct KDATAFORGEAPI_API FKDFDiagnostic
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "KDataForge")
	EKDFSeverity mSeverity = EKDFSeverity::Info;

	UPROPERTY(BlueprintReadOnly, Category = "KDataForge")
	FString mMessage;

	UPROPERTY(BlueprintReadOnly, Category = "KDataForge")
	FString mFile;

	UPROPERTY(BlueprintReadOnly, Category = "KDataForge")
	int32 mLine = INDEX_NONE;

	FString ToString() const;
};

USTRUCT()
struct KDATAFORGEAPI_API FKDFOpRecord
{
	GENERATED_BODY()

	UPROPERTY()
	FString mTargetObjectPath;

	UPROPERTY()
	FString mPropertyPath;

	UPROPERTY()
	EKDFOp mOp = EKDFOp::Set;

	UPROPERTY()
	FString mValueText;
};

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

struct KDATAFORGEAPI_API FKDFContextBase
{

	UGameInstance* mGameInstance = nullptr;

	FString mSourceFile;

	FString mPackRef;

	bool bDebug = false;

	TArray<FKDFDiagnostic>* mDiagnostics = nullptr;

	void AddDiagnostic(EKDFSeverity Severity, const FString& Message, int32 Line = INDEX_NONE) const;
	void AddInfo(const FString& Message, int32 Line = INDEX_NONE) const;
	void AddWarning(const FString& Message, int32 Line = INDEX_NONE) const;
	void AddError(const FString& Message, int32 Line = INDEX_NONE) const;
};

struct KDATAFORGEAPI_API FKDFValidationContext : FKDFContextBase
{
};

struct KDATAFORGEAPI_API FKDFApplyContext : FKDFContextBase
{

	bool bDryRun = false;

	FKDFPatchRecord* mPatchRecord = nullptr;

	int32 mAppliedOpCount = 0;
};

struct KDATAFORGEAPI_API FKDFExportContext : FKDFContextBase
{

	bool bDiffOnly = false;
};
