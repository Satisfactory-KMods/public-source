#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "KDFDataEditorHandler.h"

#include "KDFHandlerBase.generated.h"

/**
 * Convenience base for C++ handlers. Derive, set mRootType/mStage/mPriority in your constructor,
 * and override ApplyDocument (plus ValidateDocument where structural checks are possible up front).
 */
UCLASS(Abstract)
class KDATAFORGEAPI_API UKDFHandlerBase : public UObject, public IKDFDataEditorHandler
{
	GENERATED_BODY()

public:
	virtual FName GetRootType() const override { return mRootType; }

	virtual EKDFStage GetStage() const override { return mStage; }

	virtual int32 GetPriority() const override { return mPriority; }

	virtual bool ValidateDocument(const FKDFNode& Document, FKDFValidationContext& Context) override { return true; }

	virtual bool ApplyDocument(const FKDFNode& Document, FKDFApplyContext& Context) override
		PURE_VIRTUAL(UKDFHandlerBase::ApplyDocument, return false;);

protected:
	FName mRootType;
	EKDFStage mStage = EKDFStage::RuntimePatches;
	int32 mPriority = 0;
};
