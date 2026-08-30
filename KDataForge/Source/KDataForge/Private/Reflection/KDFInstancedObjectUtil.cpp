#include "Reflection/KDFInstancedObjectUtil.h"

#include "Content/KDFBlueprintClassPath.h"
#include "Loader/KDFLoaderTypes.h"
#include "Reflection/KDFPatchUtil.h"
#include "Reflection/KDFPropertyPath.h"
#include "Reflection/KDFValueCodec.h"
#include "Subsystems/KDFSubsystem.h"
#include "UObject/UObjectGlobals.h"

bool FKDFInstancedObjectUtil::ApplyList(UObject* Target, const FKDFNode& EntriesNode, const TCHAR* ArrayField,
										const TCHAR* NotFoundMessage, FKDFApplyContext& Context)
{
	UKDFSubsystem* Subsystem = UKDFSubsystem::Get(Context.mGameInstance);
	if (!IsValid(Target) || Subsystem == nullptr)
	{
		return false;
	}
	if (!EntriesNode.IsSequence())
	{
		Context.AddError(FString::Printf(TEXT("'%s' must be a sequence"), ArrayField));
		return false;
	}

	FProperty* TargetProperty = FKDFPropertyResolver::FindPropertyByNameFlexible(Target->GetClass(), ArrayField);
	const FArrayProperty* TargetArray = CastField<FArrayProperty>(TargetProperty);
	const FObjectProperty* TargetInner =
		TargetArray != nullptr ? CastField<FObjectProperty>(TargetArray->Inner) : nullptr;
	if (TargetInner == nullptr)
	{
		Context.AddError(NotFoundMessage);
		return false;
	}
	if (!Context.bDryRun)
	{
		Subsystem->GetVanillaCache().RecordSnapshot(
			Target, ArrayField,
			FKDFValueCodec::ExportText(TargetArray, TargetArray->ContainerPtrToValuePtr<void>(Target)));
	}

	bool bAddedAny = false;
	for (const TSharedRef<FKDFNode>& EntryRef : EntriesNode.Sequence)
	{
		const FKDFNode& Entry = EntryRef.Get();
		const FString ClassPath = Entry.GetString(TEXT("class"), FString());
		const FString ResolvedClassPath = FKDFBlueprintClassPath::Remap(ClassPath);
		FString Error;
		UClass* EntryClass =
			!ResolvedClassPath.IsEmpty() ? Subsystem->FindOrLoadClassCached(ResolvedClassPath, Error) : nullptr;
		if (EntryClass == nullptr)
		{
			Context.AddError(FString::Printf(TEXT("Entry needs a valid 'class' (%s)"), *Error), Entry.Line);
			continue;
		}
		if (TargetInner->PropertyClass != nullptr && !EntryClass->IsChildOf(TargetInner->PropertyClass))
		{
			Context.AddError(FString::Printf(TEXT("%s is not a valid %s subclass"), *EntryClass->GetPathName(),
											 *TargetInner->PropertyClass->GetName()),
							 Entry.Line);
			continue;
		}
		if (EntryClass->HasAnyClassFlags(CLASS_Abstract))
		{
			Context.AddError(
				FString::Printf(TEXT("Cannot instantiate abstract class %s; use a concrete Blueprint subclass"),
								*EntryClass->GetPathName()),
				Entry.Line);
			continue;
		}
		if (Context.bDryRun)
		{
			continue;
		}

		UObject* EntryInstance = NewObject<UObject>(Target, EntryClass);
		if (const FKDFNode* Properties = Entry.Find(TEXT("properties")))
		{
			FKDFPatchUtil::ApplyOpsToObject(EntryInstance, *Properties, Context);
		}

		FScriptArrayHelper Helper(TargetArray, TargetArray->ContainerPtrToValuePtr<void>(Target));
		const int32 NewIndex = Helper.AddValue();
		TargetInner->SetObjectPropertyValue(Helper.GetRawPtr(NewIndex), EntryInstance);
		bAddedAny = true;
		++Context.mAppliedOpCount;
		if (Context.mPatchRecord != nullptr)
		{
			FKDFOpRecord& OpRecord = Context.mPatchRecord->mOps.AddDefaulted_GetRef();
			OpRecord.mTargetObjectPath = Target->GetPathName();
			OpRecord.mPropertyPath = ArrayField;
			OpRecord.mOp = EKDFOp::Append;
			OpRecord.mValueText = EntryClass->GetPathName();
		}
	}
	return bAddedAny;
}
