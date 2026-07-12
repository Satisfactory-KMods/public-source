#include "Handlers/KDFSinkPointsHandler.h"

#include "Engine/DataTable.h"
#include "FGResourceSinkSettings.h"
#include "FGResourceSinkSubsystem.h"
#include "KDFLogging.h"
#include "Reflection/KDFValueCodec.h"
#include "Resources/FGItemDescriptor.h"
#include "Subsystems/KDFSubsystem.h"

namespace
{
	struct FKDFSinkPointsEntry
	{
		FString mItemPath;
		int32 mPoints = 0;
		int32 mOverridden = INDEX_NONE;
		int32 mLine = INDEX_NONE;
	};

	bool CollectSinkEntries(const FKDFNode& Document, TArray<FKDFSinkPointsEntry>& OutEntries,
							const FKDFContextBase& Context)
	{
		const FKDFNode* Entries = Document.Find(TEXT("entries"));
		if (Entries == nullptr || !Entries->IsSequence() || Entries->Num() == 0)
		{
			Context.AddError(TEXT("sinkpoints document requires a non-empty 'entries' sequence"));
			return false;
		}
		bool bAllValid = true;
		for (const TSharedRef<FKDFNode>& EntryRef : Entries->Sequence)
		{
			const FKDFNode& Entry = EntryRef.Get();
			if (!Entry.IsMap())
			{
				Context.AddError(TEXT("Sink entry must be a map with 'item' and 'points' keys"), Entry.Line);
				bAllValid = false;
				continue;
			}
			FKDFSinkPointsEntry Parsed;
			Parsed.mItemPath = Entry.GetString(TEXT("item"), FString());
			Parsed.mLine = Entry.Line;
			if (Parsed.mItemPath.IsEmpty())
			{
				Context.AddError(TEXT("Sink entry requires a non-empty 'item' descriptor class path"), Entry.Line);
				bAllValid = false;
				continue;
			}
			const FKDFNode* Points = Entry.Find(TEXT("points"));
			int64 PointsValue = 0;
			if (Points == nullptr || !Points->TryGetInt(PointsValue))
			{
				Context.AddError(
					FString::Printf(TEXT("Sink entry '%s' requires an integer 'points' value"), *Parsed.mItemPath),
					Entry.Line);
				bAllValid = false;
				continue;
			}
			Parsed.mPoints = static_cast<int32>(PointsValue);
			Parsed.mOverridden = static_cast<int32>(Entry.GetInt(TEXT("overridden"), INDEX_NONE));
			OutEntries.Add(MoveTemp(Parsed));
		}
		return bAllValid && !OutEntries.IsEmpty();
	}
} // namespace

UKDFSinkPointsHandler::UKDFSinkPointsHandler()
{
	mRootType = TEXT("sinkpoints");
	mStage = EKDFStage::SubsystemMods;
}

bool UKDFSinkPointsHandler::ValidateDocument(const FKDFNode& Document, FKDFValidationContext& Context)
{
	TArray<FKDFSinkPointsEntry> Entries;
	CollectSinkEntries(Document, Entries, Context);
	return !Entries.IsEmpty(); // partially valid documents still register their valid rows
}

bool UKDFSinkPointsHandler::ApplyDocument(const FKDFNode& Document, FKDFApplyContext& Context)
{
	// The sink subsystem reads its point tables before the content registry freezes at world begin play —
	// a table queued mid-session would never be picked up.
	if (Context.bLiveReload)
	{
		Context.AddWarning(TEXT("sink point tables cannot be re-registered during a live session — restart required"));
		return false;
	}

	TArray<FKDFSinkPointsEntry> Entries;
	CollectSinkEntries(Document, Entries, Context);
	if (Entries.IsEmpty())
	{
		return false;
	}

	UKDFSubsystem* Subsystem = UKDFSubsystem::Get(Context.mGameInstance);
	if (Subsystem == nullptr)
	{
		Context.AddError(TEXT("KDataForge subsystem unavailable"));
		return false;
	}

	const FString Track = Document.GetString(TEXT("track"), TEXT("Default"));
	const uint8 TrackValue = Track.Equals(TEXT("Exploration"), ESearchCase::IgnoreCase)
		? static_cast<uint8>(EResourceSinkTrack::RST_Exploration)
		: static_cast<uint8>(EResourceSinkTrack::RST_Default);

	UDataTable* Table = nullptr;
	if (!Context.bDryRun)
	{
		Table = NewObject<UDataTable>(Subsystem);
		Table->RowStruct = FResourceSinkPointsData::StaticStruct();
	}

	int32 RowCount = 0;
	for (const FKDFSinkPointsEntry& Entry : Entries)
	{
		FString Error;
		UClass* ItemClass = FKDFValueCodec::ResolveClass(Entry.mItemPath, Error);
		if (ItemClass == nullptr)
		{
			Context.AddError(FString::Printf(TEXT("Could not resolve item '%s': %s"), *Entry.mItemPath, *Error),
							 Entry.mLine);
			continue;
		}
		if (!ItemClass->IsChildOf(UFGItemDescriptor::StaticClass()))
		{
			Context.AddError(FString::Printf(TEXT("'%s' is not an item descriptor class"), *Entry.mItemPath),
							 Entry.mLine);
			continue;
		}
		++RowCount;
		if (Context.bDryRun)
		{
			continue;
		}

		FResourceSinkPointsData Row;
		Row.ItemClass = ItemClass;
		Row.Points = Entry.mPoints;
		Row.OverriddenResourceSinkPoints = Entry.mOverridden;
		Table->AddRow(FName(*ItemClass->GetName()), Row);

		++Context.mAppliedOpCount;
		if (Context.mPatchRecord != nullptr)
		{
			FKDFOpRecord& OpRecord = Context.mPatchRecord->mOps.AddDefaulted_GetRef();
			OpRecord.mTargetObjectPath = TEXT("/Script/FactoryGame.FGResourceSinkSubsystem");
			OpRecord.mPropertyPath = TEXT("SinkPoints");
			OpRecord.mOp = EKDFOp::Append;
			OpRecord.mValueText = FString::Printf(TEXT("%s=%d"), *ItemClass->GetPathName(), Entry.mPoints);
		}
	}

	if (Context.bDryRun || RowCount == 0)
	{
		return RowCount > 0;
	}

	// The framework registers the queued table on every world before the sink subsystem caches its points.
	Subsystem->QueueSinkTableRegistration(Table, TrackValue, Context.mSourceFile);
	UE_LOG(LogKDataForge, Log, TEXT("Queued sink point table with %d row(s) on track '%s' from %s"), RowCount, *Track,
		   *Context.mSourceFile);
	return true;
}
