#pragma once

#include "CoreMinimal.h"

#include "KDFHandlerBase.h"

#include "KDFAssetHandler.generated.h"

UCLASS()
class KDATAFORGE_API UKDFAssetHandler : public UKDFHandlerBase
{
	GENERATED_BODY()

public:
	UKDFAssetHandler();

	virtual bool ValidateDocument(const FKDFNode& Document, FKDFValidationContext& Context) override;
	virtual bool ApplyDocument(const FKDFNode& Document, FKDFApplyContext& Context) override;
};
