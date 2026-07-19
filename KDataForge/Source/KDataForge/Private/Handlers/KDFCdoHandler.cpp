#include "Handlers/KDFCdoHandler.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "KDFLogging.h"
#include "Loader/KDFLoaderTypes.h"
#include "Reflection/KDFOpEngine.h"
#include "Reflection/KDFPatchUtil.h"
#include "Reflection/KDFPropertyPath.h"

#include "Reflection/KDFValueCodec.h"
#include "Subsystems/KDFSubsystem.h"
#include "UObject/UObjectHash.h"


namespace
{
	bool CollectMatchTags(const FKDFNode& Patch, TArray<FGameplayTag>& OutTags, FString& OutError)
	{
		const FKDFNode* MatchTagNode = Patch.Find(TEXT("matchTag"));
		if (MatchTagNode == nullptr)
		{
			return true; // no tag matcher on this patch
		}
		TArray<FString> TagNames;
		if (MatchTagNode->IsScalar())
		{
			TagNames.Add(MatchTagNode->Scalar);
		}
		else if (MatchTagNode->IsSequence())
		{
			for (const TSharedRef<FKDFNode>& Element : MatchTagNode->Sequence)
			{
				TagNames.Add(Element->GetString());
			}
		}
		for (const FString& TagName : TagNames)
		{
			const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*TagName), false);
			if (!Tag.IsValid())
			{
				OutError = FString::Printf(TEXT("matchTag: unknown gameplay tag '%s'"), *TagName);
				return false;
			}
			OutTags.Add(Tag);
		}
		if (OutTags.IsEmpty())
		{
			OutError = TEXT("matchTag: no valid tags given");
			return false;
		}
		return true;
	}

} // namespace

UKDFCdoHandler::UKDFCdoHandler()
{
	mRootType = TEXT("cdo");
	mStage = EKDFStage::CDOChanges;
}

bool UKDFCdoHandler::ValidateDocument(const FKDFNode& Document, FKDFValidationContext& Context)
{
	const FKDFNode* Patches = Document.Find(TEXT("patches"));
	if (Patches == nullptr || !Patches->IsSequence() || Patches->Num() == 0)
	{
		Context.AddError(TEXT("cdo document requires a non-empty 'patches' sequence"));
		return false;
	}

	for (const TSharedRef<FKDFNode>& PatchRef : Patches->Sequence)
	{
		const FKDFNode& Patch = PatchRef.Get();
		if (!Patch.IsMap())
		{
			Context.AddError(TEXT("Each 'patches' entry must be a map"));
			continue;
		}
		if (FKDFPatchUtil::CollectTargetPaths(Patch).IsEmpty() &&
			Patch.GetString(TEXT("allAssetsOfClass"), FString()).IsEmpty() && Patch.Find(TEXT("matchTag")) == nullptr)
		{
			Context.AddError(TEXT("Patch entry needs 'target', 'allAssetsOfClass', or 'matchTag' + 'ofClass'"));
		}
		if (Patch.Find(TEXT("matchTag")) != nullptr && Patch.GetString(TEXT("ofClass"), FString()).IsEmpty())
		{
			Context.AddError(TEXT("'matchTag' requires an 'ofClass' scope class"));
		}
		const FKDFNode* Properties = Patch.Find(TEXT("properties"));
		if (Properties == nullptr || !Properties->IsSequence() || Properties->Num() == 0)
		{
			Context.AddError(TEXT("Patch entry requires a non-empty 'properties' sequence"));
			continue;
		}
		for (const TSharedRef<FKDFNode>& Entry : Properties->Sequence)
		{
			FString PathString;
			EKDFOp Op = EKDFOp::Set;
			FKDFOpArgs Args;
			FString Error;
			if (!FKDFOpEngine::ParseOpEntry(Entry.Get(), PathString, Op, Args, Error))
			{
				Context.AddError(Error, Entry->Line);
				continue;
			}
			FKDFPropertyPath Path;
			if (!FKDFPropertyPath::Parse(PathString, Path, Error))
			{
				Context.AddError(Error, Entry->Line);
			}
		}
	}

	// Validation reports per-entry errors but does not stop the document load.
	return true;
}

