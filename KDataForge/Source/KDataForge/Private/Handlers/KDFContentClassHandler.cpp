#include "Handlers/KDFContentClassHandler.h"

#include "Content/KDFDynamicContent.h"
#include "KDFLogging.h"
#include "Reflection/KDFPatchUtil.h"
#include "Reflection/KDFPropertyPath.h"
#include "Reflection/KDFValueCodec.h"
#include "Subsystems/KDFSubsystem.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	// Ships with the base game; the default node type expected by SML's RegisterResearchTree
	// ("Nodes should be of default BPD_ResearchTreeNode type to be discoverable for schematics").
	const TCHAR* DefaultResearchTreeNodeClassPath =
		TEXT("/Game/FactoryGame/Schematics/Research/BPD_ResearchTreeNode.BPD_ResearchTreeNode_C");

	/** Finds a member of a struct by its Blueprint-authored name (GUID-mangled internal names included). */
	FProperty* FindStructMember(const UScriptStruct* Struct, const TCHAR* MemberName)
	{
		return Struct != nullptr
			? FKDFPropertyResolver::FindPropertyByNameFlexible(const_cast<UScriptStruct*>(Struct), MemberName)
			: nullptr;
	}

	/** Plain {X,Y} grid coordinate — used for both `mNodeDataStruct.Coordinates` and its array/map kin. */
	struct FKDFGridPoint
	{
		int32 X = 0;
		int32 Y = 0;

		bool operator==(const FKDFGridPoint& Other) const { return X == Other.X && Y == Other.Y; }
	};

	FORCEINLINE uint32 GetTypeHash(const FKDFGridPoint& Point)
	{
		return HashCombine(::GetTypeHash(Point.X), ::GetTypeHash(Point.Y));
	}

	/** Writes X/Y into an {X,Y}-shaped struct value (Coordinates, and Parents/NodesToUnhide/road elements). */
	bool WriteGridPoint(const FStructProperty* PointStruct, void* PointPtr, int32 X, int32 Y)
	{
		if (PointStruct == nullptr || PointStruct->Struct == nullptr)
		{
			return false;
		}
		FIntProperty* XProperty = CastField<FIntProperty>(FindStructMember(PointStruct->Struct, TEXT("X")));
		FIntProperty* YProperty = CastField<FIntProperty>(FindStructMember(PointStruct->Struct, TEXT("Y")));
		if (XProperty == nullptr || YProperty == nullptr)
		{
			return false;
		}
		XProperty->SetPropertyValue_InContainer(PointPtr, X);
		YProperty->SetPropertyValue_InContainer(PointPtr, Y);
		return true;
	}

	/** Replaces an array-of-{X,Y}-struct property's contents (Parents / NodesToUnhide / road Points). */
	bool WriteGridPointArray(const FArrayProperty* ArrayProperty, void* ArrayPtr, const TArray<FKDFGridPoint>& Points)
	{
		const FStructProperty* ElementStruct =
			ArrayProperty != nullptr ? CastField<FStructProperty>(ArrayProperty->Inner) : nullptr;
		if (ElementStruct == nullptr)
		{
			return false;
		}
		FScriptArrayHelper Helper(ArrayProperty, ArrayPtr);
		Helper.EmptyValues();
		for (const FKDFGridPoint& Point : Points)
		{
			const int32 NewIndex = Helper.AddValue();
			WriteGridPoint(ElementStruct, Helper.GetRawPtr(NewIndex), Point.X, Point.Y);
		}
		return true;
	}

	/**
	 * Resolves one `parents:`/`nodesToUnhide:`/road-`key:` entry: either a literal `{X,Y}` or a
	 * `{schematic:}` reference resolved against every node's own `coordinate:` in this `nodes:` list.
	 */
	bool ResolveGridRef(const FKDFNode& Entry, const TMap<UClass*, FKDFGridPoint>& CoordinateBySchematic,
						UKDFSubsystem* Subsystem, FKDFApplyContext& Context, FKDFGridPoint& OutPoint)
	{
		if (!Entry.IsMap())
		{
			Context.AddError(TEXT("Expected a map with 'X'/'Y' or a 'schematic' reference"), Entry.Line);
			return false;
		}
		const FString SchematicRef = Entry.GetString(TEXT("schematic"), FString());
		if (!SchematicRef.IsEmpty())
		{
			FString Error;
			UClass* SchematicClass = Subsystem->FindOrLoadClassCached(SchematicRef, Error);
			const FKDFGridPoint* Found =
				SchematicClass != nullptr ? CoordinateBySchematic.Find(SchematicClass) : nullptr;
			if (Found == nullptr)
			{
				Context.AddError(
					FString::Printf(TEXT("'schematic: %s' does not match any node's coordinate in this tree"),
									*SchematicRef),
					Entry.Line);
				return false;
			}
			OutPoint = *Found;
			return true;
		}
		OutPoint.X = static_cast<int32>(Entry.GetInt(TEXT("X"), 0));
		OutPoint.Y = static_cast<int32>(Entry.GetInt(TEXT("Y"), 0));
		return true;
	}

	/** Resolves a whole `parents:`/`nodesToUnhide:` sequence via ResolveGridRef. */
	TArray<FKDFGridPoint> ResolveGridRefList(const FKDFNode& ListNode,
											 const TMap<UClass*, FKDFGridPoint>& CoordinateBySchematic,
											 UKDFSubsystem* Subsystem, FKDFApplyContext& Context)
	{
		TArray<FKDFGridPoint> Result;
		if (!ListNode.IsSequence())
		{
			Context.AddError(TEXT("Expected a sequence"), ListNode.Line);
			return Result;
		}
		for (const TSharedRef<FKDFNode>& EntryRef : ListNode.Sequence)
		{
			FKDFGridPoint Point;
			if (ResolveGridRef(EntryRef.Get(), CoordinateBySchematic, Subsystem, Context, Point))
			{
				Result.Add(Point);
			}
		}
		return Result;
	}

	/** Adds `{Key, {Points}}` entries to a `ChildrenAndRoads`-shaped `TMap<{X,Y}, {Points: [{X,Y}]}>`. */
	bool WriteChildrenAndRoadsMap(const FMapProperty* RoadsProperty, void* NodeDataPtr,
								  const TArray<TPair<FKDFGridPoint, TArray<FKDFGridPoint>>>& Roads)
	{
		const FStructProperty* KeyStruct = CastField<FStructProperty>(RoadsProperty->KeyProp);
		const FStructProperty* ValueStruct = CastField<FStructProperty>(RoadsProperty->ValueProp);
		const FArrayProperty* PointsMember = ValueStruct != nullptr
			? CastField<FArrayProperty>(FindStructMember(ValueStruct->Struct, TEXT("Points")))
			: nullptr;
		if (KeyStruct == nullptr || PointsMember == nullptr)
		{
			return false;
		}

		FScriptMapHelper Helper(RoadsProperty, RoadsProperty->ContainerPtrToValuePtr<void>(NodeDataPtr));
		for (const TPair<FKDFGridPoint, TArray<FKDFGridPoint>>& Road : Roads)
		{
			const int32 NewIndex = Helper.AddDefaultValue_Invalid_NeedsRehash();
			WriteGridPoint(KeyStruct, Helper.GetKeyPtr(NewIndex), Road.Key.X, Road.Key.Y);
			void* ValuePtr = Helper.GetValuePtr(NewIndex);
			WriteGridPointArray(PointsMember, PointsMember->ContainerPtrToValuePtr<void>(ValuePtr), Road.Value);
		}
		Helper.Rehash();
		return true;
	}

	/** Resolves a manual `childrenAndRoads:` block (single map or a sequence of them) and writes it. */
	bool WriteChildrenAndRoadsFromYaml(const FStructProperty& NodeDataStruct, void* NodeDataPtr,
									   const FKDFNode& RawNode,
									   const TMap<UClass*, FKDFGridPoint>& CoordinateBySchematic,
									   UKDFSubsystem* Subsystem, FKDFApplyContext& Context)
	{
		const FMapProperty* RoadsProperty =
			CastField<FMapProperty>(FindStructMember(NodeDataStruct.Struct, TEXT("ChildrenAndRoads")));
		if (RoadsProperty == nullptr)
		{
			Context.AddWarning(TEXT("Node class has no discoverable 'ChildrenAndRoads' field"), RawNode.Line);
			return false;
		}

		TArray<const FKDFNode*> Entries;
		if (RawNode.IsSequence())
		{
			for (const TSharedRef<FKDFNode>& EntryRef : RawNode.Sequence)
			{
				Entries.Add(&EntryRef.Get());
			}
		}
		else if (RawNode.IsMap())
		{
			Entries.Add(&RawNode);
		}

		TArray<TPair<FKDFGridPoint, TArray<FKDFGridPoint>>> Roads;
		for (const FKDFNode* EntryPtr : Entries)
		{
			const FKDFNode* KeyNode = EntryPtr->Find(TEXT("key"));
			const FKDFNode* PointsNode = EntryPtr->Find(TEXT("points"));
			if (KeyNode == nullptr || PointsNode == nullptr || !PointsNode->IsSequence())
			{
				Context.AddError(TEXT("'childrenAndRoads' entry needs a 'key' and a 'points' sequence"),
								 EntryPtr->Line);
				continue;
			}
			FKDFGridPoint KeyPoint;
			if (!ResolveGridRef(*KeyNode, CoordinateBySchematic, Subsystem, Context, KeyPoint))
			{
				continue;
			}
			TArray<FKDFGridPoint> Points;
			for (const TSharedRef<FKDFNode>& PointEntryRef : PointsNode->Sequence)
			{
				const FKDFNode& PointEntry = PointEntryRef.Get();
				Points.Add(FKDFGridPoint{static_cast<int32>(PointEntry.GetInt(TEXT("X"), 0)),
										 static_cast<int32>(PointEntry.GetInt(TEXT("Y"), 0))});
			}
			Roads.Add(TPair<FKDFGridPoint, TArray<FKDFGridPoint>>(KeyPoint, Points));
		}
		return WriteChildrenAndRoadsMap(RoadsProperty, NodeDataPtr, Roads);
	}

	/** Per-entry working state threaded through ApplyNodes' multi-pass grid-layout resolution. */
	struct FKDFResearchNodeWork
	{
		UObject* Instance = nullptr;
		UClass* NodeClass = nullptr;
		UClass* SchematicClass = nullptr;
		FKDFGridPoint Coordinate;
		bool bHasCoordinate = false;
		const FStructProperty* NodeDataStruct = nullptr;
		void* NodeDataPtr = nullptr;
		const FKDFNode* SourceNode = nullptr; // the whole `nodes:` entry — schematic/coordinate/parents/… live here
	};

	/**
	 * Resolves and writes one of the node's array-of-{X,Y} fields — `parents:`/`Parents`,
	 * `nodesToUnhide:`/`NodesToUnhide`, `unhiddenBy:`/`UnhiddenBy` — all the same shape. No-op if
	 * `YamlKey` isn't present on the node entry. Optionally hands back the resolved points (used
	 * for `parents:` when `bAutoPath` needs them again in phase 3).
	 */
	void ResolveAndWriteGridArray(const FKDFResearchNodeWork& Work, const TCHAR* YamlKey, const TCHAR* MemberName,
								  const TMap<UClass*, FKDFGridPoint>& CoordinateBySchematic, UKDFSubsystem* Subsystem,
								  FKDFApplyContext& Context, TArray<FKDFGridPoint>* OutResolved = nullptr)
	{
		const FKDFNode* ListNode = Work.SourceNode->Find(YamlKey);
		if (ListNode == nullptr)
		{
			return;
		}
		TArray<FKDFGridPoint> Resolved = ResolveGridRefList(*ListNode, CoordinateBySchematic, Subsystem, Context);
		const FArrayProperty* Member =
			CastField<FArrayProperty>(FindStructMember(Work.NodeDataStruct->Struct, MemberName));
		if (Member != nullptr)
		{
			WriteGridPointArray(Member, Member->ContainerPtrToValuePtr<void>(Work.NodeDataPtr), Resolved);
		}
		else
		{
			Context.AddWarning(FString::Printf(TEXT("Node class %s has no discoverable '%s' field"),
											   *Work.NodeClass->GetName(), MemberName),
							   ListNode->Line);
		}
		if (OutResolved != nullptr)
		{
			*OutResolved = MoveTemp(Resolved);
		}
	}
} // namespace

