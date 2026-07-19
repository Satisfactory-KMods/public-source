#pragma once

#include "CoreMinimal.h"

#include "KDFHandlerBase.h"

#include "KDFCurveHandler.generated.h"

/**
 * Handler for `type: curve` documents. Creates or replaces key data on UCurveFloat/UCurveVector
 * assets during the Assets stage.
 *
 * ```yaml
 * type: curve
 * curveType: float
 * curves:
 *   - id: DamageFalloff
 *     keys:
 *       - { time: 0, value: 1, interpMode: linear }
 *       - { time: 1, value: 0, interpMode: cubic }
 * ```
 *
 * Use `id:` to create an asset under `/KDataForge/GenAssets/<PackRef>`, or `target:` to edit an
 * existing cooked curve asset. Key lists replace all keys on the target curve.
 */
UCLASS()
class KDATAFORGE_API UKDFCurveHandler : public UKDFHandlerBase
{
	GENERATED_BODY()

public:
	UKDFCurveHandler();

	virtual bool ValidateDocument(const FKDFNode& Document, FKDFValidationContext& Context) override;
	virtual bool ApplyDocument(const FKDFNode& Document, FKDFApplyContext& Context) override;
};