void UKDFCdoHandler::GatherTargets(const FKDFNode& Patch, FKDFApplyContext& Context, TArray<UObject*>& OutTargets,
								   TArray<UClass*>& OutTargetClasses, const TSharedPtr<FKDFNode>& PropertiesNode)
{
	OutTargetClasses.Reset();
	UKDFSubsystem* Subsystem = UKDFSubsystem::Get(Context.mGameInstance);
	if (Subsystem == nullptr)
	{
		Context.AddError(TEXT("KDataForge subsystem unavailable"));
		return;
	}

	// `target:` accepts a single scalar path or a sequence of paths — every entry is resolved and
	// patched the same way, class or direct object, each with its own `applyToSubclasses` expansion.
	const TArray<FString> TargetPaths = FKDFPatchUtil::CollectTargetPaths(Patch);
	const FString AllAssetsClassPath = Patch.GetString(TEXT("allAssetsOfClass"), FString());

	for (const FString& TargetPath : TargetPaths)
	{
		FString ClassError;
		if (UClass* TargetClass = Subsystem->FindOrLoadClassCached(TargetPath, ClassError))
		{
			OutTargetClasses.Add(TargetClass);
			OutTargets.Add(Subsystem->GetAndRetainCDO(TargetClass));
			if (Patch.GetBool(TEXT("applyToSubclasses"), false))
			{
				FKDFLazyClassWatch Watch;
				Watch.mBaseClass = TargetClass;
				Watch.mPropertiesNode = PropertiesNode;
				Watch.mSourceFile = Context.mSourceFile;
				Watch.mPackRef = Context.mPackRef;
				Watch.bDebug = Context.bDebug;
				Watch.mAppliedClasses.Add(TargetClass);

				TArray<UClass*> DerivedClasses;
				GetDerivedClasses(TargetClass, DerivedClasses, true);
				for (UClass* Derived : DerivedClasses)
				{
					OutTargets.Add(Subsystem->GetAndRetainCDO(Derived));
					Watch.mAppliedClasses.Add(Derived);
				}

				// GetDerivedClasses only sees subclasses already loaded into memory — Blueprint-authored
				// items/buildings/recipes are frequently not loaded until the game references them. Watch
				// for the rest to load later in the session (KBFL parity, see FKDFLazyClassWatch).
				Subsystem->RegisterLazyClassWatch(Watch);
			}
		}
		else if (UObject* TargetObject = FSoftObjectPath(TargetPath).TryLoad())
		{
			// Direct object target — PrimaryDataAsset instances and other standalone assets.
			Subsystem->RetainObject(TargetObject);
			OutTargets.Add(TargetObject);
		}
		else
		{
			// Not necessarily wrong - the owning mod's content may not be ready yet this early in the
			// lifecycle (its pak not mounted, its own module Init still pending, ...). Watch for a
			// class at this exact path to load later instead of only reporting an error.
			Context.AddWarning(FString::Printf(TEXT("Target '%s' did not resolve yet (%s) - watching for it to load"),
											   *TargetPath, *ClassError));

			FKDFLazyClassWatch Watch;
			Watch.mExactTargetPath = TargetPath;
			Watch.mPropertiesNode = PropertiesNode;
			Watch.mSourceFile = Context.mSourceFile;
			Watch.mPackRef = Context.mPackRef;
			Watch.bDebug = Context.bDebug;
			Subsystem->RegisterLazyClassWatch(Watch);
		}
	}

	if (!AllAssetsClassPath.IsEmpty())
	{
		FString ClassError;
		UClass* AssetClass = Subsystem->FindOrLoadClassCached(AllAssetsClassPath, ClassError);
		if (AssetClass == nullptr)
		{
			Context.AddError(FString::Printf(TEXT("allAssetsOfClass: %s"), *ClassError));
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
				Subsystem->RetainObject(Asset);
				OutTargets.Add(Asset);
			}
		}
		if (Assets.IsEmpty())
		{
			Context.AddWarning(FString::Printf(TEXT("allAssetsOfClass '%s' matched no assets"), *AllAssetsClassPath));
		}
	}

	// Tag matcher: every CDO/asset in the `ofClass` scope that carries all `matchTag` tags.
	TArray<FGameplayTag> MatchTags;
	FString MatchTagError;
	if (!CollectMatchTags(Patch, MatchTags, MatchTagError))
	{
		Context.AddError(MatchTagError);
		return;
	}
	if (!MatchTags.IsEmpty())
	{
		const FString ScopeClassPath = Patch.GetString(TEXT("ofClass"), FString());
		if (ScopeClassPath.IsEmpty())
		{
			Context.AddError(TEXT("matchTag requires an 'ofClass' scope class"));
			return;
		}
		FString ClassError;
		UClass* ScopeClass = Subsystem->FindOrLoadClassCached(ScopeClassPath, ClassError);
		if (ScopeClass == nullptr)
		{
			Context.AddError(FString::Printf(TEXT("ofClass: %s"), *ClassError));
			return;
		}
		const FString TagPropertyName = Patch.GetString(TEXT("tagProperty"), FString());
		const int32 TargetCountBefore = OutTargets.Num();
		FKDFLazyClassWatch Watch;
		Watch.mBaseClass = ScopeClass;
		Watch.mPropertiesNode = PropertiesNode;
		Watch.mMatchTags = MatchTags;
		Watch.mTagPropertyName = TagPropertyName;
		Watch.mSourceFile = Context.mSourceFile;
		Watch.mPackRef = Context.mPackRef;
		Watch.bDebug = Context.bDebug;

		// Scope CDOs: the class itself plus all derived classes.
		TArray<UClass*> ScopeClasses = {ScopeClass};
		GetDerivedClasses(ScopeClass, ScopeClasses, true);
		for (UClass* Candidate : ScopeClasses)
		{
			UObject* CandidateCDO = Candidate->GetDefaultObject();
			if (FKDFPatchUtil::ObjectHasAllTags(CandidateCDO, MatchTags, TagPropertyName))
			{
				Subsystem->RetainObject(CandidateCDO);
				OutTargets.Add(CandidateCDO);
				Watch.mAppliedClasses.Add(Candidate);
			}
		}

		// Scope assets (PrimaryDataAsset instances etc.) unless disabled.
		if (Patch.GetBool(TEXT("matchAssets"), true))
		{
			const FAssetRegistryModule& AssetRegistryModule =
				FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
			TArray<FAssetData> Assets;
			AssetRegistryModule.Get().GetAssetsByClass(ScopeClass->GetClassPathName(), Assets, true);
			for (const FAssetData& AssetData : Assets)
			{
				UObject* Asset = AssetData.GetAsset();
				if (FKDFPatchUtil::ObjectHasAllTags(Asset, MatchTags, TagPropertyName))
				{
					Subsystem->RetainObject(Asset);
					OutTargets.Add(Asset);
				}
			}
		}

		if (OutTargets.Num() == TargetCountBefore)
		{
			Context.AddWarning(TEXT("matchTag matched no objects in the given ofClass scope"));
		}

		// Same rationale as applyToSubclasses above: GetDerivedClasses only sees classes already
		// loaded, so a matching Blueprint subclass that loads later in the session would otherwise
		// never get these ops.
		Subsystem->RegisterLazyClassWatch(Watch);
	}
}