bool UKDFContentClassHandler::ValidateDocument(const FKDFNode& Document, FKDFValidationContext& Context)
{
	const FKDFNode* Entries = Document.Find(mEntriesKey);
	if (Entries == nullptr || !Entries->IsSequence() || Entries->Num() == 0)
	{
		Context.AddError(FString::Printf(TEXT("%s document requires a non-empty '%s' sequence"),
										 *GetRootType().ToString(), *mEntriesKey));
		return false;
	}
	bool bAnyValid = false;
	for (const TSharedRef<FKDFNode>& EntryRef : Entries->Sequence)
	{
		const FKDFNode& Entry = EntryRef.Get();
		if (!Entry.IsMap())
		{
			Context.AddError(TEXT("Entry must be a map"), Entry.Line);
			continue;
		}
		const bool bHasId = !Entry.GetString(TEXT("id"), FString()).IsEmpty();
		const bool bHasClass = !Entry.GetString(TEXT("class"), FString()).IsEmpty();
		if (!bHasId && !bHasClass)
		{
			Context.AddError(TEXT("Entry needs an 'id' (generate) or a 'class' (register-only)"), Entry.Line);
			continue;
		}
		if (bHasClass && mRegistrationKind == EKDFContentRegKind::None && !bAllowPerEntryRegisterAs)
		{
			Context.AddError(TEXT("'class' (register-only) is only valid for recipe/schematic/research documents"),
							 Entry.Line);
			continue;
		}
		if (!bHasId && Entry.GetString(TEXT("parent"), FString()).IsEmpty() && mDefaultParentPath.IsEmpty())
		{
			Context.AddError(TEXT("Entry requires a 'parent' class for this document type"), Entry.Line);
			continue;
		}
		bAnyValid = true;
	}
	return bAnyValid;
}

