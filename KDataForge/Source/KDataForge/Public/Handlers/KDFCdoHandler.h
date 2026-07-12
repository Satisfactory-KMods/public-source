#pragma once

#include "CoreMinimal.h"

#include "KDFHandlerBase.h"

#include "KDFCdoHandler.generated.h"

/**
 * Handler for `type: cdo` documents — the framework workhorse.
 *
 * Targets (per patch entry):
 *  - `target:` class path → the class default object (optionally `applyToSubclasses: true`)
 *  - `target:` object path → that object directly (PrimaryDataAsset instances etc.)
 *  - `target:` a sequence of paths → every entry resolved and patched the same way (mixing class
 *    and object paths in one list is fine)
 *  - `allAssetsOfClass:` class path → every asset of that class found in the asset registry
 *  - `matchTag:` gameplay tag(s) + `ofClass:` scope → every CDO/asset of the scope class (incl.
 *    subclasses) that carries the tag(s), via IGameplayTagAssetInterface or a tag container
 *    property (auto-detected, or named explicitly with `tagProperty:`)
 *
 * Extras:
 *  - `applyToSpawnedActors: true` (actor classes) registers a runtime actor patch — the ops are
 *    re-applied to every actor instance of the class when it spawns.
 *  - `propagateToInstances: true` (default) pushes changed CDO values to live instances during live
 *    reload, but only where the instance still holds the old CDO value (never overwrites user edits).
 *
 * See Docs/2.yaml-schema.md for the full grammar.
 */
UCLASS()
class KDATAFORGE_API UKDFCdoHandler : public UKDFHandlerBase
{
	GENERATED_BODY()

public:
	UKDFCdoHandler();

	virtual bool ValidateDocument(const FKDFNode& Document, FKDFValidationContext& Context) override;
	virtual bool ApplyDocument(const FKDFNode& Document, FKDFApplyContext& Context) override;
	virtual bool ExportObject(UObject* Target, FKDFNode& OutDocument, FKDFExportContext& Context) override;

private:
	bool ApplyPatchEntry(const FKDFNode& Patch, FKDFApplyContext& Context);

	/**
	 * @param PropertiesNode / bPropagate  Forwarded only to register a lazy class watch (see
	 *        FKDFLazyClassWatch) for `applyToSubclasses` / `matchTag` scopes — subclasses that load
	 *        later in the session (common for Blueprint-authored items/buildings/recipes) still get
	 *        these ops applied to their CDO, the same way the initially-loaded ones did.
	 */
	void GatherTargets(const FKDFNode& Patch, FKDFApplyContext& Context, TArray<UObject*>& OutTargets,
					   TArray<UClass*>& OutTargetClasses, const TSharedPtr<FKDFNode>& PropertiesNode, bool bPropagate);
};