bool UKDFCdoHandler::ApplyPatchEntry(const FKDFNode& Patch, FKDFApplyContext& Context)
{
	if (Patch.GetBool(TEXT("deferOneGameTick"), false))
	{
		const TSharedRef<FKDFNode> DeferredPatch = MakeShared<FKDFNode>(Patch);
		DeferredPatch->SetChild(TEXT("deferOneGameTick"), FKDFNode::MakeScalar(TEXT("false"), false));
		const TWeakObjectPtr<UKDFCdoHandler> WeakThis(this);
		const TWeakObjectPtr<UGameInstance> WeakGameInstance(Context.mGameInstance);
		const FString SourceFile = Context.mSourceFile;
		const FString PackRef = Context.mPackRef;
		const bool bDebug = Context.bDebug;

		const auto ScheduleNextTick = [WeakThis, WeakGameInstance, DeferredPatch, SourceFile, PackRef, bDebug]()
		{
			FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda(
				[WeakThis, WeakGameInstance, DeferredPatch, SourceFile, PackRef, bDebug](float)
				{
					UKDFCdoHandler* Handler = WeakThis.Get();
					UGameInstance* GameInstance = WeakGameInstance.Get();
					if (Handler == nullptr || GameInstance == nullptr)
					{
						return false;
					}
					FKDFApplyContext DeferredContext;
					DeferredContext.mGameInstance = GameInstance;
					DeferredContext.mSourceFile = SourceFile;
					DeferredContext.mPackRef = PackRef;
					DeferredContext.bDebug = bDebug;
					Handler->ApplyPatchEntry(DeferredPatch.Get(), DeferredContext);
					return false;
				}));
		};

		bool bWorldHasBegunPlay = false;
		if (GEngine != nullptr)
		{
			for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
			{
				const UWorld* World = WorldContext.World();
				if (World != nullptr && World->GetGameInstance() == Context.mGameInstance && World->HasBegunPlay())
				{
					bWorldHasBegunPlay = true;
					break;
				}
			}
		}

		if (bWorldHasBegunPlay)
		{
			ScheduleNextTick();
		}
		else
		{
			const TSharedRef<FDelegateHandle> WorldTickHandle = MakeShared<FDelegateHandle>();
			*WorldTickHandle = FWorldDelegates::OnWorldTickStart.AddLambda(
				[WorldTickHandle, WeakGameInstance, ScheduleNextTick](UWorld* World, ELevelTick, float)
				{
					if (World == nullptr || !World->HasBegunPlay() ||
						World->GetGameInstance() != WeakGameInstance.Get())
					{
						return;
					}
					FWorldDelegates::OnWorldTickStart.Remove(*WorldTickHandle);
					ScheduleNextTick();
				});
		}
		Context.AddInfo(TEXT("Deferred CDO patch until one game tick after BeginPlay"));
		return true;
	}

	const TSharedPtr<FKDFNode> PropertiesNode = Patch.FindShared(TEXT("properties"));
	if (!PropertiesNode.IsValid() || !PropertiesNode->IsSequence())
	{
		Context.AddError(TEXT("Patch entry requires a 'properties' sequence"));
		return false;
	}

	TArray<UObject*> Targets;
	TArray<UClass*> TargetClasses;
	GatherTargets(Patch, Context, Targets, TargetClasses, PropertiesNode);
	if (Targets.IsEmpty())
	{
		return false;
	}

	bool bAppliedAny = false;
	for (UObject* Target : Targets)
	{
		bAppliedAny |= FKDFPatchUtil::ApplyOpsToObject(Target, *PropertiesNode, Context);
	}

	// Runtime actor patch: re-apply these ops to every spawning instance of each resolved class
	// (one registration per `target:` entry that resolved to an actor class).
	if (Patch.GetBool(TEXT("applyToSpawnedActors"), false))
	{
		bool bRegisteredAny = false;
		UKDFSubsystem* Subsystem = UKDFSubsystem::Get(Context.mGameInstance);
		for (UClass* TargetClass : TargetClasses)
		{
			if (TargetClass == nullptr || !TargetClass->IsChildOf(AActor::StaticClass()))
			{
				continue;
			}
			FKDFActorPatch ActorPatch;
			ActorPatch.mTargetClass = TargetClass;
			ActorPatch.bIncludeSubclasses = Patch.GetBool(TEXT("applyToSubclasses"), false);
			ActorPatch.mPropertiesNode = PropertiesNode;
			ActorPatch.mSourceFile = Context.mSourceFile;
			ActorPatch.mPackRef = Context.mPackRef;
			Subsystem->RegisterActorPatch(ActorPatch);
			Context.AddInfo(FString::Printf(TEXT("Registered spawn-time actor patch for %s"), *TargetClass->GetName()));
			bRegisteredAny = true;
		}
		if (!bRegisteredAny)
		{
			Context.AddWarning(TEXT("applyToSpawnedActors requires a 'target' that is an actor class — ignored"));
		}
	}
	return bAppliedAny;
}

