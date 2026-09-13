// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "KDFHandlerBase.h"

#include "KDFDataAssetHandler.generated.h"

UCLASS()
class KDATAFORGE_API UKDFDataAssetHandler : public UKDFHandlerBase
{
	GENERATED_BODY()

public:
	UKDFDataAssetHandler();

	virtual bool ValidateDocument(const FKDFNode& Document, FKDFValidationContext& Context) override;
	virtual bool ApplyDocument(const FKDFNode& Document, FKDFApplyContext& Context) override;
};

UCLASS()
class KDATAFORGE_API UKDFDataAssetFinalizeHandler : public UKDFDataAssetHandler
{
	GENERATED_BODY()

public:
	UKDFDataAssetFinalizeHandler();

	virtual bool ApplyDocument(const FKDFNode& Document, FKDFApplyContext& Context) override;
};