bool UKDFContentClassHandler::ApplyInstancedObjects(UObject* TargetCDO, const FKDFNode& EntriesNode,
													const TCHAR* ArrayField, const TCHAR* NotFoundMessage,
													FKDFApplyContext& Context)
{
	UKDFSubsystem* Subsystem = UKDFSubsystem::Get(Context.mGameInstance);
	if (!EntriesNode.IsSequence())
	{
		Context.AddError(FString::Printf(TEXT("'%s' must be a sequence"), ArrayField));
		return false;
	}

	// Resolve the target instanced object array via reflection so the stub layout stays authoritative.
	FProperty* TargetProperty = FKDFPropertyResolver::FindPropertyByNameFlexible(TargetCDO->GetClass(), ArrayField);
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
			TargetCDO, ArrayField,
			FKDFValueCodec::ExportText(TargetArray, TargetArray->ContainerPtrToValuePtr<void>(TargetCDO)));
	}

	bool bAddedAny = false;
	for (const TSharedRef<FKDFNode>& EntryRef : EntriesNode.Sequence)
	{
		const FKDFNode& Entry = EntryRef.Get();
		const FString ClassPath = Entry.GetString(TEXT("class"), FString());
		FString Error;
		UClass* EntryClass = !ClassPath.IsEmpty() ? Subsystem->FindOrLoadClassCached(ClassPath, Error) : nullptr;
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

		UObject* EntryInstance = NewObject<UObject>(TargetCDO, EntryClass);
		if (const FKDFNode* Properties = Entry.Find(TEXT("properties")))
		{
			FKDFPatchUtil::ApplyOpsToObject(EntryInstance, *Properties, false, Context);
		}

		FScriptArrayHelper Helper(TargetArray, TargetArray->ContainerPtrToValuePtr<void>(TargetCDO));
		const int32 NewIndex = Helper.AddValue();
		TargetInner->SetObjectPropertyValue(Helper.GetRawPtr(NewIndex), EntryInstance);
		bAddedAny = true;
		++Context.mAppliedOpCount;
		if (Context.mPatchRecord != nullptr)
		{
			FKDFOpRecord& OpRecord = Context.mPatchRecord->mOps.AddDefaulted_GetRef();
			OpRecord.mTargetObjectPath = TargetCDO->GetPathName();
			OpRecord.mPropertyPath = ArrayField;
			OpRecord.mOp = EKDFOp::Append;
			OpRecord.mValueText = EntryClass->GetPathName();
		}
	}
	return bAddedAny;
}

