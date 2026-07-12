#pragma once

#include "CoreMinimal.h"

#include "KDFHandlerBase.h"
#include "Loader/KDFLoaderTypes.h"

#include "KDFContentClassHandler.generated.h"

/**
 * Generic handler for content creation document types (item/resource/building/recipe/schematic/
 * research/unlock). Each entry generates a class with save-stable identity via
 * FKDFDynamicContentRegistry (`Gen_<PackRef>_<Id>` in `/KDataForge/Gen/<PackRef>`) and applies its
 * `properties:` ops to the new CDO.
 *
 * Extras:
 *  - recipe/schematic/research: queued for per-world UModContentRegistry registration
 *    (`register: false` opts out); `class:` instead of `id:` registers an EXISTING class.
 *  - schematic: `unlocks:` creates instanced UFGUnlock subobjects appended to mUnlocks. Any
 *    concrete UFGUnlock subclass works this way — including `FGUnlockScannableResource`
 *    (`mResourcePairsToAddToScanner`) to grant resource-scanner visibility for a resource.
 *  - schematic: `dependencies:` creates instanced UFGAvailabilityDependency subobjects appended to
 *    mSchematicDependencies — the same instanced-object-array shape as `unlocks:`/mUnlocks, gating
 *    the schematic's purchase/visibility on other schematics, items, research, etc. (see
 *    ApplyInstancedObjects).
 *  - research: `nodes:` creates instanced UFGResearchTreeNode subobjects appended to mNodes,
 *    placing a schematic into the tree's visual graph (see ApplyNodes).
 *
 * Concrete subclasses only configure the entries key, default parent, stage, and registration kind.
 */
UCLASS(Abstract)
class KDATAFORGE_API UKDFContentClassHandler : public UKDFHandlerBase
{
	GENERATED_BODY()

public:
	virtual bool ValidateDocument(const FKDFNode& Document, FKDFValidationContext& Context) override;
	virtual bool ApplyDocument(const FKDFNode& Document, FKDFApplyContext& Context) override;

protected:
	bool ApplyEntry(const FKDFNode& Entry, FKDFApplyContext& Context);

	/**
	 * Shared implementation for `unlocks:`/mUnlocks and `dependencies:`/mSchematicDependencies:
	 * both are an `EditDefaultsOnly, Instanced` `TArray<TObjectPtr<T>>` of an abstract base class,
	 * populated the same way — each list entry names a concrete `class:`, gets `NewObject`'d onto
	 * the target CDO, has its own `properties:` applied, then is appended to the array. `ArrayField`
	 * is the target array's own name, resolved via the same authored-name-flexible reflection used
	 * everywhere else in KDataForge (so it works regardless of C++ access specifiers).
	 */
	bool ApplyInstancedObjects(UObject* TargetCDO, const FKDFNode& EntriesNode, const TCHAR* ArrayField,
							   const TCHAR* NotFoundMessage, FKDFApplyContext& Context);

