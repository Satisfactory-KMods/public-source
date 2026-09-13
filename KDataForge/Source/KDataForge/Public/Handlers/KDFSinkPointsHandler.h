// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "KDFHandlerBase.h"

#include "KDFSinkPointsHandler.generated.h"

UCLASS()
class KDATAFORGE_API UKDFSinkPointsHandler : public UKDFHandlerBase
{
	GENERATED_BODY()

public:
	UKDFSinkPointsHandler();

	virtual bool ValidateDocument(const FKDFNode& Document, FKDFValidationContext& Context) override;
	virtual bool ApplyDocument(const FKDFNode& Document, FKDFApplyContext& Context) override;
};
