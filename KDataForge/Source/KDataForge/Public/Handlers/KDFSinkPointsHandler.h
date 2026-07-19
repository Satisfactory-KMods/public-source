#pragma once

#include "CoreMinimal.h"

#include "KDFHandlerBase.h"

#include "KDFSinkPointsHandler.generated.h"

/**
 * Handler for `type: sinkpoints` documents — registers AWESOME Sink point values for item descriptors.
 *
 * Builds a transient UDataTable of FResourceSinkPointsData rows and queues it on UKDFSubsystem,
 * which registers it per world before the content registry freezes.
 *
 * See Docs/2.yaml-schema.md for the grammar.
 */
UCLASS()
class KDATAFORGE_API UKDFSinkPointsHandler : public UKDFHandlerBase
{
	GENERATED_BODY()

public:
	UKDFSinkPointsHandler();

	virtual bool ValidateDocument(const FKDFNode& Document, FKDFValidationContext& Context) override;
	virtual bool ApplyDocument(const FKDFNode& Document, FKDFApplyContext& Context) override;
};
