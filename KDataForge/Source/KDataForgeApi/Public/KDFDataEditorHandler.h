#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "KDFNode.h"
#include "KDFTypes.h"

#include "KDFDataEditorHandler.generated.h"

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UKDFDataEditorHandler : public UInterface
{
	GENERATED_BODY()
};

class KDATAFORGEAPI_API IKDFDataEditorHandler
{
	GENERATED_BODY()

public:

	virtual FName GetRootType() const = 0;

	virtual EKDFStage GetStage() const = 0;

	virtual int32 GetPriority() const { return 0; }

	virtual bool ValidateDocument(const FKDFNode& Document, FKDFValidationContext& Context) = 0;

	virtual bool ApplyDocument(const FKDFNode& Document, FKDFApplyContext& Context) = 0;

	virtual bool ExportObject(UObject* Target, FKDFNode& OutDocument, FKDFExportContext& Context) { return false; }

	virtual UClass* GetEditorPanelClass() const { return nullptr; }
};
