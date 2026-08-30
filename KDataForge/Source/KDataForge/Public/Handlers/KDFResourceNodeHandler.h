#pragma once

#include "CoreMinimal.h"

#include "KDFHandlerBase.h"

#include "KDFResourceNodeHandler.generated.h"

UCLASS()
class KDATAFORGE_API UKDFResourceNodeHandler : public UKDFHandlerBase
{
	GENERATED_BODY()

public:
	UKDFResourceNodeHandler();

	virtual bool ValidateDocument(const FKDFNode& Document, FKDFValidationContext& Context) override;
	virtual bool ApplyDocument(const FKDFNode& Document, FKDFApplyContext& Context) override;
};
