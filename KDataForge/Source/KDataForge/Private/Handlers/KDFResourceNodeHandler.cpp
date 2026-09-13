// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#include "Handlers/KDFResourceNodeHandler.h"

#include "KDFLogging.h"
#include "Reflection/KDFValueCodec.h"
#include "Resources/FGResourceDescriptor.h"
#include "Resources/FGResourceNodeBase.h"
#include "Subsystems/KDFSubsystem.h"

namespace
{

	bool ParseNodeType(const FString& Name, uint8& OutValue)
	{
		const UEnum* NodeTypeEnum = StaticEnum<EResourceNodeType>();
		int64 Value = NodeTypeEnum->GetValueByNameString(Name);
		if (Value == INDEX_NONE)
		{
			Value = NodeTypeEnum->GetValueByNameString(FString(TEXT("EResourceNodeType::")) + Name);
		}
		if (Value == INDEX_NONE || Value == static_cast<int64>(EResourceNodeType::Invalid))
		{
			return false;
		}
		OutValue = static_cast<uint8>(Value);
		return true;
	}

	FString DescribeNodeTypes(const TArray<uint8>& NodeTypes)
	{
		if (NodeTypes.IsEmpty())
		{
			return TEXT("all node types");
		}
		const UEnum* NodeTypeEnum = StaticEnum<EResourceNodeType>();
		TArray<FString> Names;
		for (const uint8 NodeType : NodeTypes)
		{
			Names.Add(NodeTypeEnum->GetNameStringByValue(NodeType));
		}
		return FString::Join(Names, TEXT(","));
	}

	bool CollectPurges(const FKDFNode& Document, TArray<FKDFNodePurge>& OutPurges, const FKDFContextBase* Context)
	{
		const FKDFNode* Entries = Document.Find(TEXT("remove"));
		if (Entries == nullptr || !Entries->IsSequence() || Entries->Num() == 0)
		{
			if (Context != nullptr)
			{
				Context->AddError(TEXT("resourcenode document requires a non-empty 'remove' sequence"));
			}
			return false;
		}

		bool bAllValid = true;
		for (const TSharedRef<FKDFNode>& EntryRef : Entries->Sequence)
		{
			const FKDFNode& Entry = EntryRef.Get();

			const FString ResourcePath = Entry.IsScalar() ? Entry.Scalar : Entry.GetString(TEXT("resource"), FString());
			if (ResourcePath.IsEmpty())
			{
				if (Context != nullptr)
				{
					Context->AddError(TEXT("Remove entry requires a non-empty 'resource' descriptor class"),
									  Entry.Line);
				}
				bAllValid = false;
				continue;
			}

			FString Error;
			UClass* ResourceClass = FKDFValueCodec::ResolveClass(ResourcePath, Error);
			if (ResourceClass == nullptr)
			{
				if (Context != nullptr)
				{
					Context->AddError(
						FString::Printf(TEXT("Could not resolve resource '%s': %s"), *ResourcePath, *Error),
						Entry.Line);
				}
				bAllValid = false;
				continue;
			}
			if (!ResourceClass->IsChildOf(UFGResourceDescriptor::StaticClass()))
			{
				if (Context != nullptr)
				{
					Context->AddError(FString::Printf(TEXT("'%s' is not a resource descriptor class"), *ResourcePath),
									  Entry.Line);
				}
				bAllValid = false;
				continue;
			}

			FKDFNodePurge Purge;
			Purge.mResourceClass = ResourceClass;
			if (Entry.IsMap())
			{
				Purge.bAllowOccupied = Entry.GetBool(TEXT("allowOccupied"), false);
				Purge.bRemoveFromScanner = Entry.GetBool(TEXT("removeFromScanner"), true);

				if (const FKDFNode* NodeTypes = Entry.Find(TEXT("nodeTypes")); NodeTypes != nullptr)
				{
					if (!NodeTypes->IsSequence())
					{
						if (Context != nullptr)
						{
							Context->AddError(TEXT("'nodeTypes' must be a sequence of EResourceNodeType names"),
											  NodeTypes->Line);
						}
						bAllValid = false;
						continue;
					}
					bool bEntryValid = true;
					for (const TSharedRef<FKDFNode>& NodeTypeRef : NodeTypes->Sequence)
					{
						uint8 NodeTypeValue = 0;
						if (!ParseNodeType(NodeTypeRef->GetString(), NodeTypeValue))
						{
							if (Context != nullptr)
							{
								Context->AddError(
									FString::Printf(TEXT("Unknown node type '%s' (expected Node, FrackingSatellite, "
														 "FrackingCore, Geyser or Deposit)"),
													*NodeTypeRef->GetString()),
									NodeTypeRef->Line);
							}
							bAllValid = false;
							bEntryValid = false;
							continue;
						}
						Purge.mNodeTypes.AddUnique(NodeTypeValue);
					}
					if (!bEntryValid)
					{

						continue;
					}
				}
			}
			OutPurges.Add(MoveTemp(Purge));
		}
		return bAllValid && !OutPurges.IsEmpty();
	}
}

UKDFResourceNodeHandler::UKDFResourceNodeHandler()
{
	mRootType = TEXT("resourcenode");

	mStage = EKDFStage::SubsystemMods;
}

bool UKDFResourceNodeHandler::ValidateDocument(const FKDFNode& Document, FKDFValidationContext& Context)
{
	TArray<FKDFNodePurge> Purges;
	CollectPurges(Document, Purges, &Context);
	return !Purges.IsEmpty();
}

bool UKDFResourceNodeHandler::ApplyDocument(const FKDFNode& Document, FKDFApplyContext& Context)
{
	TArray<FKDFNodePurge> Purges;
	CollectPurges(Document, Purges, nullptr);
	if (Purges.IsEmpty())
	{
		return false;
	}
	if (Context.bDryRun)
	{
		return true;
	}

	UKDFSubsystem* Subsystem = UKDFSubsystem::Get(Context.mGameInstance);
	if (Subsystem == nullptr)
	{
		Context.AddError(TEXT("KDataForge subsystem unavailable"));
		return false;
	}

	for (FKDFNodePurge& Purge : Purges)
	{
		Purge.mSourceFile = Context.mSourceFile;
		Purge.mPackRef = Context.mPackRef;

		Subsystem->RetainObject(Purge.mResourceClass.Get());
		Subsystem->RegisterNodePurge(Purge);

		++Context.mAppliedOpCount;
		if (Context.mPatchRecord != nullptr)
		{
			FKDFOpRecord& OpRecord = Context.mPatchRecord->mOps.AddDefaulted_GetRef();
			OpRecord.mTargetObjectPath = GetPathNameSafe(Purge.mResourceClass.Get());
			OpRecord.mPropertyPath = TEXT("ResourceNodes");
			OpRecord.mOp = EKDFOp::Remove;
			OpRecord.mValueText = DescribeNodeTypes(Purge.mNodeTypes);
		}
	}

	UE_LOG(LogKDataForge, Log, TEXT("Registered %d resource node purge rule(s) from %s"), Purges.Num(),
		   *Context.mSourceFile);
	return true;
}