	/**
	 * `nodes:` on a research entry — each node instances a UFGResearchTreeNode subclass (default the
	 * base game's `BPD_ResearchTreeNode`) and appends it to the tree's `mNodes` array, the same
	 * instanced-object-array shape as ApplyUnlocks/mUnlocks.
	 *
	 * `BPD_ResearchTreeNode` carries its data as a Blueprint variable, `mNodeDataStruct`, whose
	 * members are all reachable by their authored name via ordinary reflection
	 * (FKDFPropertyResolver::FindPropertyByNameFlexible matches GetAuthoredName(), so the GUID
	 * mangling Blueprint gives struct members doesn't matter) — confirmed against both the engine's
	 * own reconstructed `UFGResearchTree::IsDataValid` and SatisfactoryPlus's wiki-export reflection
	 * code, which read the same struct the same way:
	 * Each node entry's fields are all flat siblings of `schematic:` (not nested):
	 *   - `Schematic` (TSubclassOf<UFGSchematic>) — written from `schematic:`.
	 *   - `Coordinates` ({X,Y} int struct) — written from `coordinate:`.
	 *   - `Parents` / `NodesToUnhide` / `UnhiddenBy` (array of {X,Y} struct) — written from
	 *     `parents:` / `nodesToUnhide:` / `unhiddenBy:`; each entry is either a literal `{X,Y}` or a
	 *     `{schematic:}` reference resolved against every other node's own `coordinate:` in the same
	 *     `nodes:` list (order independent — every node's coordinate is recorded before any
	 *     parent/road resolves).
	 *   - `ChildrenAndRoads` (`TMap<{X,Y}, {Points: array of {X,Y}}>`) — the visual road from this
	 *     node to a child. `bAutoPath` (the research entry's `autoPath:`) picks how it's filled:
	 *     `true` inverts every node's resolved `Parents` into its parent's `ChildrenAndRoads` with a
	 *     straight two-point connector (this repo's own default — no equivalent utility exists in
	 *     KBFL/KPrivateCodeLib to call into); `false` (default) takes it verbatim from each node's
	 *     `childrenAndRoads:` (single map or a sequence of them).
	 * A field that isn't found under its expected name (e.g. a `class:`-overridden custom node type
	 * with a differently-shaped data struct) logs a warning and is skipped rather than failing the
	 * whole document — diagnostics here are never fatal, same as everywhere else in KDataForge.
	 * Anything else the node class exposes is still reachable through its own `properties:`.
	 */
	bool ApplyNodes(UObject* TreeCDO, const FKDFNode& NodesNode, bool bAutoPath, FKDFApplyContext& Context);

	/** Plural document key holding the entry sequence ("items", "recipes", …). */
	FString mEntriesKey;

	/** Parent class path used when an entry has no `parent:`. Empty = `parent:` is mandatory. */
	FString mDefaultParentPath;

	/** Base class every parent must derive from (type safety for the document type). */
	FString mRequiredBaseClassPath;

	EKDFContentRegKind mRegistrationKind = EKDFContentRegKind::None;

	bool bSupportsUnlocks = false;

	/** schematic: enables the `dependencies:` block (mSchematicDependencies — see ApplyInstancedObjects). */
	bool bSupportsDependencies = false;

	/** research: enables the `nodes:` block (see ApplyNodes). */
	bool bSupportsNodes = false;

	/** Generic `class` documents pick their registration per entry (`registerAs: recipe|schematic|research`). */
	bool bAllowPerEntryRegisterAs = false;
};

/**
 * `type: class` — the generic escape hatch: generates a class from ANY parent (native C++ or
 * Blueprint) with the same save-stable identity as the preset kinds. Optional per-entry
 * `registerAs: recipe|schematic|research` queues SML content registration.
 */
UCLASS()
class KDATAFORGE_API UKDFClassHandler : public UKDFContentClassHandler
{
	GENERATED_BODY()

public:
	UKDFClassHandler();
};

UCLASS()
class KDATAFORGE_API UKDFItemHandler : public UKDFContentClassHandler
{
	GENERATED_BODY()

public:
	UKDFItemHandler();
};

UCLASS()
class KDATAFORGE_API UKDFResourceHandler : public UKDFContentClassHandler
{
	GENERATED_BODY()

public:
	UKDFResourceHandler();
};

UCLASS()
class KDATAFORGE_API UKDFBuildingHandler : public UKDFContentClassHandler
{
	GENERATED_BODY()

public:
	UKDFBuildingHandler();
};

UCLASS()
class KDATAFORGE_API UKDFRecipeHandler : public UKDFContentClassHandler
{
	GENERATED_BODY()

public:
	UKDFRecipeHandler();
};

UCLASS()
class KDATAFORGE_API UKDFSchematicHandler : public UKDFContentClassHandler
{
	GENERATED_BODY()

public:
	UKDFSchematicHandler();
};

UCLASS()
class KDATAFORGE_API UKDFResearchHandler : public UKDFContentClassHandler
{
	GENERATED_BODY()

public:
	UKDFResearchHandler();
};

UCLASS()
class KDATAFORGE_API UKDFUnlockHandler : public UKDFContentClassHandler
{
	GENERATED_BODY()

public:
	UKDFUnlockHandler();
};
