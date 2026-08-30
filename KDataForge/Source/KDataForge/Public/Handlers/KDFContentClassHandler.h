#pragma once

#include "CoreMinimal.h"

#include "KDFHandlerBase.h"
#include "Loader/KDFLoaderTypes.h"

#include "KDFContentClassHandler.generated.h"

UCLASS(Abstract)
class KDATAFORGE_API UKDFContentClassHandler : public UKDFHandlerBase
{
	GENERATED_BODY()

public:
	virtual bool ValidateDocument(const FKDFNode& Document, FKDFValidationContext& Context) override;
	virtual bool ApplyDocument(const FKDFNode& Document, FKDFApplyContext& Context) override;

protected:
	bool ApplyEntry(const FKDFNode& Entry, FKDFApplyContext& Context);

	void CollectRemovals(const FKDFNode& Document, const FKDFContextBase& Context, bool bReportDiagnostics,
						 TArray<FKDFContentRemoval>& OutRemovals) const;

	bool ApplyRemovals(const FKDFNode& Document, FKDFApplyContext& Context);

	bool ApplyClassListShortcutUnlock(UObject* TargetCDO, const FKDFNode& EntriesNode, const TCHAR* UnlockClassPath,
									  const TCHAR* ValueField, const TCHAR* RequiredBaseClassPath,
									  const TCHAR* ShortcutName, FKDFApplyContext& Context);

	bool ApplyCountShortcutUnlock(UObject* TargetCDO, const FKDFNode& CountNode, const TCHAR* UnlockClassPath,
								  const TCHAR* ValueField, const TCHAR* ShortcutName, FKDFApplyContext& Context);

	bool ApplyScannableResourcesShortcutUnlock(UObject* TargetCDO, const FKDFNode& EntriesNode,
											   FKDFApplyContext& Context);

	bool ApplyNodes(UObject* TreeCDO, const FKDFNode& NodesNode, bool bAutoPath, FKDFApplyContext& Context);

	bool RemoveNodesBySchematic(UObject* TreeCDO, const FKDFNode& EntriesNode, FKDFApplyContext& Context);

	FString mEntriesKey;

	FString mDefaultParentPath;

	FString mRequiredBaseClassPath;

	EKDFContentRegKind mRegistrationKind = EKDFContentRegKind::None;

	bool bSupportsUnlocks = false;

	bool bSupportsDependencies = false;

	bool bSupportsNodes = false;

	bool bSupportsResearchDependencies = false;

	bool bAllowPerEntryRegisterAs = false;
};

UCLASS()
class KDATAFORGE_API UKDFClassHandler : public UKDFContentClassHandler
{
	GENERATED_BODY()

public:
	UKDFClassHandler();
};

UCLASS()
class KDATAFORGE_API UKDFItemHandler : public UKDFContentClassHandler
{
	GENERATED_BODY()

public:
	UKDFItemHandler();
};

UCLASS()
class KDATAFORGE_API UKDFResourceHandler : public UKDFContentClassHandler
{
	GENERATED_BODY()

public:
	UKDFResourceHandler();
};

UCLASS()
class KDATAFORGE_API UKDFBuildingHandler : public UKDFContentClassHandler
{
	GENERATED_BODY()

public:
	UKDFBuildingHandler();
};

UCLASS()
class KDATAFORGE_API UKDFRecipeHandler : public UKDFContentClassHandler
{
	GENERATED_BODY()

public:
	UKDFRecipeHandler();
};

UCLASS()
class KDATAFORGE_API UKDFSchematicHandler : public UKDFContentClassHandler
{
	GENERATED_BODY()

public:
	UKDFSchematicHandler();
};

UCLASS()
class KDATAFORGE_API UKDFResearchHandler : public UKDFContentClassHandler
{
	GENERATED_BODY()

public:
	UKDFResearchHandler();
};

UCLASS()
class KDATAFORGE_API UKDFUnlockHandler : public UKDFContentClassHandler
{
	GENERATED_BODY()

public:
	UKDFUnlockHandler();
};
