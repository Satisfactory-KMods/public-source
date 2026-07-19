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

	bool ReadGridPoint(const FStructProperty* PointStruct, const void* PointPtr, FKDFGridPoint& OutPoint)
	{
		if (PointStruct == nullptr || PointStruct->Struct == nullptr || PointPtr == nullptr)
		{
			return false;
		}
		const FIntProperty* XProperty = CastField<FIntProperty>(FindStructMember(PointStruct->Struct, TEXT("X")));
		const FIntProperty* YProperty = CastField<FIntProperty>(FindStructMember(PointStruct->Struct, TEXT("Y")));
		if (XProperty == nullptr || YProperty == nullptr)
		{
			return false;
		}
		OutPoint.X = XProperty->GetPropertyValue_InContainer(PointPtr);
		OutPoint.Y = YProperty->GetPropertyValue_InContainer(PointPtr);
		return true;
	}

	/** Builds vanilla-style road points: adjacent grid cells, excluding the parent and including the child. */
	TArray<FKDFGridPoint> BuildAutoRoadPoints(const FKDFGridPoint& Parent, const FKDFGridPoint& Child)
	{
		TArray<FKDFGridPoint> Points;
		FKDFGridPoint Cursor = Parent;
		while (Cursor.X != Child.X)
		{
			Cursor.X += Cursor.X < Child.X ? 1 : -1;
			Points.Add(Cursor);
		}
		while (Cursor.Y != Child.Y)
		{
			Cursor.Y += Cursor.Y < Child.Y ? 1 : -1;
			Points.Add(Cursor);
		}
		return Points;
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

	/** Preserves existing array entries and appends only coordinates not already present. */
	bool AppendUniqueGridPoints(const FArrayProperty* ArrayProperty, void* ArrayPtr,
								const TArray<FKDFGridPoint>& Points)
	{
		const FStructProperty* ElementStruct =
			ArrayProperty != nullptr ? CastField<FStructProperty>(ArrayProperty->Inner) : nullptr;
		if (ElementStruct == nullptr)
		{
			return false;
		}

		FScriptArrayHelper Helper(ArrayProperty, ArrayPtr);
		TArray<FKDFGridPoint> Existing;
		for (int32 Index = 0; Index < Helper.Num(); ++Index)
		{
			FKDFGridPoint Point;
			if (ReadGridPoint(ElementStruct, Helper.GetRawPtr(Index), Point))
			{
				Existing.Add(Point);
			}
		}
		for (const FKDFGridPoint& Point : Points)
		{
			if (Existing.Contains(Point))
			{
				continue;
			}
			const int32 NewIndex = Helper.AddValue();
			WriteGridPoint(ElementStruct, Helper.GetRawPtr(NewIndex), Point.X, Point.Y);
			Existing.Add(Point);
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
			int32 TargetIndex = INDEX_NONE;
			for (int32 Index = 0; Index < Helper.GetMaxIndex(); ++Index)
			{
				if (!Helper.IsValidIndex(Index))
				{
					continue;
				}
				FKDFGridPoint ExistingKey;
				if (ReadGridPoint(KeyStruct, Helper.GetKeyPtr(Index), ExistingKey) && ExistingKey == Road.Key)
				{
					TargetIndex = Index;
					break;
				}
			}
			if (TargetIndex == INDEX_NONE)
			{
				TargetIndex = Helper.AddDefaultValue_Invalid_NeedsRehash();
				WriteGridPoint(KeyStruct, Helper.GetKeyPtr(TargetIndex), Road.Key.X, Road.Key.Y);
			}
			void* ValuePtr = Helper.GetValuePtr(TargetIndex);
			WriteGridPointArray(PointsMember, PointsMember->ContainerPtrToValuePtr<void>(ValuePtr), Road.Value);
			// Blueprint struct keys use reflected hash/equality. Restore valid map state before next lookup.
			Helper.Rehash();
		}
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
		bool bExisting = false;
		bool bSkipAppend = false;
	};

	/** Adds existing tree nodes to the same schematic→coordinate index used by newly authored nodes. */
	void IndexExistingResearchNodes(UObject* TreeCDO, const FArrayProperty* NodesArray,
									const FObjectProperty* NodeInner,
									TMap<UClass*, FKDFGridPoint>& CoordinateBySchematic,
									TArray<FKDFResearchNodeWork>& WorkingNodes)
	{
		FScriptArrayHelper NodesHelper(NodesArray, NodesArray->ContainerPtrToValuePtr<void>(TreeCDO));
		for (int32 Index = 0; Index < NodesHelper.Num(); ++Index)
		{
			UObject* Instance = NodeInner->GetObjectPropertyValue(NodesHelper.GetRawPtr(Index));
			if (Instance == nullptr)
			{
				continue;
			}
			FKDFResearchNodeWork& Work = WorkingNodes.AddDefaulted_GetRef();
			Work.Instance = Instance;
			Work.NodeClass = Instance->GetClass();
			Work.bExisting = true;
			Work.NodeDataStruct = CastField<FStructProperty>(
				FKDFPropertyResolver::FindPropertyByNameFlexible(Work.NodeClass, TEXT("mNodeDataStruct")));
			Work.NodeDataPtr =
				Work.NodeDataStruct != nullptr ? Work.NodeDataStruct->ContainerPtrToValuePtr<void>(Instance) : nullptr;
			if (Work.NodeDataStruct == nullptr || Work.NodeDataPtr == nullptr)
			{
				continue;
			}
			const FClassProperty* SchematicMember =
				CastField<FClassProperty>(FindStructMember(Work.NodeDataStruct->Struct, TEXT("Schematic")));
			const FStructProperty* CoordinateMember =
				CastField<FStructProperty>(FindStructMember(Work.NodeDataStruct->Struct, TEXT("Coordinates")));
			if (SchematicMember != nullptr && CoordinateMember != nullptr)
			{
				Work.SchematicClass = Cast<UClass>(SchematicMember->GetObjectPropertyValue(Work.NodeDataPtr));
				Work.bHasCoordinate =
					ReadGridPoint(CoordinateMember, CoordinateMember->ContainerPtrToValuePtr<void>(Work.NodeDataPtr),
								  Work.Coordinate);
				if (Work.SchematicClass != nullptr && Work.bHasCoordinate)
				{
					CoordinateBySchematic.Add(Work.SchematicClass, Work.Coordinate);
				}
			}
		}
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
			FKDFPatchUtil::ApplyOpsToObject(EntryInstance, *Properties, Context);
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

bool UKDFContentClassHandler::ApplyClassListShortcutUnlock(UObject* TargetCDO, const FKDFNode& EntriesNode,
														   const TCHAR* UnlockClassPath, const TCHAR* ValueField,
														   const TCHAR* RequiredBaseClassPath,
														   const TCHAR* ShortcutName, FKDFApplyContext& Context)
{
	UKDFSubsystem* Subsystem = UKDFSubsystem::Get(Context.mGameInstance);
	if (!EntriesNode.IsSequence())
	{
		Context.AddError(FString::Printf(TEXT("'%s' must be a sequence"), ShortcutName), EntriesNode.Line);
		return false;
	}

	FString Error;
	UClass* RequiredBaseClass = Subsystem->FindOrLoadClassCached(RequiredBaseClassPath, Error);
	TArray<UClass*> Values;
	for (const TSharedRef<FKDFNode>& EntryRef : EntriesNode.Sequence)
	{
		const FKDFNode& Entry = EntryRef.Get();
		if (!Entry.IsScalar() || Entry.Scalar.IsEmpty())
		{
			Context.AddError(FString::Printf(TEXT("'%s' entries must be class references"), ShortcutName), Entry.Line);
			continue;
		}
		UClass* ValueClass = Subsystem->FindOrLoadClassCached(Entry.Scalar, Error);
		if (ValueClass == nullptr || (RequiredBaseClass != nullptr && !ValueClass->IsChildOf(RequiredBaseClass)))
		{
			Context.AddError(FString::Printf(TEXT("'%s' entry '%s' is not a valid %s class (%s)"), ShortcutName,
											 *Entry.Scalar, RequiredBaseClassPath,
											 ValueClass == nullptr ? *Error : TEXT("wrong base class")),
							 Entry.Line);
			continue;
		}
		Values.AddUnique(ValueClass);
	}
	if (Values.Num() == 0 || Context.bDryRun)
	{
		return Values.Num() > 0;
	}

	const FArrayProperty* UnlocksArray = CastField<FArrayProperty>(
		FKDFPropertyResolver::FindPropertyByNameFlexible(TargetCDO->GetClass(), TEXT("mUnlocks")));
	const FObjectProperty* UnlockInner =
		UnlocksArray != nullptr ? CastField<FObjectProperty>(UnlocksArray->Inner) : nullptr;
	UClass* UnlockClass = Subsystem->FindOrLoadClassCached(UnlockClassPath, Error);
	const FArrayProperty* ValueArray = nullptr;
	if (UnlockInner == nullptr || UnlockClass == nullptr || !UnlockClass->IsChildOf(UnlockInner->PropertyClass) ||
		(ValueArray = CastField<FArrayProperty>(
			 FKDFPropertyResolver::FindPropertyByNameFlexible(UnlockClass, ValueField))) == nullptr ||
		CastField<FClassProperty>(ValueArray->Inner) == nullptr)
	{
		Context.AddError(FString::Printf(TEXT("Cannot create '%s' shortcut unlock (%s)"), ShortcutName, *Error),
						 EntriesNode.Line);
		return false;
	}

	UObject* UnlockInstance = NewObject<UObject>(TargetCDO, UnlockClass);
	const FClassProperty* ValueInner = CastField<FClassProperty>(ValueArray->Inner);
	FScriptArrayHelper ValueHelper(ValueArray, ValueArray->ContainerPtrToValuePtr<void>(UnlockInstance));
	for (UClass* ValueClass : Values)
	{
		const int32 Index = ValueHelper.AddValue();
		ValueInner->SetObjectPropertyValue(ValueHelper.GetRawPtr(Index), ValueClass);
	}
	FScriptArrayHelper UnlockHelper(UnlocksArray, UnlocksArray->ContainerPtrToValuePtr<void>(TargetCDO));
	const int32 UnlockIndex = UnlockHelper.AddValue();
	UnlockInner->SetObjectPropertyValue(UnlockHelper.GetRawPtr(UnlockIndex), UnlockInstance);
	++Context.mAppliedOpCount;
	return true;
}

bool UKDFContentClassHandler::ApplyCountShortcutUnlock(UObject* TargetCDO, const FKDFNode& CountNode,
													   const TCHAR* UnlockClassPath, const TCHAR* ValueField,
													   const TCHAR* ShortcutName, FKDFApplyContext& Context)
{
	int32 Count = 0;
	if (!CountNode.IsScalar() || !LexTryParseString(Count, *CountNode.Scalar) || Count <= 0)
	{
		Context.AddError(FString::Printf(TEXT("'%s' must be a positive integer"), ShortcutName), CountNode.Line);
		return false;
	}
	if (Context.bDryRun)
	{
		return true;
	}

	UKDFSubsystem* Subsystem = UKDFSubsystem::Get(Context.mGameInstance);
	FString Error;
	const FArrayProperty* UnlocksArray = CastField<FArrayProperty>(
		FKDFPropertyResolver::FindPropertyByNameFlexible(TargetCDO->GetClass(), TEXT("mUnlocks")));
	const FObjectProperty* UnlockInner =
		UnlocksArray != nullptr ? CastField<FObjectProperty>(UnlocksArray->Inner) : nullptr;
	UClass* UnlockClass = Subsystem->FindOrLoadClassCached(UnlockClassPath, Error);
	const FIntProperty* CountProperty = UnlockClass != nullptr
		? CastField<FIntProperty>(FKDFPropertyResolver::FindPropertyByNameFlexible(UnlockClass, ValueField))
		: nullptr;
	if (UnlockInner == nullptr || UnlockClass == nullptr || !UnlockClass->IsChildOf(UnlockInner->PropertyClass) ||
		CountProperty == nullptr)
	{
		Context.AddError(FString::Printf(TEXT("Cannot create '%s' shortcut unlock (%s)"), ShortcutName, *Error),
						 CountNode.Line);
		return false;
	}

	UObject* UnlockInstance = NewObject<UObject>(TargetCDO, UnlockClass);
	CountProperty->SetPropertyValue_InContainer(UnlockInstance, Count);
	FScriptArrayHelper UnlockHelper(UnlocksArray, UnlocksArray->ContainerPtrToValuePtr<void>(TargetCDO));
	const int32 UnlockIndex = UnlockHelper.AddValue();
	UnlockInner->SetObjectPropertyValue(UnlockHelper.GetRawPtr(UnlockIndex), UnlockInstance);
	++Context.mAppliedOpCount;
	return true;
}

bool UKDFContentClassHandler::ApplyScannableResourcesShortcutUnlock(UObject* TargetCDO, const FKDFNode& EntriesNode,
																	FKDFApplyContext& Context)
{
	if (!EntriesNode.IsSequence())
	{
		Context.AddError(TEXT("'scannableResources' must be a sequence"), EntriesNode.Line);
		return false;
	}

	// Reuse the generic instanced-unlock path after translating the compact public form into the
	// engine's exact FScannableResourcePair property shape. This keeps class resolution and all value
	// conversion (including vanilla, mod, and generated resources) identical to ordinary unlocks.
	TSharedRef<FKDFNode> Pairs = FKDFNode::MakeSequence();
	for (const TSharedRef<FKDFNode>& EntryRef : EntriesNode.Sequence)
	{
		const FKDFNode& Entry = EntryRef.Get();
		TSharedPtr<FKDFNode> Resource = Entry.FindShared(TEXT("resource"));
		if (!Entry.IsMap() || !Resource.IsValid() || !Resource->IsScalar())
		{
			Context.AddError(TEXT("'scannableResources' entries need a scalar 'resource' class reference"), Entry.Line);
			continue;
		}
		TSharedRef<FKDFNode> Pair = FKDFNode::MakeMap();
		Pair->SetChild(TEXT("ResourceDescriptor"), Resource.ToSharedRef());
		TSharedPtr<FKDFNode> NodeType = Entry.FindShared(TEXT("nodeType"));
		Pair->SetChild(TEXT("ResourceNodeType"),
					   NodeType.IsValid() ? NodeType.ToSharedRef() : FKDFNode::MakeScalar(TEXT("Node")));
		Pairs->AddChild(Pair);
	}
	if (Pairs->Num() == 0)
	{
		return false;
	}

	TSharedRef<FKDFNode> Property = FKDFNode::MakeMap();
	Property->SetChild(TEXT("path"), FKDFNode::MakeScalar(TEXT("mResourcePairsToAddToScanner")));
	Property->SetChild(TEXT("value"), Pairs);
	TSharedRef<FKDFNode> Properties = FKDFNode::MakeSequence();
	Properties->AddChild(Property);
	TSharedRef<FKDFNode> Unlock = FKDFNode::MakeMap();
	Unlock->SetChild(TEXT("class"), FKDFNode::MakeScalar(TEXT("/Script/FactoryGame.FGUnlockScannableResource")));
	Unlock->SetChild(TEXT("properties"), Properties);
	TSharedRef<FKDFNode> Unlocks = FKDFNode::MakeSequence();
	Unlocks->AddChild(Unlock);
	return ApplyInstancedObjects(TargetCDO, Unlocks.Get(), TEXT("mUnlocks"),
								 TEXT("Schematic class has no mUnlocks object array"), Context);
}

bool UKDFContentClassHandler::ApplyNodes(UObject* TreeCDO, const FKDFNode& NodesNode, bool bAutoPath,
										 FKDFApplyContext& Context)
{
	UKDFSubsystem* Subsystem = UKDFSubsystem::Get(Context.mGameInstance);
	if (!NodesNode.IsSequence())
	{
		Context.AddError(TEXT("Research node additions must be a sequence ('addNodes' or legacy 'nodes')"));
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
	IndexExistingResearchNodes(TreeCDO, NodesArray, NodeInner, CoordinateBySchematic, WorkingNodes);
	TSet<UClass*> ClaimedSchematics;
	TSet<FKDFGridPoint> ClaimedCoordinates;
	for (const FKDFResearchNodeWork& ExistingWork : WorkingNodes)
	{
		if (ExistingWork.SchematicClass != nullptr)
		{
			ClaimedSchematics.Add(ExistingWork.SchematicClass);
		}
		if (ExistingWork.bHasCoordinate)
		{
			ClaimedCoordinates.Add(ExistingWork.Coordinate);
		}
	}
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
		if (Work.NodeDataPtr == nullptr)
		{
			Context.AddError(FString::Printf(TEXT("Node class %s has no mNodeDataStruct"), *NodeClass->GetPathName()),
							 Node.Line);
			Work.bSkipAppend = true;
		}

		const FString SchematicPath = Node.GetString(TEXT("schematic"), FString());
		if (SchematicPath.IsEmpty())
		{
			Context.AddError(TEXT("Node entry requires a 'schematic' class reference"), Node.Line);
			Work.bSkipAppend = true;
		}
		else
		{
			UClass* SchematicClass = Subsystem->FindOrLoadClassCached(SchematicPath, Error);
			if (SchematicClass == nullptr ||
				(SchematicBaseClass != nullptr && !SchematicClass->IsChildOf(SchematicBaseClass)))
			{
				Context.AddError(FString::Printf(TEXT("Node 'schematic' is invalid (%s)"),
												 SchematicClass == nullptr ? *Error : TEXT("not a schematic class")),
								 Node.Line);
				Work.bSkipAppend = true;
			}
			else if (ClaimedSchematics.Contains(SchematicClass))
			{
				Context.AddError(FString::Printf(TEXT("Research tree already contains a node for schematic %s"),
												 *SchematicClass->GetPathName()),
								 Node.Line);
				Work.bSkipAppend = true;
			}
			else if (Work.NodeDataPtr != nullptr)
			{
				FClassProperty* SchematicMember =
					CastField<FClassProperty>(FindStructMember(Work.NodeDataStruct->Struct, TEXT("Schematic")));
				if (SchematicMember != nullptr)
				{
					SchematicMember->SetObjectPropertyValue(Work.NodeDataPtr, SchematicClass);
					Work.SchematicClass = SchematicClass;
					ClaimedSchematics.Add(SchematicClass);
				}
				else
				{
					Context.AddError(FString::Printf(TEXT("Node class %s has no discoverable 'Schematic' field"),
													 *NodeClass->GetName()),
									 Node.Line);
					Work.bSkipAppend = true;
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
			if (ClaimedCoordinates.Contains(Work.Coordinate))
			{
				Context.AddError(FString::Printf(TEXT("Research tree coordinate {%d,%d} is already occupied"),
												 Work.Coordinate.X, Work.Coordinate.Y),
								 Node.Line);
				Work.bSkipAppend = true;
				ClaimedSchematics.Remove(Work.SchematicClass);
			}
			else
			{
				ClaimedCoordinates.Add(Work.Coordinate);
				CoordinateBySchematic.Add(Work.SchematicClass, Work.Coordinate);
			}
		}
	}

	// --- Phase 2: `parents:` / `nodesToUnhide:` / `unhiddenBy:`, and (manual mode) `childrenAndRoads:`.
	TMultiMap<FKDFGridPoint, FKDFGridPoint> AutoEdgesByParent; // only populated when bAutoPath
	TMultiMap<FKDFGridPoint, FKDFGridPoint> AutoUnhideEdgesByTarget;
	for (FKDFResearchNodeWork& Work : WorkingNodes)
	{
		if (Work.bExisting || Work.bSkipAppend || Work.SourceNode == nullptr || Work.NodeDataStruct == nullptr ||
			Work.NodeDataPtr == nullptr)
		{
			continue;
		}

		TArray<FKDFGridPoint> ResolvedParents;
		ResolveAndWriteGridArray(Work, TEXT("parents"), TEXT("Parents"), CoordinateBySchematic, Subsystem, Context,
								 &ResolvedParents);
		ResolveAndWriteGridArray(Work, TEXT("nodesToUnhide"), TEXT("NodesToUnhide"), CoordinateBySchematic, Subsystem,
								 Context);
		TArray<FKDFGridPoint> ResolvedUnhiddenBy;
		ResolveAndWriteGridArray(Work, TEXT("unhiddenBy"), TEXT("UnhiddenBy"), CoordinateBySchematic, Subsystem,
								 Context, &ResolvedUnhiddenBy);
		if (Work.bHasCoordinate)
		{
			for (const FKDFGridPoint& TargetPoint : ResolvedUnhiddenBy)
			{
				AutoUnhideEdgesByTarget.Add(TargetPoint, Work.Coordinate);
			}
		}

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

	// Both inverse reveal fields drive locked-node hover highlighting. Preserve target entries while
	// adding every child that names the target through `unhiddenBy:`.
	for (FKDFResearchNodeWork& Work : WorkingNodes)
	{
		if (Work.bSkipAppend || !Work.bHasCoordinate || Work.NodeDataStruct == nullptr || Work.NodeDataPtr == nullptr)
		{
			continue;
		}
		TArray<FKDFGridPoint> NodesToUnhide;
		AutoUnhideEdgesByTarget.MultiFind(Work.Coordinate, NodesToUnhide);
		if (NodesToUnhide.Num() == 0)
		{
			continue;
		}
		NodesToUnhide.Sort([](const FKDFGridPoint& A, const FKDFGridPoint& B)
						   { return A.X != B.X ? A.X < B.X : A.Y < B.Y; });
		const FArrayProperty* NodesToUnhideMember =
			CastField<FArrayProperty>(FindStructMember(Work.NodeDataStruct->Struct, TEXT("NodesToUnhide")));
		if (NodesToUnhideMember != nullptr)
		{
			AppendUniqueGridPoints(NodesToUnhideMember,
				NodesToUnhideMember->ContainerPtrToValuePtr<void>(Work.NodeDataPtr), NodesToUnhide);
		}
	}

	// --- Phase 3 (autoPath only): invert every recorded parent edge into that parent's own
	// ChildrenAndRoads as an orthogonal, one-grid-step path. No equivalent road-layout utility exists
	// in KBFL/KPrivateCodeLib to call into instead — this is a new, intentionally simple default;
	// use a manual `childrenAndRoads:` (autoPath: false) for anything more elaborate.
	if (bAutoPath)
	{
		for (FKDFResearchNodeWork& Work : WorkingNodes)
		{
			if (Work.bSkipAppend || !Work.bHasCoordinate || Work.NodeDataStruct == nullptr)
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
				Roads.Add(TPair<FKDFGridPoint, TArray<FKDFGridPoint>>(
					Child, BuildAutoRoadPoints(Work.Coordinate, Child)));
			}
			WriteChildrenAndRoadsMap(RoadsMember, Work.NodeDataPtr, Roads);
		}
	}

	// --- Phase 4: `properties:`, append to mNodes, patch-record bookkeeping.
	bool bAddedAny = false;
	for (FKDFResearchNodeWork& Work : WorkingNodes)
	{
		if (Work.bExisting || Work.bSkipAppend || Work.SourceNode == nullptr)
		{
			continue;
		}
		if (const FKDFNode* Properties = Work.SourceNode->Find(TEXT("properties")))
		{
			FKDFPatchUtil::ApplyOpsToObject(Work.Instance, *Properties, Context);
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

bool UKDFContentClassHandler::RemoveNodesBySchematic(UObject* TreeCDO, const FKDFNode& EntriesNode,
													 FKDFApplyContext& Context)
{
	UKDFSubsystem* Subsystem = UKDFSubsystem::Get(Context.mGameInstance);
	if (!EntriesNode.IsSequence())
	{
		Context.AddError(TEXT("'removeNodes' must be a sequence"), EntriesNode.Line);
		return false;
	}

	TSet<UClass*> SchematicsToRemove;
	FString Error;
	for (const TSharedRef<FKDFNode>& EntryRef : EntriesNode.Sequence)
	{
		const FKDFNode& Entry = EntryRef.Get();
		const FString Reference = Entry.IsScalar() ? Entry.Scalar : Entry.GetString(TEXT("schematic"), FString());
		UClass* SchematicClass = Reference.IsEmpty() ? nullptr : Subsystem->FindOrLoadClassCached(Reference, Error);
		if (SchematicClass == nullptr)
		{
			Context.AddError(FString::Printf(TEXT("'removeNodes' entry needs a valid schematic class (%s)"), *Error),
							 Entry.Line);
			continue;
		}
		SchematicsToRemove.Add(SchematicClass);
	}
	if (SchematicsToRemove.Num() == 0 || Context.bDryRun)
	{
		return SchematicsToRemove.Num() > 0;
	}

	const FArrayProperty* NodesArray = CastField<FArrayProperty>(
		FKDFPropertyResolver::FindPropertyByNameFlexible(TreeCDO->GetClass(), TEXT("mNodes")));
	const FObjectProperty* NodeInner = NodesArray != nullptr ? CastField<FObjectProperty>(NodesArray->Inner) : nullptr;
	if (NodeInner == nullptr)
	{
		Context.AddError(TEXT("Research tree class has no mNodes object array"), EntriesNode.Line);
		return false;
	}

	TSet<FKDFGridPoint> CoordinatesToRemove;
	FScriptArrayHelper NodesHelper(NodesArray, NodesArray->ContainerPtrToValuePtr<void>(TreeCDO));
	bool bSnapshotRecorded = false;
	int32 RemovedNodeCount = 0;
	for (int32 Index = NodesHelper.Num() - 1; Index >= 0; --Index)
	{
		UObject* Node = NodeInner->GetObjectPropertyValue(NodesHelper.GetRawPtr(Index));
		const FStructProperty* Data = Node != nullptr
			? CastField<FStructProperty>(
				  FKDFPropertyResolver::FindPropertyByNameFlexible(Node->GetClass(), TEXT("mNodeDataStruct")))
			: nullptr;
		void* DataPtr = Data != nullptr ? Data->ContainerPtrToValuePtr<void>(Node) : nullptr;
		const FClassProperty* Schematic =
			Data != nullptr ? CastField<FClassProperty>(FindStructMember(Data->Struct, TEXT("Schematic"))) : nullptr;
		const FStructProperty* Coordinate =
			Data != nullptr ? CastField<FStructProperty>(FindStructMember(Data->Struct, TEXT("Coordinates"))) : nullptr;
		UClass* SchematicClass =
			Schematic != nullptr ? Cast<UClass>(Schematic->GetObjectPropertyValue(DataPtr)) : nullptr;
		if (SchematicClass == nullptr || !SchematicsToRemove.Contains(SchematicClass))
		{
			continue;
		}
		FKDFGridPoint Point;
		if (Coordinate != nullptr &&
			ReadGridPoint(Coordinate, Coordinate->ContainerPtrToValuePtr<void>(DataPtr), Point))
		{
			CoordinatesToRemove.Add(Point);
		}
		if (!bSnapshotRecorded)
		{
			Subsystem->GetVanillaCache().RecordSnapshot(
				TreeCDO, TEXT("mNodes"),
				FKDFValueCodec::ExportText(NodesArray, NodesArray->ContainerPtrToValuePtr<void>(TreeCDO)));
			bSnapshotRecorded = true;
		}
		NodesHelper.RemoveValues(Index, 1);
		++RemovedNodeCount;
		++Context.mAppliedOpCount;
		if (Context.mPatchRecord != nullptr)
		{
			FKDFOpRecord& OpRecord = Context.mPatchRecord->mOps.AddDefaulted_GetRef();
			OpRecord.mTargetObjectPath = TreeCDO->GetPathName();
			OpRecord.mPropertyPath = TEXT("mNodes");
			OpRecord.mOp = EKDFOp::Remove;
			OpRecord.mValueText = SchematicClass->GetPathName();
		}
	}

	// Every surviving node is purged of prerequisite/reveal links and outgoing roads to the removed points.
	for (int32 Index = 0; Index < NodesHelper.Num(); ++Index)
	{
		UObject* Node = NodeInner->GetObjectPropertyValue(NodesHelper.GetRawPtr(Index));
		const FStructProperty* Data = Node != nullptr
			? CastField<FStructProperty>(
				  FKDFPropertyResolver::FindPropertyByNameFlexible(Node->GetClass(), TEXT("mNodeDataStruct")))
			: nullptr;
		void* DataPtr = Data != nullptr ? Data->ContainerPtrToValuePtr<void>(Node) : nullptr;
		if (Data == nullptr || DataPtr == nullptr)
		{
			continue;
		}
		for (const TCHAR* FieldName : {TEXT("Parents"), TEXT("NodesToUnhide"), TEXT("UnhiddenBy")})
		{
			const FArrayProperty* Field = CastField<FArrayProperty>(FindStructMember(Data->Struct, FieldName));
			const FStructProperty* PointType = Field != nullptr ? CastField<FStructProperty>(Field->Inner) : nullptr;
			if (PointType == nullptr)
			{
				continue;
			}
			FScriptArrayHelper FieldHelper(Field, Field->ContainerPtrToValuePtr<void>(DataPtr));
			for (int32 PointIndex = FieldHelper.Num() - 1; PointIndex >= 0; --PointIndex)
			{
				FKDFGridPoint Point;
				if (ReadGridPoint(PointType, FieldHelper.GetRawPtr(PointIndex), Point) &&
					CoordinatesToRemove.Contains(Point))
				{
					FieldHelper.RemoveValues(PointIndex, 1);
				}
			}
		}
		const FMapProperty* Roads = CastField<FMapProperty>(FindStructMember(Data->Struct, TEXT("ChildrenAndRoads")));
		const FStructProperty* KeyType = Roads != nullptr ? CastField<FStructProperty>(Roads->KeyProp) : nullptr;
		if (KeyType != nullptr)
		{
			FScriptMapHelper RoadsHelper(Roads, Roads->ContainerPtrToValuePtr<void>(DataPtr));
			for (int32 RoadIndex = RoadsHelper.GetMaxIndex() - 1; RoadIndex >= 0; --RoadIndex)
			{
				if (!RoadsHelper.IsValidIndex(RoadIndex))
				{
					continue;
				}
				FKDFGridPoint Point;
				if (ReadGridPoint(KeyType, RoadsHelper.GetKeyPtr(RoadIndex), Point) &&
					CoordinatesToRemove.Contains(Point))
				{
					RoadsHelper.RemoveAt(RoadIndex);
				}
			}
			RoadsHelper.Rehash();
		}
	}
	return RemovedNodeCount > 0;
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
		FKDFPatchUtil::ApplyOpsToObject(TargetCDO, *Properties, Context);
	}
	if (bSupportsUnlocks)
	{
		if (const FKDFNode* Unlocks = Entry.Find(TEXT("unlocks")))
		{
			ApplyInstancedObjects(TargetCDO, *Unlocks, TEXT("mUnlocks"),
								  TEXT("Schematic class has no mUnlocks object array"), Context);
		}
		if (const FKDFNode* Recipes = Entry.Find(TEXT("recipes")))
		{
			ApplyClassListShortcutUnlock(TargetCDO, *Recipes, TEXT("/Script/FactoryGame.FGUnlockRecipe"),
										 TEXT("mRecipes"), TEXT("/Script/FactoryGame.FGRecipe"), TEXT("recipes"),
										 Context);
		}
		if (const FKDFNode* Schematics = Entry.Find(TEXT("schematics")))
		{
			ApplyClassListShortcutUnlock(TargetCDO, *Schematics, TEXT("/Script/FactoryGame.FGUnlockSchematic"),
										 TEXT("mSchematics"), TEXT("/Script/FactoryGame.FGSchematic"),
										 TEXT("schematics"), Context);
		}
		if (const FKDFNode* HandSlots = Entry.Find(TEXT("handSlots")))
		{
			ApplyCountShortcutUnlock(TargetCDO, *HandSlots, TEXT("/Script/FactoryGame.FGUnlockArmEquipmentSlot"),
									 TEXT("mNumArmEquipmentSlotsToUnlock"), TEXT("handSlots"), Context);
		}
		if (const FKDFNode* InventorySlots = Entry.Find(TEXT("inventorySlots")))
		{
			ApplyCountShortcutUnlock(TargetCDO, *InventorySlots, TEXT("/Script/FactoryGame.FGUnlockInventorySlot"),
									 TEXT("mNumInventorySlotsToUnlock"), TEXT("inventorySlots"), Context);
		}
		if (const FKDFNode* Resources = Entry.Find(TEXT("scannableResources")))
		{
			ApplyScannableResourcesShortcutUnlock(TargetCDO, *Resources, Context);
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
		if (const FKDFNode* RemoveNodes = Entry.Find(TEXT("removeNodes")))
		{
			RemoveNodesBySchematic(TargetCDO, *RemoveNodes, Context);
		}
		const FKDFNode* AddNodes = Entry.Find(TEXT("addNodes"));
		const FKDFNode* LegacyNodes = Entry.Find(TEXT("nodes"));
		if (AddNodes != nullptr)
		{
			ApplyNodes(TargetCDO, *AddNodes, Entry.GetBool(TEXT("autoPath"), false), Context);
		}
		if (LegacyNodes != nullptr)
		{
			if (AddNodes != nullptr)
			{
				Context.AddWarning(TEXT("Both 'addNodes' and legacy 'nodes' are present; ignoring 'nodes'"),
								   LegacyNodes->Line);
			}
			else
			{
				ApplyNodes(TargetCDO, *LegacyNodes, Entry.GetBool(TEXT("autoPath"), false), Context);
			}
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