bool UKDFContentClassHandler::ApplyNodes(UObject* TreeCDO, const FKDFNode& NodesNode, bool bAutoPath,
										 FKDFApplyContext& Context)
{
	UKDFSubsystem* Subsystem = UKDFSubsystem::Get(Context.mGameInstance);
	if (!NodesNode.IsSequence())
	{
		Context.AddError(TEXT("'nodes' must be a sequence"));
		return false;
	}

	// mNodes is an instanced TArray<UFGResearchTreeNode*> — resolve it via reflection so the stub layout stays
	// authoritative.
	FProperty* NodesProperty = FKDFPropertyResolver::FindPropertyByNameFlexible(TreeCDO->GetClass(), TEXT("mNodes"));
	const FArrayProperty* NodesArray = CastField<FArrayProperty>(NodesProperty);
	const FObjectProperty* NodeInner = NodesArray != nullptr ? CastField<FObjectProperty>(NodesArray->Inner) : nullptr;
	if (NodeInner == nullptr)
	{
		Context.AddError(TEXT("Research tree class has no mNodes object array"));
		return false;
	}
	if (!Context.bDryRun)
	{
		Subsystem->GetVanillaCache().RecordSnapshot(
			TreeCDO, TEXT("mNodes"),
			FKDFValueCodec::ExportText(NodesArray, NodesArray->ContainerPtrToValuePtr<void>(TreeCDO)));
	}

	FString Error;
	UClass* SchematicBaseClass = Subsystem->FindOrLoadClassCached(TEXT("/Script/FactoryGame.FGSchematic"), Error);

	// --- Phase 1: create every node instance, resolve its class/schematic/coordinate. Every node's
	// coordinate must be known before phase 2 resolves `schematic:` parent/road references, since
	// those can point at a node defined later in the same list.
	TArray<FKDFResearchNodeWork> WorkingNodes;
	TMap<UClass*, FKDFGridPoint> CoordinateBySchematic;
	for (const TSharedRef<FKDFNode>& NodeRef : NodesNode.Sequence)
	{
		const FKDFNode& Node = NodeRef.Get();
		const FString NodeClassPath = Node.GetString(TEXT("class"), FString(DefaultResearchTreeNodeClassPath));
		UClass* NodeClass = Subsystem->FindOrLoadClassCached(NodeClassPath, Error);
		if (NodeClass == nullptr)
		{
			Context.AddError(FString::Printf(TEXT("Node entry needs a valid 'class' (%s)"), *Error), Node.Line);
			continue;
		}
		if (NodeInner->PropertyClass != nullptr && !NodeClass->IsChildOf(NodeInner->PropertyClass))
		{
			Context.AddError(FString::Printf(TEXT("%s is not a research tree node class"), *NodeClass->GetPathName()),
							 Node.Line);
			continue;
		}
		if (Context.bDryRun)
		{
			continue;
		}

		UObject* NodeInstance = NewObject<UObject>(TreeCDO, NodeClass);

		FKDFResearchNodeWork& Work = WorkingNodes.AddDefaulted_GetRef();
		Work.Instance = NodeInstance;
		Work.NodeClass = NodeClass;
		Work.SourceNode = &Node;
		Work.NodeDataStruct = CastField<FStructProperty>(
			FKDFPropertyResolver::FindPropertyByNameFlexible(NodeClass, TEXT("mNodeDataStruct")));
		Work.NodeDataPtr =
			Work.NodeDataStruct != nullptr ? Work.NodeDataStruct->ContainerPtrToValuePtr<void>(NodeInstance) : nullptr;

		const FString SchematicPath = Node.GetString(TEXT("schematic"), FString());
		if (!SchematicPath.IsEmpty())
		{
			UClass* SchematicClass = Subsystem->FindOrLoadClassCached(SchematicPath, Error);
			if (SchematicClass == nullptr ||
				(SchematicBaseClass != nullptr && !SchematicClass->IsChildOf(SchematicBaseClass)))
			{
				Context.AddError(FString::Printf(TEXT("Node 'schematic' is invalid (%s)"),
												 SchematicClass == nullptr ? *Error : TEXT("not a schematic class")),
								 Node.Line);
			}
			else if (Work.NodeDataPtr != nullptr)
			{
				FClassProperty* SchematicMember =
					CastField<FClassProperty>(FindStructMember(Work.NodeDataStruct->Struct, TEXT("Schematic")));
				if (SchematicMember != nullptr)
				{
					SchematicMember->SetObjectPropertyValue(Work.NodeDataPtr, SchematicClass);
					Work.SchematicClass = SchematicClass;
				}
				else
				{
					Context.AddWarning(
						FString::Printf(TEXT("Node class %s has no discoverable 'Schematic' field for 'schematic:' — ")
											TEXT("use 'properties:' with the exact path instead"),
										*NodeClass->GetName()),
						Node.Line);
				}
			}
		}

		if (Work.NodeDataPtr != nullptr)
		{
			if (const FKDFNode* CoordinateNode = Node.Find(TEXT("coordinate")))
			{
				const FStructProperty* CoordinateMember =
					CastField<FStructProperty>(FindStructMember(Work.NodeDataStruct->Struct, TEXT("Coordinates")));
				if (CoordinateMember != nullptr)
				{
					void* CoordinatePtr = CoordinateMember->ContainerPtrToValuePtr<void>(Work.NodeDataPtr);
					const int32 X = static_cast<int32>(CoordinateNode->GetInt(TEXT("X"), 0));
					const int32 Y = static_cast<int32>(CoordinateNode->GetInt(TEXT("Y"), 0));
					if (WriteGridPoint(CoordinateMember, CoordinatePtr, X, Y))
					{
						Work.Coordinate = FKDFGridPoint{X, Y};
						Work.bHasCoordinate = true;
					}
				}
				else
				{
					Context.AddWarning(FString::Printf(TEXT("Node class %s has no discoverable 'Coordinates' field"),
													   *NodeClass->GetName()),
									   CoordinateNode->Line);
				}
			}
		}

		if (Work.SchematicClass != nullptr && Work.bHasCoordinate)
		{
			CoordinateBySchematic.Add(Work.SchematicClass, Work.Coordinate);
		}
	}

	// --- Phase 2: `parents:` / `nodesToUnhide:` / `unhiddenBy:`, and (manual mode) `childrenAndRoads:`.
	TMultiMap<FKDFGridPoint, FKDFGridPoint> AutoEdgesByParent; // only populated when bAutoPath
	for (FKDFResearchNodeWork& Work : WorkingNodes)
	{
		if (Work.NodeDataStruct == nullptr || Work.NodeDataPtr == nullptr)
		{
			continue;
		}

		TArray<FKDFGridPoint> ResolvedParents;
		ResolveAndWriteGridArray(Work, TEXT("parents"), TEXT("Parents"), CoordinateBySchematic, Subsystem, Context,
								 &ResolvedParents);
		ResolveAndWriteGridArray(Work, TEXT("nodesToUnhide"), TEXT("NodesToUnhide"), CoordinateBySchematic, Subsystem,
								 Context);
		ResolveAndWriteGridArray(Work, TEXT("unhiddenBy"), TEXT("UnhiddenBy"), CoordinateBySchematic, Subsystem,
								 Context);

		if (!bAutoPath)
		{
			if (const FKDFNode* RoadsNode = Work.SourceNode->Find(TEXT("childrenAndRoads")))
			{
				WriteChildrenAndRoadsFromYaml(*Work.NodeDataStruct, Work.NodeDataPtr, *RoadsNode, CoordinateBySchematic,
											  Subsystem, Context);
			}
		}
		else if (Work.bHasCoordinate)
		{
			for (const FKDFGridPoint& ParentPoint : ResolvedParents)
			{
				AutoEdgesByParent.Add(ParentPoint, Work.Coordinate);
			}
		}
	}

	// --- Phase 3 (autoPath only): invert every recorded parent edge into that parent's own
	// ChildrenAndRoads as a straight two-point connector. No equivalent road-layout utility exists
	// in KBFL/KPrivateCodeLib to call into instead — this is a new, intentionally simple default;
	// use a manual `childrenAndRoads:` (autoPath: false) for anything more elaborate.
	if (bAutoPath)
	{
		for (FKDFResearchNodeWork& Work : WorkingNodes)
		{
			if (!Work.bHasCoordinate || Work.NodeDataStruct == nullptr)
			{
				continue;
			}
			TArray<FKDFGridPoint> Children;
			AutoEdgesByParent.MultiFind(Work.Coordinate, Children);
			if (Children.Num() == 0)
			{
				continue;
			}
			Children.Sort([](const FKDFGridPoint& A, const FKDFGridPoint& B)
						  { return A.X != B.X ? A.X < B.X : A.Y < B.Y; });

			const FMapProperty* RoadsMember =
				CastField<FMapProperty>(FindStructMember(Work.NodeDataStruct->Struct, TEXT("ChildrenAndRoads")));
			if (RoadsMember == nullptr)
			{
				Context.AddWarning(FString::Printf(
					TEXT("Node class %s has no discoverable 'ChildrenAndRoads' field — autoPath had nothing to write"),
					*Work.NodeClass->GetName()));
				continue;
			}
			TArray<TPair<FKDFGridPoint, TArray<FKDFGridPoint>>> Roads;
			for (const FKDFGridPoint& Child : Children)
			{
				TArray<FKDFGridPoint> RoadPoints;
				RoadPoints.Add(Work.Coordinate);
				RoadPoints.Add(Child);
				Roads.Add(TPair<FKDFGridPoint, TArray<FKDFGridPoint>>(Child, RoadPoints));
			}
			WriteChildrenAndRoadsMap(RoadsMember, Work.NodeDataPtr, Roads);
		}
	}

	// --- Phase 4: `properties:`, append to mNodes, patch-record bookkeeping.
	bool bAddedAny = false;
	for (FKDFResearchNodeWork& Work : WorkingNodes)
	{
		if (const FKDFNode* Properties = Work.SourceNode->Find(TEXT("properties")))
		{
			FKDFPatchUtil::ApplyOpsToObject(Work.Instance, *Properties, false, Context);
		}

		FScriptArrayHelper Helper(NodesArray, NodesArray->ContainerPtrToValuePtr<void>(TreeCDO));
		const int32 NewIndex = Helper.AddValue();
		NodeInner->SetObjectPropertyValue(Helper.GetRawPtr(NewIndex), Work.Instance);
		bAddedAny = true;
		++Context.mAppliedOpCount;
		if (Context.mPatchRecord != nullptr)
		{
			FKDFOpRecord& OpRecord = Context.mPatchRecord->mOps.AddDefaulted_GetRef();
			OpRecord.mTargetObjectPath = TreeCDO->GetPathName();
			OpRecord.mPropertyPath = TEXT("mNodes");
			OpRecord.mOp = EKDFOp::Append;
			OpRecord.mValueText = Work.NodeClass->GetPathName();
		}
	}
	return bAddedAny;
}

