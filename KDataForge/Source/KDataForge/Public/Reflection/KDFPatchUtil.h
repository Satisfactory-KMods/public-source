#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "KDFTypes.h"

struct FKDFNode;

/**
 * Shared application of a `properties:` op sequence to one object — vanilla snapshots before first
 * writes, op records into the patch record, and data-asset change tracking (consumer rescan
 * notification). Used by the cdo handler, all content
 * creation handlers, and the actor patch subsystem.
 */
class KDATAFORGE_API FKDFPatchUtil
{
public:
	/** Applies the property operations to Target. Returns true when at least one op applied. */
	static bool ApplyOpsToObject(UObject* Target, const FKDFNode& PropertiesNode, FKDFApplyContext& Context);

	/** True when debug op logging is on for this context (document/pack `debug:` or KDF.Debug/-KDFDebug). */
	static bool IsDebugEnabled(bool bContextDebug);

	/** One debug line for an applied op: `[KDF DEBUG] file: target.path op : pre -> post`. */
	static void LogAppliedOp(const FString& SourceFile, const UObject* Target, const FString& PropertyPath,
							 const FString& OpName, const FString& PreValue, const FString& PostValue);

	/**
	 * Engine-cache fixups after a property write. Currently: writing `mStackSize` on an
	 * FGItemDescriptor-derived target refreshes `mCachedStackSize` from UFGResourceSettings
	 * (the game reads the cache — the KBFL UKBFLCDOItemStackSize lesson).
	 */
	static void PostWriteFixups(UObject* Target, const FString& PropertyPath);

	/**
	 * True when Object carries every tag in RequiredTags — via IGameplayTagAssetInterface, or a
	 * (named or first) FGameplayTagContainer property. Backs cdo `matchTag` scoping, both at the
	 * initial apply and for classes that load later (see FKDFLazyClassWatch).
	 */
	static bool ObjectHasAllTags(UObject* Object, const TArray<FGameplayTag>& RequiredTags,
								 const FString& TagPropertyName);

	/**
	 * `target:` on a cdo-style patch entry accepts either a single scalar path or a sequence of
	 * paths — every entry resolved and patched the same way. Shared between the runtime cdo handler
	 * and the `WITH_EDITOR`-only `KDF.ApplyToAssets` tool so both parse the grammar identically.
	 */
	static TArray<FString> CollectTargetPaths(const FKDFNode& Patch);
};
