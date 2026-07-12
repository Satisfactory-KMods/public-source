#pragma once

#include "CoreMinimal.h"

#include "KDFHandlerBase.h"

#include "KDFDataAssetHandler.generated.h"

/**
 * Handler for `type: dataasset` documents — creates PrimaryDataAsset INSTANCES at runtime.
 *
 * Assets are created in `/KDataForge/GenAssets/<PackRef>` with save-stable id-based names and
 * announced to the asset registry, so registry-scanning subsystems (KAPI's
 * UKAPIDataAssetSubsystem, RefinedRDApi's URRDADataAssetSubsystem, …) pick them up on their next
 * scan — which KDataForge triggers automatically after the load (see
 * UKDFSubsystem::NotifyDataAssetConsumers).
 *
 * ```yaml
 * type: dataasset
 * assets:
 *   - id: CleanerItem_MyFluid
 *     class: /Script/KAPI.KAPICleanerItemDescription   # any UDataAsset subclass
 *     properties: [...]                                # cdo-style op entries
 * ```
 */
UCLASS()
class KDATAFORGE_API UKDFDataAssetHandler : public UKDFHandlerBase
{
	GENERATED_BODY()

public:
	UKDFDataAssetHandler();

	virtual bool ValidateDocument(const FKDFNode& Document, FKDFValidationContext& Context) override;
	virtual bool ApplyDocument(const FKDFNode& Document, FKDFApplyContext& Context) override;
};
