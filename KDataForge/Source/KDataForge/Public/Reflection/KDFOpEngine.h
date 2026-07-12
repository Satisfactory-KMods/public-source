#pragma once

#include "CoreMinimal.h"

#include "KDFTypes.h"
#include "Reflection/KDFPropertyPath.h"

struct FKDFNode;

/** Parsed arguments of one property operation (see Docs/YamlSchemaSpec.md for which ops use which fields). */
struct KDATAFORGE_API FKDFOpArgs
{
	/** Primary value node (`value:`). */
	const FKDFNode* mValue = nullptr;

	/** Element index (`index:` — insert, remove_at, replace, duplicate, swap first index). */
	int32 mIndexA = INDEX_NONE;

	/** Second element index (`with:` — swap). */
	int32 mIndexB = INDEX_NONE;

	/** Clamp bounds (`min:` / `max:`). */
	const FKDFNode* mMin = nullptr;
	const FKDFNode* mMax = nullptr;

	/** Source property path (`from:` — copy, move). */
	FString mFromPath;
};

/**
 * Applies property operations to resolved property values.
 * Op × property-kind support is validated up front so schema errors surface as diagnostics, not silent no-ops.
 */
class KDATAFORGE_API FKDFOpEngine
{
public:
	static bool ParseOpName(const FString& Name, EKDFOp& OutOp);
	static FString OpToString(EKDFOp Op);

	/**
	 * Parses one `properties:` entry map (`path`, optional `op` [default set], `value`, `index`, `with`,
	 * `min`, `max`, `from`). OutArgs references nodes owned by Entry — keep Entry alive while applying.
	 */
	static bool ParseOpEntry(const FKDFNode& Entry, FString& OutPathString, EKDFOp& OutOp, FKDFOpArgs& OutArgs,
							 FString& OutError);

	/** True when the op makes sense for the leaf property kind (used for parse-time validation and the editor UI). */
	static bool IsOpSupported(EKDFOp Op, const FProperty* LeafProperty);

	/**
	 * Resolves Path on RootObject and applies the operation.
	 * @return false with OutError set when resolution or conversion fails; nothing is partially written.
	 */
	static bool ApplyOp(UObject* RootObject, const FKDFPropertyPath& Path, EKDFOp Op, const FKDFOpArgs& Args,
						FString& OutError);
};