bool UKDFContentClassHandler::ApplyEntry(const FKDFNode& Entry, FKDFApplyContext& Context)
{
	UKDFSubsystem* Subsystem = UKDFSubsystem::Get(Context.mGameInstance);
	if (Subsystem == nullptr || !Entry.IsMap())
	{
		return false;
	}
	FString Error;

	const FString ExistingClassPath = Entry.GetString(TEXT("class"), FString());
	const FString Id = Entry.GetString(TEXT("id"), FString());

	UObject* TargetCDO = nullptr;
	UClass* ContentClass = nullptr;

	if (!ExistingClassPath.IsEmpty())
	{
		// Register-only mode (recipe/schematic/research): existing class, optional property tweaks.
		ContentClass = Subsystem->FindOrLoadClassCached(ExistingClassPath, Error);
		if (ContentClass == nullptr)
		{
			Context.AddError(FString::Printf(TEXT("class: %s"), *Error), Entry.Line);
			return false;
		}
		TargetCDO = Subsystem->GetAndRetainCDO(ContentClass);
	}
	else
	{
		const FString ParentPath = Entry.GetString(TEXT("parent"), mDefaultParentPath);
		UClass* ParentClass = Subsystem->FindOrLoadClassCached(ParentPath, Error);
		if (ParentClass == nullptr)
		{
			Context.AddError(FString::Printf(TEXT("parent: %s"), *Error), Entry.Line);
			return false;
		}
		if (!mRequiredBaseClassPath.IsEmpty())
		{
			UClass* RequiredBase = Subsystem->FindOrLoadClassCached(mRequiredBaseClassPath, Error);
			if (RequiredBase != nullptr && !ParentClass->IsChildOf(RequiredBase))
			{
				Context.AddError(FString::Printf(TEXT("parent %s is not a %s"), *ParentClass->GetPathName(),
												 *mRequiredBaseClassPath),
								 Entry.Line);
				return false;
			}
		}
		if (Context.bDryRun)
		{
			return true; // class generation is not dry-runnable — validation stops here
		}
		ContentClass = Subsystem->GetDynamicContent().GetOrCreateClass(Context.mPackRef, Id, ParentClass, Error);
		if (ContentClass == nullptr)
		{
			Context.AddError(Error, Entry.Line);
			return false;
		}
		TargetCDO = Subsystem->GetAndRetainCDO(ContentClass);
		if (Context.mPatchRecord != nullptr)
		{
			FKDFOpRecord& OpRecord = Context.mPatchRecord->mOps.AddDefaulted_GetRef();
			OpRecord.mTargetObjectPath = ContentClass->GetPathName();
			OpRecord.mPropertyPath = TEXT("<generated>");
			OpRecord.mOp = EKDFOp::Set;
			OpRecord.mValueText = ParentClass->GetPathName();
		}
	}

	if (const FKDFNode* Properties = Entry.Find(TEXT("properties")))
	{
		FKDFPatchUtil::ApplyOpsToObject(TargetCDO, *Properties, false, Context);
	}
	if (bSupportsUnlocks)
	{
		if (const FKDFNode* Unlocks = Entry.Find(TEXT("unlocks")))
		{
			ApplyInstancedObjects(TargetCDO, *Unlocks, TEXT("mUnlocks"),
								  TEXT("Schematic class has no mUnlocks object array"), Context);
		}
	}
	if (bSupportsDependencies)
	{
		if (const FKDFNode* Dependencies = Entry.Find(TEXT("dependencies")))
		{
			ApplyInstancedObjects(TargetCDO, *Dependencies, TEXT("mSchematicDependencies"),
								  TEXT("Schematic class has no mSchematicDependencies object array"), Context);
		}
	}
	if (bSupportsNodes)
	{
		if (const FKDFNode* Nodes = Entry.Find(TEXT("nodes")))
		{
			ApplyNodes(TargetCDO, *Nodes, Entry.GetBool(TEXT("autoPath"), false), Context);
		}
	}

	EKDFContentRegKind RegistrationKind = mRegistrationKind;
	if (bAllowPerEntryRegisterAs)
	{
		const FString RegisterAs = Entry.GetString(TEXT("registerAs"), FString()).ToLower();
		if (RegisterAs == TEXT("recipe"))
		{
			RegistrationKind = EKDFContentRegKind::Recipe;
		}
		else if (RegisterAs == TEXT("schematic"))
		{
			RegistrationKind = EKDFContentRegKind::Schematic;
		}
		else if (RegisterAs == TEXT("research"))
		{
			RegistrationKind = EKDFContentRegKind::ResearchTree;
		}
		else if (!RegisterAs.IsEmpty())
		{
			Context.AddWarning(
				FString::Printf(TEXT("Unknown registerAs '%s' — expected recipe|schematic|research"), *RegisterAs),
				Entry.Line);
		}
	}
	if (RegistrationKind != EKDFContentRegKind::None && Entry.GetBool(TEXT("register"), true) && !Context.bDryRun)
	{
		Subsystem->QueueContentRegistration(RegistrationKind, ContentClass, Context.mSourceFile);
	}
	return true;
}

