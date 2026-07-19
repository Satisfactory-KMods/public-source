#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectKey.h"

/**
 * First-write snapshot of original ("vanilla") property values, keyed by object and property path.
 * Snapshots power the editor's diff viewer and undo/export workflows.
 * Values are stored in the codec's canonical text form.
 */
class KDATAFORGE_API FKDFVanillaCache
{
public:
	/** Records the original value for (Object, PropertyPath). First write wins — later calls are no-ops. */
	void RecordSnapshot(const UObject* Object, const FString& PropertyPath, const FString& ExportedValue);

	/** Original value, or nullptr if this property was never touched. */
	const FString* FindSnapshot(const UObject* Object, const FString& PropertyPath) const;

	/**
	 * True if PropertyName itself, or anything nested under it (array/map/struct elements —
	 * "PropertyName[0]", "PropertyName.Member", ...), was snapshotted. Snapshot keys are full
	 * property paths (e.g. "mIngredients[0].Amount"), so an exact-match lookup on the bare
	 * top-level name misses nested edits — this is what diff-only exports must use instead.
	 */
	bool HasSnapshotUnderProperty(const UObject* Object, const FString& PropertyName) const;

	/** All snapshots for one object (diff viewer), or nullptr. */
	const TMap<FString, FString>* FindObjectSnapshots(const UObject* Object) const;

	bool HasAnySnapshot(const UObject* Object) const;

	int32 NumObjects() const { return mSnapshots.Num(); }

	/** All snapshots (revert walks this to restore every touched property). */
	const TMap<FObjectKey, TMap<FString, FString>>& GetAllSnapshots() const { return mSnapshots; }

	void Reset();

private:
	TMap<FObjectKey, TMap<FString, FString>> mSnapshots;
};
