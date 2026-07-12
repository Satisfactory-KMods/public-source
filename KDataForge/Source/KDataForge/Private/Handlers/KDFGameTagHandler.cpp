#include "Handlers/KDFGameTagHandler.h"

#include "GameplayTagsManager.h"
#include "GameplayTagsSettings.h"
#include "KDFLogging.h"

namespace
{
	struct FKDFTagEntry
	{
		FString mTag;
		FString mComment;
	};

	bool CollectTagEntries(const FKDFNode& Document, TArray<FKDFTagEntry>& OutEntries, FKDFValidationContext* Context)
	{
		const FKDFNode* Tags = Document.Find(TEXT("tags"));
		if (Tags == nullptr || !Tags->IsSequence() || Tags->Num() == 0)
		{
			if (Context != nullptr)
			{
				Context->AddError(TEXT("gametag document requires a non-empty 'tags' sequence"));
			}
			return false;
		}
		bool bAllValid = true;
		for (const TSharedRef<FKDFNode>& TagNodeRef : Tags->Sequence)
		{
			const FKDFNode& TagNode = TagNodeRef.Get();
			FKDFTagEntry Entry;
			if (TagNode.IsScalar())
			{
				Entry.mTag = TagNode.Scalar;
			}
			else if (TagNode.IsMap())
			{
				Entry.mTag = TagNode.GetString(TEXT("tag"), FString());
				Entry.mComment = TagNode.GetString(TEXT("comment"), FString());
			}
			if (Entry.mTag.IsEmpty())
			{
				if (Context != nullptr)
				{
					Context->AddError(TEXT("Tag entry must be a tag name or a map with a 'tag' key"), TagNode.Line);
				}
				bAllValid = false;
				continue;
			}
			FText TagError;
			FString FixedTag;
			if (!UGameplayTagsManager::Get().IsValidGameplayTagString(Entry.mTag, &TagError, &FixedTag))
			{
				if (Context != nullptr)
				{
					Context->AddError(FString::Printf(TEXT("Invalid tag '%s': %s (suggested: '%s')"), *Entry.mTag,
													  *TagError.ToString(), *FixedTag),
									  TagNode.Line);
				}
				bAllValid = false;
				continue;
			}
			OutEntries.Add(MoveTemp(Entry));
		}
		return bAllValid && !OutEntries.IsEmpty();
	}
} // namespace

UKDFGameTagHandler::UKDFGameTagHandler()
{
	mRootType = TEXT("gametag");
	mStage = EKDFStage::GameTags;
}

bool UKDFGameTagHandler::ValidateDocument(const FKDFNode& Document, FKDFValidationContext& Context)
{
	TArray<FKDFTagEntry> Entries;
	CollectTagEntries(Document, Entries, &Context);
	return !Entries.IsEmpty(); // partially valid documents still apply their valid tags
}

bool UKDFGameTagHandler::ApplyDocument(const FKDFNode& Document, FKDFApplyContext& Context)
{
	TArray<FKDFTagEntry> Entries;
	if (!CollectTagEntries(Document, Entries, nullptr) && Entries.IsEmpty())
	{
		return false;
	}
	if (Context.bDryRun)
	{
		return true;
	}

	// Tags are appended to the UGameplayTagsSettings CDO (in memory only — never saved to config).
	// ConstructGameplayTagTree reads that list unconditionally on every rebuild, which makes this
	// survive the engine's own later reconstructs. Loose tag inis via AddTagIniSearchPath do NOT:
	// their search-path state is marked consumed after the first pass and rebuilds skip them.
	// Requires ImportTagsFromConfig=True (Satisfactory ships with it enabled).
	UGameplayTagsManager& TagsManager = UGameplayTagsManager::Get();
	UGameplayTagsSettings* TagSettings = GetMutableDefault<UGameplayTagsSettings>();
	bool bAddedAny = false;
	TSet<FString> AddedTags;
	for (const FKDFTagEntry& Entry : Entries)
	{
		const FGameplayTagTableRow Row(FName(*Entry.mTag), Entry.mComment);
		if (!TagSettings->GameplayTagList.Contains(Row))
		{
			TagSettings->GameplayTagList.Add(Row);
			bAddedAny = true;
			AddedTags.Add(Entry.mTag);
		}
	}
	if (bAddedAny)
	{
		TagsManager.DestroyGameplayTagTree();
		TagsManager.ConstructGameplayTagTree();
		UE_LOG(LogKDataForge, Log, TEXT("Gameplay tag tree rebuilt for runtime-registered tags"));
	}

	int32 RegisteredCount = 0;
	for (const FKDFTagEntry& Entry : Entries)
	{
		const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*Entry.mTag), false);
		if (Tag.IsValid())
		{
			++RegisteredCount;
			++Context.mAppliedOpCount;
			if (Context.mPatchRecord != nullptr && AddedTags.Contains(Entry.mTag))
			{
				FKDFOpRecord& OpRecord = Context.mPatchRecord->mOps.AddDefaulted_GetRef();
				OpRecord.mTargetObjectPath = TEXT("/Script/GameplayTags.GameplayTagsSettings");
				OpRecord.mPropertyPath = TEXT("GameplayTagList");
				OpRecord.mOp = EKDFOp::Append;
				OpRecord.mValueText = Entry.mTag;
			}
		}
		else
		{
			Context.AddError(FString::Printf(TEXT("Tag '%s' did not register"), *Entry.mTag));
		}
	}
	UE_LOG(LogKDataForge, Log, TEXT("Registered %d gameplay tag(s) from %s"), RegisteredCount, *Context.mSourceFile);
	return RegisteredCount > 0;
}

bool UKDFGameTagHandler::RevertDocument(const FKDFPatchRecord& Record, FKDFApplyContext& Context)
{
	UGameplayTagsSettings* TagSettings = GetMutableDefault<UGameplayTagsSettings>();
	int32 RemovedCount = 0;
	for (const FKDFOpRecord& Op : Record.mOps)
	{
		if (Op.mTargetObjectPath != TEXT("/Script/GameplayTags.GameplayTagsSettings") ||
			Op.mPropertyPath != TEXT("GameplayTagList"))
		{
			continue;
		}
		RemovedCount += TagSettings->GameplayTagList.RemoveAll(
			[&Op](const FGameplayTagTableRow& Row) { return Row.Tag == FName(*Op.mValueText); });
	}
	if (RemovedCount > 0)
	{
		UGameplayTagsManager::Get().DestroyGameplayTagTree();
		UGameplayTagsManager::Get().ConstructGameplayTagTree();
		Context.mAppliedOpCount += RemovedCount;
	}
	return RemovedCount > 0;
}
