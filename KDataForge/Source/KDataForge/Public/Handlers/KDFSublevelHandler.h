#pragma once

#include "CoreMinimal.h"
#include "KDFHandlerBase.h"

#include "KDFSublevelHandler.generated.h"

UCLASS()
class KDATAFORGE_API UKDFSublevelHandler : public UKDFHandlerBase
{
	GENERATED_BODY()

public:
	UKDFSublevelHandler();

	virtual bool ValidateDocument(const FKDFNode& Document, FKDFValidationContext& Context) override;
	virtual bool ApplyDocument(const FKDFNode& Document, FKDFApplyContext& Context) override;
};
