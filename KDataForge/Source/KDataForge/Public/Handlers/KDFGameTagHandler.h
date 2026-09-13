// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "KDFHandlerBase.h"

#include "KDFGameTagHandler.generated.h"

UCLASS()
class KDATAFORGE_API UKDFGameTagHandler : public UKDFHandlerBase
{
	GENERATED_BODY()

public:
	UKDFGameTagHandler();

	virtual bool ValidateDocument(const FKDFNode& Document, FKDFValidationContext& Context) override;
	virtual bool ApplyDocument(const FKDFNode& Document, FKDFApplyContext& Context) override;
};
