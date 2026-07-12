#pragma once

#include "CoreMinimal.h"
#include "UObject/UnrealType.h"

#include "KDFNode.h"

class FKDFDynamicContentRegistry;

/**
 * Bidirectional YAML ↔ FProperty value conversion.
 *
 * Primary path: explicit CastField dispatch for the common property kinds.
 * Fallback path: FProperty::ImportText_Direct / ExportTextItem_Direct — this is what makes
 * "every reflectable type" honest without hand-writing every case.
 */
class KDATAFORGE_API FKDFValueCodec
{
public:
	/**
	 * Registers the live generated-content registry so ResolveClass / ResolveObject can resolve a
	 * bare generated-content id (e.g. `MySuperIngot`) in addition to its full path. Called once by
	 * UKDFSubsystem::Initialize; pass nullptr on Deinitialize to stop resolving (avoids a dangling
	 * pointer once the owning subsystem is gone).
	 */
	static void SetDynamicContentRegistry(FKDFDynamicContentRegistry* Registry);

	/**
	 * RAII scope that sets which pack a bare generated-content id resolves against first, for any
	 * ResolveClass / ResolveObject call made while the scope is alive (including ones nested inside
	 * NodeToProperty). Nest freely — the innermost scope wins, and the previous value is restored on
	 * destruction. Not thread-safe: value resolution always runs on the game thread (see the
	 * architecture doc's "All UObject work stays on the game thread" invariant), so a plain scoped
	 * variable is sufficient — no need for a thread-local.
	 */
	class KDATAFORGE_API FPackScope
	{
	public:
		explicit FPackScope(const FString& PackRef);
		~FPackScope();

		FPackScope(const FPackScope&) = delete;
		FPackScope& operator=(const FPackScope&) = delete;

	private:
		FString mPrevious;
	};

	/** The innermost active FPackScope's pack ref, or empty when no scope is active. */
	static FString GetCurrentPackScope();

	/** Writes a YAML node into a property value. Returns false with OutError set on conversion failure. */
	static bool NodeToProperty(const FKDFNode& Node, const FProperty* Property, void* ValuePtr, FString& OutError);

	/** Reads a property value into a YAML node (used by exporters and the diff viewer). */
	static bool PropertyToNode(const FProperty* Property, const void* ValuePtr, FKDFNode& OutNode);

	/** Canonical text form of a property value (vanilla snapshots, op records, checksums). */
	static FString ExportText(const FProperty* Property, const void* ValuePtr);

	/** Inverse of ExportText (revert path). */
	static bool ImportText(const FString& Text, const FProperty* Property, void* ValuePtr);

	/** Property-defined equality. */
	static bool ValuesEqual(const FProperty* Property, const void* A, const void* B);

	/**
	 * Resolves a class from a path string. Accepts `/Script/Module.Class`, `/Game/....Asset.Asset_C`,
	 * asset paths without the `_C` suffix (retried automatically), a bare generated-content id (e.g.
	 * `MySuperIngot` for a class created by an `item`/`resource`/`building`/`class`/... document with
	 * `id: MySuperIngot` — tried against the FPackScope-current pack first, then every pack), and a
	 * bare class-name shortcut (e.g. `Desc_Water` instead of
	 * `/Game/FactoryGame/Resource/RawResources/Water/Desc_Water.Desc_Water_C`) resolved by name
	 * against all currently loaded classes. If either bare-name form matches more than one class, an
	 * error is logged to LogKDataForge listing every match and the CDO/patch is still applied to the
	 * first match (sorted by full class path, for determinism).
	 */
	static UClass* ResolveClass(const FString& Path, FString& OutError);

	/**
	 * Resolves an object reference from a path string, type-checked against the property's class.
	 * Also accepts a bare generated-content id for `dataasset`/`asset` instances (see ResolveClass),
	 * and falls back to ResolveClass so class references work in plain object properties too.
	 */
	static UObject* ResolveObject(const FString& Path, const FObjectPropertyBase* Property, FString& OutError);
};