bool UKDFContentClassHandler::ApplyDocument(const FKDFNode& Document, FKDFApplyContext& Context)
{
	const FKDFNode* Entries = Document.Find(mEntriesKey);
	if (Entries == nullptr || !Entries->IsSequence())
	{
		return false;
	}
	bool bAppliedAny = false;
	for (const TSharedRef<FKDFNode>& Entry : Entries->Sequence)
	{
		bAppliedAny |= ApplyEntry(Entry.Get(), Context);
	}
	return bAppliedAny;
}

// --- Concrete document types.
// All class GENERATION runs in the Items stage (priority-ordered) so later stages (Recipes,
// Schematics, Research) can reference the generated classes. Registration-capable types keep
// their own stages — they only reference earlier content.

UKDFClassHandler::UKDFClassHandler()
{
	mRootType = TEXT("class");
	mStage = EKDFStage::Items;
	mPriority = 50; // before all preset kinds — user-defined bases first
	mEntriesKey = TEXT("classes");
	mDefaultParentPath = FString(); // `parent:` is mandatory — ANY class works
	mRequiredBaseClassPath = FString(); // no base restriction by design
	bAllowPerEntryRegisterAs = true;
}

UKDFItemHandler::UKDFItemHandler()
{
	mRootType = TEXT("item");
	mStage = EKDFStage::Items;
	mPriority = 30;
	mEntriesKey = TEXT("items");
	mDefaultParentPath = TEXT("/Script/FactoryGame.FGItemDescriptor");
	mRequiredBaseClassPath = TEXT("/Script/FactoryGame.FGItemDescriptor");
}

