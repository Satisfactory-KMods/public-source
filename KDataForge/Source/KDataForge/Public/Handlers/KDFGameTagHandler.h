#pragma once

#include "CoreMinimal.h"

#include "KDFHandlerBase.h"

#include "KDFGameTagHandler.generated.h"

/**
 * Handler for `type: gametag` documents — registers new gameplay tags at runtime.
 *
 * Tags are appended to the UGameplayTagsSettings CDO (in memory only) followed by a tag-tree
 * rebuild when needed — the only runtime channel that survives the engine's own tree reconstructs
 * (loose tag inis via AddTagIniSearchPath are skipped on rebuilds once consumed). Tag *container
 * edits* on properties are ordinary cdo property ops (append/remove/clear on
 * FGameplayTagContainer properties).
 *
 * See Docs/2.yaml-schema.md for the grammar.
 */
UCLASS()
class KDATAFORGE_API UKDFGameTagHandler : public UKDFHandlerBase
{
	GENERATED_BODY()

public:
	UKDFGameTagHandler();

	virtual bool ValidateDocument(const FKDFNode& Document, FKDFValidationContext& Context) override;
	virtual bool ApplyDocument(const FKDFNode& Document, FKDFApplyContext& Context) override;
	virtual bool RevertDocument(const FKDFPatchRecord& Record, FKDFApplyContext& Context) override;
};
