#pragma once

#include "CoreMinimal.h"
#include "KDFHandlerBase.h"

#include "KDFCdoHandler.generated.h"

UCLASS()
class KDATAFORGE_API UKDFCdoHandler : public UKDFHandlerBase
{
	GENERATED_BODY()

public:
	UKDFCdoHandler();

	virtual bool ValidateDocument(const FKDFNode& Document, FKDFValidationContext& Context) override;
	virtual bool ApplyDocument(const FKDFNode& Document, FKDFApplyContext& Context) override;
	virtual bool ExportObject(UObject* Target, FKDFNode& OutDocument, FKDFExportContext& Context) override;

private:
	bool ApplyPatchEntry(const FKDFNode& Patch, FKDFApplyContext& Context);

	void GatherTargets(const FKDFNode& Patch, FKDFApplyContext& Context, TArray<UObject*>& OutTargets,
					   TArray<UClass*>& OutTargetClasses, const TSharedPtr<FKDFNode>& PropertiesNode);
};
