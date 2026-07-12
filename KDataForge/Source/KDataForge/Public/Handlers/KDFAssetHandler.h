#pragma once

#include "CoreMinimal.h"

#include "KDFHandlerBase.h"

#include "KDFAssetHandler.generated.h"

/**
 * Handler for `type: asset` documents — 2D image files become runtime UTexture2D assets.
 *
 * ```yaml
 * type: asset
 * assets:
 *   - id: MyIcon
 *     file: icons/my-icon.png   # relative to the pack root; png / jpg / tga / bmp
 * ```
 *
 * Textures live at `/KDataForge/GenAssets/<PackRef>.Gen_<PackRef>_<Id>` (announced to the asset
 * registry) and are referenced from properties by that path (item icons etc.). Live-reload safe.
 * 2D only — no mesh/model import by policy.
 */
UCLASS()
class KDATAFORGE_API UKDFAssetHandler : public UKDFHandlerBase
{
	GENERATED_BODY()

public:
	UKDFAssetHandler();

	virtual bool ValidateDocument(const FKDFNode& Document, FKDFValidationContext& Context) override;
	virtual bool ApplyDocument(const FKDFNode& Document, FKDFApplyContext& Context) override;
};