UKDFResourceHandler::UKDFResourceHandler()
{
	mRootType = TEXT("resource");
	mStage = EKDFStage::Items;
	mPriority = 25;
	mEntriesKey = TEXT("resources");
	mDefaultParentPath = TEXT("/Script/FactoryGame.FGResourceDescriptor");
	mRequiredBaseClassPath = TEXT("/Script/FactoryGame.FGResourceDescriptor");
}

UKDFBuildingHandler::UKDFBuildingHandler()
{
	mRootType = TEXT("building");
	mStage = EKDFStage::Items;
	mPriority = 20;
	mEntriesKey = TEXT("buildings");
	mDefaultParentPath = TEXT("/Script/FactoryGame.FGBuildingDescriptor");
	mRequiredBaseClassPath = TEXT("/Script/FactoryGame.FGBuildingDescriptor");
}

UKDFUnlockHandler::UKDFUnlockHandler()
{
	mRootType = TEXT("unlock");
	mStage = EKDFStage::Items;
	mPriority = 40; // before items — schematics may reference generated unlock classes
	mEntriesKey = TEXT("unlocks");
	mDefaultParentPath = FString(); // no sensible default — a concrete FGUnlock subclass is required
	mRequiredBaseClassPath = TEXT("/Script/FactoryGame.FGUnlock");
}

