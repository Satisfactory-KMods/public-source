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

/**
 * Contract for KDataForge document handlers.
 *
 * A handler owns one YAML root type (e.g. "cdo", "recipe") and is dispatched during its declared stage.
 * Handlers never see YAML text — they receive the parsed FKDFNode model.
 *
 * Third-party mods register handler objects via UKDFSubsystem::RegisterHandler during the
 * CONSTRUCTION lifecycle phase of their game instance module (must happen before KDataForge's
 * INITIALIZATION phase applies documents).
 *
 * Prefer deriving from UKDFHandlerBase instead of implementing this interface directly.
 */
class KDATAFORGEAPI_API IKDFDataEditorHandler
{
	GENERATED_BODY()

public:
	/** Root type this handler owns; matched against the document's `type:` key (case-insensitive). */
	virtual FName GetRootType() const = 0;

	/** Stage this handler runs in. Stages execute in fixed EKDFStage order. */
	virtual EKDFStage GetStage() const = 0;

	/** Tie-break within (stage, root type). Higher runs earlier. */
	virtual int32 GetPriority() const { return 0; }

	/** Structural validation of a parsed document. Return false to skip ApplyDocument for it. */
	virtual bool ValidateDocument(const FKDFNode& Document, FKDFValidationContext& Context) = 0;

	/** Applies a validated document to the running game. */
	virtual bool ApplyDocument(const FKDFNode& Document, FKDFApplyContext& Context) = 0;

	/** Exports a runtime object back into a document of this root type. Optional (used by the in-game editor). */
	virtual bool ExportObject(UObject* Target, FKDFNode& OutDocument, FKDFExportContext& Context) { return false; }

	/** UUserWidget subclass shown as the editor panel for this root type, or nullptr for the generic inspector. */
	virtual UClass* GetEditorPanelClass() const { return nullptr; }
};
