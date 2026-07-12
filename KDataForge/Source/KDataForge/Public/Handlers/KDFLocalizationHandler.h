#pragma once

#include "CoreMinimal.h"

#include "KDFHandlerBase.h"

#include "KDFLocalizationHandler.generated.h"

/**
 * Handler for `type: localization` documents — injects display strings into the live text localization
 * manager without .locres files.
 *
 * Scalar entries register new source==translation pairs; map entries (`source`/`text`) override existing
 * texts — the `source` string must match the original for the engine to swap the display string. An
 * optional `culture:` gates the document by prefix match against the current language ("de" matches
 * "de-DE"). Live-reload safe: entries re-apply through the same manager update path.
 *
 * See Docs/2.yaml-schema.md for the grammar.
 */
UCLASS()
class KDATAFORGE_API UKDFLocalizationHandler : public UKDFHandlerBase
{
	GENERATED_BODY()

public:
	UKDFLocalizationHandler();

	virtual bool ValidateDocument(const FKDFNode& Document, FKDFValidationContext& Context) override;
	virtual bool ApplyDocument(const FKDFNode& Document, FKDFApplyContext& Context) override;
};