UKDFRecipeHandler::UKDFRecipeHandler()
{
	mRootType = TEXT("recipe");
	mStage = EKDFStage::Recipes;
	mEntriesKey = TEXT("recipes");
	mDefaultParentPath = TEXT("/Script/FactoryGame.FGRecipe");
	mRequiredBaseClassPath = TEXT("/Script/FactoryGame.FGRecipe");
	mRegistrationKind = EKDFContentRegKind::Recipe;
}

UKDFSchematicHandler::UKDFSchematicHandler()
{
	mRootType = TEXT("schematic");
	mStage = EKDFStage::Schematics;
	mEntriesKey = TEXT("schematics");
	mDefaultParentPath = TEXT("/Script/FactoryGame.FGSchematic");
	mRequiredBaseClassPath = TEXT("/Script/FactoryGame.FGSchematic");
	mRegistrationKind = EKDFContentRegKind::Schematic;
	bSupportsUnlocks = true;
	bSupportsDependencies = true;
}

UKDFResearchHandler::UKDFResearchHandler()
{
	mRootType = TEXT("research");
	mStage = EKDFStage::Research;
	mEntriesKey = TEXT("research");
	mDefaultParentPath = TEXT("/Script/FactoryGame.FGResearchTree");
	mRequiredBaseClassPath = TEXT("/Script/FactoryGame.FGResearchTree");
	mRegistrationKind = EKDFContentRegKind::ResearchTree;
	bSupportsNodes = true;
}