bool UKDFCdoHandler::ApplyDocument(const FKDFNode& Document, FKDFApplyContext& Context)
{
	const FKDFNode* Patches = Document.Find(TEXT("patches"));
	if (Patches == nullptr || !Patches->IsSequence())
	{
		return false;
	}
	bool bAppliedAny = false;
	for (const TSharedRef<FKDFNode>& Patch : Patches->Sequence)
	{
		bAppliedAny |= ApplyPatchEntry(Patch.Get(), Context);
	}
	return bAppliedAny;
}

bool UKDFCdoHandler::ExportObject(UObject* Target, FKDFNode& OutDocument, FKDFExportContext& Context)
{
	if (!IsValid(Target))
	{
		return false;
	}
	UKDFSubsystem* Subsystem = UKDFSubsystem::Get(Context.mGameInstance);

	const bool bIsCDO = Target->HasAnyFlags(RF_ClassDefaultObject);
	const FString TargetPath = bIsCDO ? Target->GetClass()->GetPathName() : Target->GetPathName();

	const TSharedRef<FKDFNode> Properties = FKDFNode::MakeSequence();
	const void* Container = Target;
	for (TFieldIterator<FProperty> It(Target->GetClass()); It; ++It)
	{
		const FProperty* Property = *It;
		if (Property->HasAnyPropertyFlags(CPF_Deprecated | CPF_Transient | CPF_EditorOnly))
		{
			continue;
		}
		// Snapshot keys are full property paths (e.g. "mIngredients[0].Amount"), not just the
		// top-level name — an edit nested inside an array/struct member must still mark the
		// top-level property as changed, or diff-only exports silently drop it.
		if (Context.bDiffOnly && Subsystem != nullptr &&
			!Subsystem->GetVanillaCache().HasSnapshotUnderProperty(Target, Property->GetName()))
		{
			continue;
		}
		FKDFNode ValueNode;
		if (!FKDFValueCodec::PropertyToNode(Property, Property->ContainerPtrToValuePtr<void>(Container), ValueNode))
		{
			continue;
		}
		const TSharedRef<FKDFNode> Entry = FKDFNode::MakeMap();
		Entry->SetChild(TEXT("path"), FKDFNode::MakeScalar(Property->GetAuthoredName(), false));
		Entry->SetChild(TEXT("value"), MakeShared<FKDFNode>(MoveTemp(ValueNode)));
		Properties->AddChild(Entry);
	}

	const TSharedRef<FKDFNode> PatchEntry = FKDFNode::MakeMap();
	PatchEntry->SetChild(TEXT("target"), FKDFNode::MakeScalar(TargetPath, true));
	PatchEntry->SetChild(TEXT("properties"), Properties);
	const TSharedRef<FKDFNode> Patches = FKDFNode::MakeSequence();
	Patches->AddChild(PatchEntry);

	OutDocument = *FKDFNode::MakeMap();
	OutDocument.SetChild(TEXT("type"), FKDFNode::MakeScalar(TEXT("cdo"), false));
	OutDocument.SetChild(TEXT("patches"), Patches);
	return true;
}
