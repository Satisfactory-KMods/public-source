#include "Reflection/KDFPatchUtil.h"

#include "Engine/DataAsset.h"
#include "FGResourceSettings.h"
#include "GameplayTagAssetInterface.h"
#include "HAL/IConsoleManager.h"
#include "KDFLogging.h"
#include "KDFNode.h"
#include "Misc/CommandLine.h"
#include "Reflection/KDFOpEngine.h"
#include "Reflection/KDFPropertyPath.h"
#include "Reflection/KDFValueCodec.h"
#include "Reflection/KDFVanillaCache.h"
#include "Resources/FGItemDescriptor.h"
#include "Subsystems/KDFSubsystem.h"
#include "UObject/UObjectHash.h"

namespace
{
	TAutoConsoleVariable<bool> CVarKDFDebug(TEXT("KDF.Debug"), false,
											TEXT("Log every property change KDataForge applies (pre -> post)"));

	/** Tags owned by an object: IGameplayTagAssetInterface first, then a (named or first) tag container property. */
	void GetObjectOwnedTags(UObject* Object, const FString& TagPropertyName, FGameplayTagContainer& OutTags)
	{
		if (const IGameplayTagAssetInterface* TagInterface = Cast<IGameplayTagAssetInterface>(Object))
		{
			TagInterface->GetOwnedGameplayTags(OutTags);
			return;
		}
		for (TFieldIterator<FStructProperty> It(Object->GetClass()); It; ++It)
		{
			if (It->Struct != FGameplayTagContainer::StaticStruct())
			{
				continue;
			}
			if (!TagPropertyName.IsEmpty() && !It->GetName().Equals(TagPropertyName, ESearchCase::IgnoreCase) &&
				!It->GetAuthoredName().Equals(TagPropertyName, ESearchCase::IgnoreCase))
			{
				continue;
			}
			OutTags.AppendTags(*It->ContainerPtrToValuePtr<FGameplayTagContainer>(Object));
			if (!TagPropertyName.IsEmpty())
			{
				return;
			}
		}
	}
} // namespace

bool FKDFPatchUtil::ObjectHasAllTags(UObject* Object, const TArray<FGameplayTag>& RequiredTags,
									 const FString& TagPropertyName)
{
	if (!IsValid(Object))
	{
		return false;
	}
	FGameplayTagContainer OwnedTags;
	GetObjectOwnedTags(Object, TagPropertyName, OwnedTags);
	for (const FGameplayTag& Required : RequiredTags)
	{
		if (!OwnedTags.HasTag(Required))
		{
			return false;
		}
	}
	return true;
}

bool FKDFPatchUtil::IsDebugEnabled(bool bContextDebug)
{
	static const bool bCommandLine = FParse::Param(FCommandLine::Get(), TEXT("KDFDebug"));
	return bContextDebug || bCommandLine || CVarKDFDebug.GetValueOnGameThread();
}

void FKDFPatchUtil::LogAppliedOp(const FString& SourceFile, const UObject* Target, const FString& PropertyPath,
								 const FString& OpName, const FString& PreValue, const FString& PostValue)
{
	UE_LOG(LogKDataForge, Log, TEXT("[KDF DEBUG] %s: %s.%s %s : %s -> %s"),
		   SourceFile.IsEmpty() ? TEXT("<editor>") : *SourceFile, *GetPathNameSafe(Target), *PropertyPath, *OpName,
		   *PreValue, *PostValue);
}

void FKDFPatchUtil::PostWriteFixups(UObject* Target, const FString& PropertyPath)
{
	// Stack sizes are cached: the game reads mCachedStackSize, not the enum — refresh it after any
	// write that touched mStackSize (leaf or nested path ending in it).
	if (PropertyPath.EndsWith(TEXT("mStackSize")))
	{
		if (UFGItemDescriptor* ItemDescriptor = Cast<UFGItemDescriptor>(Target))
		{
			if (const int32* FoundSize = UFGResourceSettings::Get()->mStackSizes.FindKey(ItemDescriptor->mStackSize))
			{
				ItemDescriptor->mCachedStackSize = *FoundSize;
				UE_LOG(LogKDataForge, Log, TEXT("Stack-size cache refreshed for %s (mCachedStackSize=%d)"),
					   *Target->GetPathName(), *FoundSize);
			}
		}
	}
}

TArray<FString> FKDFPatchUtil::CollectTargetPaths(const FKDFNode& Patch)
{
	TArray<FString> TargetPaths;
	const FKDFNode* TargetNode = Patch.Find(TEXT("target"));
	if (TargetNode == nullptr)
	{
		return TargetPaths;
	}
	if (TargetNode->IsScalar())
	{
		TargetPaths.Add(TargetNode->GetString());
	}
	else if (TargetNode->IsSequence())
	{
		for (const TSharedRef<FKDFNode>& Element : TargetNode->Sequence)
		{
			TargetPaths.Add(Element->GetString());
		}
	}
	TargetPaths.RemoveAll([](const FString& Path) { return Path.IsEmpty(); });
	return TargetPaths;
}

