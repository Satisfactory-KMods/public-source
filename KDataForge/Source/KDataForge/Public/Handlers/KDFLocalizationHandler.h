#pragma once

#include "CoreMinimal.h"

#include "KDFHandlerBase.h"

#include "KDFLocalizationHandler.generated.h"

UCLASS()
class KDATAFORGE_API UKDFLocalizationHandler : public UKDFHandlerBase
{
	GENERATED_BODY()

public:
	UKDFLocalizationHandler();

	virtual bool ValidateDocument(const FKDFNode& Document, FKDFValidationContext& Context) override;
	virtual bool ApplyDocument(const FKDFNode& Document, FKDFApplyContext& Context) override;
};
