#include "Handlers/KDFSublevelHandler.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "KDFLogging.h"
#include "Reflection/KDFPatchUtil.h"
#include "Subsystems/KDFSubsystem.h"

namespace
{

	const TCHAR* const RRDLSublevelAssetPath = TEXT("/Script/RefinedRDLib.RRDLSublevelAsset");

	const TCHAR* const KBFLSubLevelSpawningPath = TEXT("/Script/KBFL.KBFLSubLevelSpawning");

	const TCHAR* const BlockedMapName = TEXT("KDF_BLOCKED_SUBLEVEL");

	UClass* FindOptionalClass(const TCHAR* ClassPath) { return FindObject<UClass>(nullptr, ClassPath); }

	TSharedRef<FKDFNode> MakeOpEntry(const TCHAR* PropertyPath, const TCHAR* Op, const TSharedPtr<FKDFNode>& Value)
	{
		const TSharedRef<FKDFNode> Entry = FKDFNode::MakeMap();
		Entry->SetChild(TEXT("path"), FKDFNode::MakeScalar(PropertyPath, false));
		if (Op != nullptr)
		{
			Entry->SetChild(TEXT("op"), FKDFNode::MakeScalar(Op, false));
		}
		if (Value.IsValid())
		{
			Entry->SetChild(TEXT("value"), Value.ToSharedRef());
		}
		return Entry;
	}

	TSharedPtr<FKDFNode> MakeBlockOps(const UObject* Target)
	{
		const UClass* RRDLClass = FindOptionalClass(RRDLSublevelAssetPath);
		if (RRDLClass != nullptr && Target->IsA(RRDLClass))
		{

			const TSharedRef<FKDFNode> Ops = FKDFNode::MakeSequence();
			Ops->AddChild(MakeOpEntry(TEXT("mMapName"), nullptr, FKDFNode::MakeScalar(BlockedMapName, true)));
			return Ops;
		}
		const UClass* KBFLClass = FindOptionalClass(KBFLSubLevelSpawningPath);
		if (KBFLClass != nullptr && Target->IsA(KBFLClass))
		{

			const TSharedRef<FKDFNode> Ops = FKDFNode::MakeSequence();
			Ops->AddChild(MakeOpEntry(TEXT("mSubLevelArray"), TEXT("clear"), nullptr));
			Ops->AddChild(MakeOpEntry(TEXT("bEnabled"), nullptr, FKDFNode::MakeScalar(TEXT("false"), false)));
			return Ops;
		}
		return nullptr;
	}

	void GatherTargets(const FKDFNode& Entry, UKDFSubsystem& Subsystem, FKDFApplyContext& Context,
					   TArray<UObject*>& OutTargets)
	{
		TArray<FString> TargetPaths;
		if (Entry.IsScalar())
		{
			TargetPaths.Add(Entry.GetString());
		}
		else
		{
			TargetPaths = FKDFPatchUtil::CollectTargetPaths(Entry);
		}
		for (const FString& TargetPath : TargetPaths)
		{

			if (UObject* TargetObject = FSoftObjectPath(TargetPath).TryLoad())
			{
				OutTargets.Add(TargetObject);
			}
			else
			{

				Context.AddWarning(FString::Printf(TEXT("sublevel target '%s' did not resolve - the mod is not "
													    "installed, or the path is wrong"),
												   *TargetPath),
								   Entry.Line);
			}
		}

		const FString AllAssetsClassPath =
			Entry.IsMap() ? Entry.GetString(TEXT("allAssetsOfClass"), FString()) : FString();
		if (AllAssetsClassPath.IsEmpty())
		{
			return;
		}

		FString ClassError;
		UClass* AssetClass = Subsystem.FindOrLoadClassCached(AllAssetsClassPath, ClassError);
		if (AssetClass == nullptr)
		{

			Context.AddInfo(FString::Printf(TEXT("sublevel class '%s' did not resolve (%s) - the owning mod is not "
												 "installed, or the class path is wrong"),
											*AllAssetsClassPath, *ClassError),
							Entry.Line);
			return;
		}
		const FAssetRegistryModule& AssetRegistryModule =
			FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		TArray<FAssetData> Assets;
		AssetRegistryModule.Get().GetAssetsByClass(AssetClass->GetClassPathName(), Assets, true);
		for (const FAssetData& AssetData : Assets)
		{
			if (UObject* Asset = AssetData.GetAsset())
			{
				OutTargets.Add(Asset);
			}
		}
		if (Assets.IsEmpty())
		{
			Context.AddInfo(FString::Printf(TEXT("allAssetsOfClass '%s' matched no assets - if that mod IS "
												 "installed the asset registry was not ready, list the assets as "
												 "explicit 'target:' paths instead"),
											*AllAssetsClassPath),
							Entry.Line);
		}
	}
}

UKDFSublevelHandler::UKDFSublevelHandler()
{
	mRootType = TEXT("sublevel");
	mStage = EKDFStage::CDOChanges;
}

bool UKDFSublevelHandler::ValidateDocument(const FKDFNode& Document, FKDFValidationContext& Context)
{
	const FKDFNode* Block = Document.Find(TEXT("block"));
	if (Block == nullptr || !Block->IsSequence() || Block->Num() == 0)
	{
		Context.AddError(TEXT("sublevel document requires a non-empty 'block' sequence"));
		return false;
	}
	for (const TSharedRef<FKDFNode>& EntryRef : Block->Sequence)
	{
		const FKDFNode& Entry = EntryRef.Get();
		bool bHasPath = false;
		if (Entry.IsScalar())
		{
			bHasPath = !Entry.GetString().IsEmpty();
		}
		else
		{
			bHasPath = !FKDFPatchUtil::CollectTargetPaths(Entry).IsEmpty() ||
				!Entry.GetString(TEXT("allAssetsOfClass"), FString()).IsEmpty();
		}
		if (!bHasPath)
		{
			Context.AddError(TEXT("Each 'block' entry must be an asset path, or a map with 'target' and/or "
								  "'allAssetsOfClass'"),
							 Entry.Line);
		}
	}

	return true;
}

bool UKDFSublevelHandler::ApplyDocument(const FKDFNode& Document, FKDFApplyContext& Context)
{
	const FKDFNode* Block = Document.Find(TEXT("block"));
	if (Block == nullptr || !Block->IsSequence())
	{
		return false;
	}
	UKDFSubsystem* Subsystem = UKDFSubsystem::Get(Context.mGameInstance);
	if (Subsystem == nullptr)
	{
		Context.AddError(TEXT("KDataForge subsystem unavailable"));
		return false;
	}

	bool bBlockedAny = false;
	for (const TSharedRef<FKDFNode>& EntryRef : Block->Sequence)
	{
		TArray<UObject*> Targets;
		GatherTargets(EntryRef.Get(), *Subsystem, Context, Targets);
		for (UObject* Target : Targets)
		{
			const TSharedPtr<FKDFNode> Ops = MakeBlockOps(Target);
			if (!Ops.IsValid())
			{
				Context.AddWarning(FString::Printf(TEXT("'%s' is not a RefinedRDLib or KBFL sublevel asset - patch "
														"it with a 'type: cdo' document instead"),
												   *Target->GetPathName()),
								   EntryRef->Line);
				continue;
			}

			Subsystem->RetainObject(Target);
			if (FKDFPatchUtil::ApplyOpsToObject(Target, *Ops, Context))
			{
				bBlockedAny = true;
				UE_LOG(LogKDataForge, Log, TEXT("Blocked sublevel asset %s (%s)"), *Target->GetPathName(),
					   *Context.mSourceFile);
			}
		}
	}
	return bBlockedAny;
}
