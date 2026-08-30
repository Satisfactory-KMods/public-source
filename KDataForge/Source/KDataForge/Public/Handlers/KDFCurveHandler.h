#pragma once

#include "CoreMinimal.h"

#include "KDFHandlerBase.h"

#include "KDFCurveHandler.generated.h"

UCLASS()
class KDATAFORGE_API UKDFCurveHandler : public UKDFHandlerBase
{
	GENERATED_BODY()

public:
	UKDFCurveHandler();

	virtual bool ValidateDocument(const FKDFNode& Document, FKDFValidationContext& Context) override;
	virtual bool ApplyDocument(const FKDFNode& Document, FKDFApplyContext& Context) override;
};
