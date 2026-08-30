#include "Handlers/KDFDataAssetHandler.h"

#include "Content/KDFDynamicContent.h"
#include "Engine/DataAsset.h"
#include "KDFLogging.h"
#include "Reflection/KDFInstancedObjectUtil.h"
#include "Reflection/KDFPatchUtil.h"
#include "Subsystems/KDFSubsystem.h"

UKDFDataAssetHandler::UKDFDataAssetHandler()
{
	mRootType = TEXT("dataasset");
	mStage = EKDFStage::Assets;
}

bool UKDFDataAssetHandler::ValidateDocument(const FKDFNode& Document, FKDFValidationContext& Context)
{
	const FKDFNode* Assets = Document.Find(TEXT("assets"));
	if (Assets == nullptr || !Assets->IsSequence() || Assets->Num() == 0)
	{
		Context.AddError(TEXT("dataasset document requires a non-empty 'assets' sequence"));
		return false;
	}
	bool bAnyValid = false;
	for (const TSharedRef<FKDFNode>& EntryRef : Assets->Sequence)
	{
		const FKDFNode& Entry = EntryRef.Get();
		if (!Entry.IsMap() || Entry.GetString(TEXT("id"), FString()).IsEmpty() ||
			Entry.GetString(TEXT("class"), FString()).IsEmpty())
		{
			Context.AddError(TEXT("Asset entry needs 'id' and 'class'"), Entry.Line);
			continue;
		}
		bAnyValid = true;
	}
	return bAnyValid;
}

bool UKDFDataAssetHandler::ApplyDocument(const FKDFNode& Document, FKDFApplyContext& Context)
{
	UKDFSubsystem* Subsystem = UKDFSubsystem::Get(Context.mGameInstance);
	const FKDFNode* Assets = Document.Find(TEXT("assets"));
	if (Subsystem == nullptr || Assets == nullptr || !Assets->IsSequence())
	{
		return false;
	}

	bool bAppliedAny = false;
	for (const TSharedRef<FKDFNode>& EntryRef : Assets->Sequence)
	{
		const FKDFNode& Entry = EntryRef.Get();
		const FString Id = Entry.GetString(TEXT("id"), FString());
		const FString ClassPath = Entry.GetString(TEXT("class"), FString());
		if (Id.IsEmpty() || ClassPath.IsEmpty())
		{
			continue;
		}

		FString Error;
		UClass* AssetClass = Subsystem->FindOrLoadClassCached(ClassPath, Error);
		if (AssetClass == nullptr)
		{
			Context.AddError(FString::Printf(TEXT("class: %s"), *Error), Entry.Line);
			continue;
		}
		if (!AssetClass->IsChildOf(UDataAsset::StaticClass()))
		{
			Context.AddError(FString::Printf(TEXT("%s is not a data asset class"), *AssetClass->GetPathName()),
							 Entry.Line);
			continue;
		}
		if (Context.bDryRun)
		{
			continue;
		}

		UObject* Asset = Subsystem->GetDynamicContent().GetOrCreateAsset(Context.mPackRef, Id, AssetClass, Error);
		if (Asset == nullptr)
		{
			Context.AddError(Error, Entry.Line);
			continue;
		}
		bAppliedAny = true;
		++Context.mAppliedOpCount;
		if (Context.mPatchRecord != nullptr)
		{
			FKDFOpRecord& OpRecord = Context.mPatchRecord->mOps.AddDefaulted_GetRef();
			OpRecord.mTargetObjectPath = Asset->GetPathName();
			OpRecord.mPropertyPath = TEXT("<generated-asset>");
			OpRecord.mOp = EKDFOp::Set;
			OpRecord.mValueText = AssetClass->GetPathName();
		}

		if (const FKDFNode* Properties = Entry.Find(TEXT("properties")))
		{
			FKDFPatchUtil::ApplyOpsToObject(Asset, *Properties, Context);
		}
		Subsystem->MarkDataAssetsChanged();
	}
	return bAppliedAny;
}

UKDFDataAssetFinalizeHandler::UKDFDataAssetFinalizeHandler()
{
	mStage = EKDFStage::Unlocks;
}

bool UKDFDataAssetFinalizeHandler::ApplyDocument(const FKDFNode& Document, FKDFApplyContext& Context)
{
	UKDFSubsystem* Subsystem = UKDFSubsystem::Get(Context.mGameInstance);
	const FKDFNode* Assets = Document.Find(TEXT("assets"));
	if (Subsystem == nullptr || Assets == nullptr || !Assets->IsSequence())
	{
		return false;
	}

	bool bAppliedAny = false;
	for (const TSharedRef<FKDFNode>& EntryRef : Assets->Sequence)
	{
		const FKDFNode& Entry = EntryRef.Get();
		const FKDFNode* Unlocks = Entry.Find(TEXT("unlocks"));
		const FKDFNode* Dependencies = Entry.Find(TEXT("dependencies"));
		if (Unlocks == nullptr && Dependencies == nullptr)
		{
			continue;
		}

		const FString Id = Entry.GetString(TEXT("id"), FString());
		UObject* Asset = Subsystem->GetDynamicContent().FindGeneratedAsset(Context.mPackRef, Id);
		if (!IsValid(Asset))
		{
			Context.AddError(FString::Printf(TEXT("Generated data asset '%s' was not created before finalization"), *Id),
							 Entry.Line);
			continue;
		}

		if (Unlocks != nullptr)
		{
			bAppliedAny |= FKDFInstancedObjectUtil::ApplyList(Asset, *Unlocks, TEXT("mUnlocks"),
												  TEXT("Data asset has no mUnlocks object array"), Context);
		}
		if (Dependencies != nullptr)
		{
			bAppliedAny |= FKDFInstancedObjectUtil::ApplyList(Asset, *Dependencies, TEXT("mTaskDependencies"),
												  TEXT("Data asset has no mTaskDependencies object array"), Context);
		}
		Subsystem->MarkDataAssetsChanged();
	}
	return bAppliedAny;
}