bool FKDFPatchUtil::ApplyOpsToObject(UObject* Target, const FKDFNode& PropertiesNode, bool bPropagate,
									 FKDFApplyContext& Context)
{
	UKDFSubsystem* Subsystem = UKDFSubsystem::Get(Context.mGameInstance);
	if (Subsystem == nullptr || !IsValid(Target) || !PropertiesNode.IsSequence())
	{
		return false;
	}
	FKDFVanillaCache& VanillaCache = Subsystem->GetVanillaCache();

	bool bAppliedAny = false;
	for (const TSharedRef<FKDFNode>& EntryRef : PropertiesNode.Sequence)
	{
		const FKDFNode& Entry = EntryRef.Get();
		FString PathString;
		EKDFOp Op = EKDFOp::Set;
		FKDFOpArgs Args;
		FString Error;
		if (!FKDFOpEngine::ParseOpEntry(Entry, PathString, Op, Args, Error))
		{
			Context.AddError(Error, Entry.Line);
			continue;
		}
		FKDFPropertyPath Path;
		if (!FKDFPropertyPath::Parse(PathString, Path, Error))
		{
			Context.AddError(Error, Entry.Line);
			continue;
		}

		// Snapshot the original value before the first write (revert / diff / propagation heuristic).
		FString PreValue;
		FKDFResolvedProperty PreResolved;
		const bool bPreResolved = FKDFPropertyResolver::Resolve(Target, Path, PreResolved, Error);
		if (bPreResolved)
		{
			PreValue = FKDFValueCodec::ExportText(PreResolved.mProperty, PreResolved.mValuePtr);
			VanillaCache.RecordSnapshot(Target, Path.ToString(), PreValue);
			if (Context.bLiveReloadSafeStage)
			{
				Subsystem->GetLiveReloadCache().RecordSnapshot(Target, Path.ToString(), PreValue);
			}
		}

		if (Context.bDryRun)
		{
			continue;
		}
		if (!FKDFOpEngine::ApplyOp(Target, Path, Op, Args, Error))
		{
			Context.AddError(FString::Printf(TEXT("%s (target %s)"), *Error, *Target->GetPathName()), Entry.Line);
			continue;
		}
		bAppliedAny = true;
		++Context.mAppliedOpCount;
		PostWriteFixups(Target, Path.ToString());

		FString PostValue;
		FKDFResolvedProperty PostResolved;
		if (FKDFPropertyResolver::Resolve(Target, Path, PostResolved, Error))
		{
			PostValue = FKDFValueCodec::ExportText(PostResolved.mProperty, PostResolved.mValuePtr);
		}
		if (IsDebugEnabled(Context.bDebug))
		{
			LogAppliedOp(Context.mSourceFile, Target, Path.ToString(), FKDFOpEngine::OpToString(Op), PreValue,
						 PostValue);
		}
		if (Context.mPatchRecord != nullptr)
		{
			FKDFOpRecord& OpRecord = Context.mPatchRecord->mOps.AddDefaulted_GetRef();
			OpRecord.mTargetObjectPath = Target->GetPathName();
			OpRecord.mPropertyPath = Path.ToString();
			OpRecord.mOp = Op;
			OpRecord.mValueText = PostValue;
		}

		// Live reload: push the change to instances that still hold the old CDO value.
		if (Context.bLiveReload && bPropagate && bPreResolved && Target->HasAnyFlags(RF_ClassDefaultObject))
		{
			TArray<UObject*> Instances;
			GetObjectsOfClass(Target->GetClass(), Instances, true,
							  RF_ClassDefaultObject | RF_ArchetypeObject | RF_BeginDestroyed);
			int32 PropagatedCount = 0;
			for (UObject* Instance : Instances)
			{
				FKDFResolvedProperty InstanceResolved;
				if (!FKDFPropertyResolver::Resolve(Instance, Path, InstanceResolved, Error))
				{
					continue;
				}
				const FString InstanceValue =
					FKDFValueCodec::ExportText(InstanceResolved.mProperty, InstanceResolved.mValuePtr);
				if (InstanceValue == PreValue &&
					FKDFValueCodec::ImportText(PostValue, InstanceResolved.mProperty, InstanceResolved.mValuePtr))
				{
					++PropagatedCount;
				}
			}
			if (PropagatedCount > 0)
			{
				Context.AddInfo(FString::Printf(TEXT("Propagated %s to %d unmodified instance(s) of %s"),
												*Path.ToString(), PropagatedCount, *Target->GetClass()->GetName()));
			}
		}
	}

	// Data-asset consumers (KAPI, RefinedRDApi, …) need a rescan when their assets change.
	if (bAppliedAny && Target->IsA<UDataAsset>())
	{
		Subsystem->MarkDataAssetsChanged();
	}
	return bAppliedAny;
}
