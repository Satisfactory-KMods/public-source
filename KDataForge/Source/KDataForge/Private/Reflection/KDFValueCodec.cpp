#include "Reflection/KDFValueCodec.h"

#include "Content/KDFDynamicContent.h"
#include "Internationalization/TextKey.h"
#include "KDFLogging.h"
#include "Misc/OutputDeviceNull.h"
#include "Reflection/KDFPropertyPath.h"
#include "UObject/Class.h"
#include "UObject/TextProperty.h"
#include "UObject/UObjectHash.h"

namespace
{
	FKDFDynamicContentRegistry* GDynamicContentRegistry = nullptr;
	thread_local FString GPackScopeRef;

	bool IsValidEnumImportValue(const UEnum* Enum, const int64 Value)
	{
#if WITH_METADATA
		if (!Enum->HasMetaData(TEXT("Bitflags")))
		{
			return Enum->IsValidEnumValue(Value);
		}
		if (Value < 0)
		{
			return false;
		}

		const bool bValuesAreMasks = Enum->HasMetaData(TEXT("UseEnumValuesAsMaskValuesInEditor"));
		uint64 AllowedMask = 0;
		for (int32 Index = 0; Index < Enum->NumEnums(); ++Index)
		{
			if (Enum->GetNameStringByIndex(Index).EndsWith(TEXT("_MAX")))
			{
				continue;
			}
			const int64 Enumerator = Enum->GetValueByIndex(Index);
			if (Enumerator < 0)
			{
				continue;
			}
			if (bValuesAreMasks)
			{
				AllowedMask |= static_cast<uint64>(Enumerator);
			}
			else if (Enumerator < 64)
			{
				AllowedMask |= uint64{1} << Enumerator;
			}
		}
		return (static_cast<uint64>(Value) & ~AllowedMask) == 0;
#else
		return Enum->IsValidEnumValue(Value);
#endif
	}

	/** Logs and picks the deterministic first match when a bare generated-content id is ambiguous. */
	template <typename T>
	T* PickFirstGeneratedMatch(const FString& Id, const TArray<T*>& Matches, const TCHAR* Kind)
	{
		if (Matches.IsEmpty())
		{
			return nullptr;
		}
		if (Matches.Num() > 1)
		{
			TArray<FString> MatchPaths;
			for (const T* Match : Matches)
			{
				MatchPaths.Add(Match->GetPathName());
			}
			UE_LOG(LogKDataForge, Error,
				   TEXT("Generated-content id '%s' matched %d %s across packs (%s) — applying to the first match "
						"'%s'. Qualify with the full /KDataForge/Gen/<PackRef> path to disambiguate."),
				   *Id, Matches.Num(), Kind, *FString::Join(MatchPaths, TEXT(", ")), *MatchPaths[0]);
		}
		return Matches[0]; // FindGeneratedClassesById/FindGeneratedAssetsById already sort by path
	}

	/** Bare generated-content class id: the FPackScope-current pack first, then every pack. */
	UClass* ResolveGeneratedClassById(const FString& Id)
	{
		if (GDynamicContentRegistry == nullptr)
		{
			return nullptr;
		}
		if (!GPackScopeRef.IsEmpty())
		{
			if (UClass* Exact = GDynamicContentRegistry->FindGeneratedClass(GPackScopeRef, Id))
			{
				return Exact;
			}
		}
		TArray<UClass*> Matches;
		GDynamicContentRegistry->FindGeneratedClassesById(Id, Matches);
		return PickFirstGeneratedMatch(Id, Matches, TEXT("classes"));
	}

	/** Bare generated-content asset id (`dataasset` / `asset` instances): same scoping as classes. */
	UObject* ResolveGeneratedAssetById(const FString& Id)
	{
		if (GDynamicContentRegistry == nullptr)
		{
			return nullptr;
		}
		if (!GPackScopeRef.IsEmpty())
		{
			if (UObject* Exact = GDynamicContentRegistry->FindGeneratedAsset(GPackScopeRef, Id))
			{
				return Exact;
			}
		}
		TArray<UObject*> Matches;
		GDynamicContentRegistry->FindGeneratedAssetsById(Id, Matches);
		return PickFirstGeneratedMatch(Id, Matches, TEXT("assets"));
	}

	bool NodeToStructValue(const FKDFNode& Node, const FStructProperty* StructProperty, void* ValuePtr,
						   FString& OutError)
	{
		UScriptStruct* Struct = StructProperty->Struct;

		if (Node.IsScalar())
		{
			// Native text form, e.g. FVector "(X=1,Y=2,Z=3)", FGameplayTag "A.B", FLinearColor, FGuid …
			FOutputDeviceNull Errors;
			if (StructProperty->ImportText_Direct(*Node.Scalar, ValuePtr, nullptr, PPF_None, &Errors) == nullptr)
			{
				OutError = FString::Printf(TEXT("Could not parse '%s' as %s"), *Node.Scalar, *Struct->GetName());
				return false;
			}
			return true;
		}

		if (Node.IsMap())
		{
			for (const TTuple<FString, TSharedRef<FKDFNode>>& Pair : Node.Map)
			{
				FProperty* Member = FKDFPropertyResolver::FindPropertyByNameFlexible(Struct, Pair.Key);
				if (Member == nullptr)
				{
					OutError = FString::Printf(TEXT("Struct %s has no member '%s'"), *Struct->GetName(), *Pair.Key);
					return false;
				}
				if (!FKDFValueCodec::NodeToProperty(Pair.Value.Get(), Member,
													Member->ContainerPtrToValuePtr<void>(ValuePtr), OutError))
				{
					return false;
				}
			}
			return true;
		}

		if (Node.IsSequence())
		{
			// Positional form: assign sequence elements to struct members in declaration order ([x, y, z] → FVector).
			int32 ElementIndex = 0;
			for (TFieldIterator<FProperty> It(Struct); It; ++It)
			{
				if (ElementIndex >= Node.Sequence.Num())
				{
					break;
				}
				if (!FKDFValueCodec::NodeToProperty(Node.Sequence[ElementIndex].Get(), *It,
													It->ContainerPtrToValuePtr<void>(ValuePtr), OutError))
				{
					return false;
				}
				++ElementIndex;
			}
			if (ElementIndex < Node.Sequence.Num())
			{
				OutError = FString::Printf(TEXT("Too many elements (%d) for struct %s"), Node.Sequence.Num(),
										   *Struct->GetName());
				return false;
			}
			return true;
		}

		OutError = FString::Printf(TEXT("Cannot convert null to struct %s"), *Struct->GetName());
		return false;
	}

	bool NodeToText(const FKDFNode& Node, const FTextProperty* TextProperty, void* ValuePtr, FString& OutError)
	{
		if (Node.IsMap())
		{
			const FString Key = Node.GetString(TEXT("key"), FString());
			const FString Namespace = Node.GetString(TEXT("ns"), FString());
			const FString Source = Node.GetString(TEXT("source"), FString());
			if (Key.IsEmpty())
			{
				OutError = TEXT("FText map form requires a 'key' entry ({ key, ns, source })");
				return false;
			}
			// Runtime-safe keyed text creation (FText::ChangeKey is editor-only and breaks Shipping).
			const FText Value = FText::AsLocalizable_Advanced(FTextKey(Namespace), FTextKey(Key), FString(Source));
			TextProperty->SetPropertyValue(ValuePtr, Value);
			return true;
		}
		if (Node.IsScalar())
		{
			TextProperty->SetPropertyValue(ValuePtr, FText::FromString(Node.Scalar));
			return true;
		}
		if (Node.IsNull())
		{
			TextProperty->SetPropertyValue(ValuePtr, FText::GetEmpty());
			return true;
		}
		OutError = TEXT("Cannot convert sequence to FText");
		return false;
	}

	/**
	 * Container properties (array/set/map) also accept a scalar carrying the property's own native
	 * ExportText form (e.g. `((ItemClass=...,Amount=2))`) — the same "whole value" round-trip used
	 * for `ArrayAddElement` and undo/redo. Structs already support this dual form via
	 * NodeToStructValue; this mirrors it for containers instead of hard-requiring a sequence/map.
	 */
	bool TryImportNativeText(const FKDFNode& Node, const FProperty* Property, void* ValuePtr)
	{
		if (!Node.IsScalar())
		{
			return false;
		}
		FOutputDeviceNull Errors;
		return Property->ImportText_Direct(*Node.Scalar, ValuePtr, nullptr, PPF_None, &Errors) != nullptr;
	}

	/**
	 * Finds every currently loaded class whose object name matches `ShortName` (with `_C` appended if
	 * missing), case-insensitive. Only searches already-loaded classes — this mirrors engine's own
	 * `FindFirstObject` shortcut resolution rather than forcing new packages off disk.
	 */
	void FindClassesByShortName(const FString& ShortName, TArray<UClass*>& OutClasses)
	{
		const FString SearchName = ShortName.EndsWith(TEXT("_C")) ? ShortName : ShortName + TEXT("_C");
		TArray<UClass*> AllClasses;
		GetDerivedClasses(UObject::StaticClass(), AllClasses, true);
		for (UClass* Candidate : AllClasses)
		{
			if (IsValid(Candidate) && Candidate->GetName().Equals(SearchName, ESearchCase::IgnoreCase))
			{
				OutClasses.Add(Candidate);
			}
		}
		OutClasses.Sort([](const UClass& A, const UClass& B) { return A.GetPathName() < B.GetPathName(); });
	}
} // namespace

namespace
{
	bool NodeToPropertyInternal(const FKDFNode& Node, const FProperty* Property, void* ValuePtr, FString& OutError)
	{
		if (const FBoolProperty* BoolProperty = CastField<FBoolProperty>(Property))
		{
			bool bValue = false;
			if (!Node.TryGetBool(bValue))
			{
				OutError = FString::Printf(TEXT("Expected bool, got '%s'"), *Node.GetString());
				return false;
			}
			BoolProperty->SetPropertyValue(ValuePtr, bValue);
			return true;
		}

		if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
		{
			const UEnum* Enum = EnumProperty->GetEnum();
			int64 Value = 0;
			if (!Node.TryGetInt(Value))
			{
				Value = Enum->GetValueByNameString(Node.GetString(), EGetByNameFlags::CheckAuthoredName);
				if (Value == INDEX_NONE)
				{
					OutError =
						FString::Printf(TEXT("'%s' is not a value of enum %s"), *Node.GetString(), *Enum->GetName());
					return false;
				}
			}
			else if (!IsValidEnumImportValue(Enum, Value))
			{
				// Silent invalid-enum writes bite later at use sites — reject with a name hint instead.
				OutError = FString::Printf(TEXT("%lld is not a valid value of enum %s (use a name, e.g. %s)"), Value,
										   *Enum->GetName(), *Enum->GetNameStringByIndex(0));
				return false;
			}
			EnumProperty->GetUnderlyingProperty()->SetIntPropertyValue(ValuePtr, Value);
			return true;
		}

		if (const FNumericProperty* NumericProperty = CastField<FNumericProperty>(Property))
		{
			if (const UEnum* Enum = NumericProperty->GetIntPropertyEnum())
			{
				int64 Value = 0;
				if (!Node.TryGetInt(Value))
				{
					Value = Enum->GetValueByNameString(Node.GetString(), EGetByNameFlags::CheckAuthoredName);
					if (Value == INDEX_NONE)
					{
						OutError = FString::Printf(TEXT("'%s' is not a value of enum %s"), *Node.GetString(),
												   *Enum->GetName());
						return false;
					}
				}
				if (!IsValidEnumImportValue(Enum, Value))
				{
					OutError = FString::Printf(TEXT("%lld is not a valid value of enum %s (use a name, e.g. %s)"),
											   Value, *Enum->GetName(), *Enum->GetNameStringByIndex(0));
					return false;
				}
				NumericProperty->SetIntPropertyValue(ValuePtr, Value);
				return true;
			}
			if (NumericProperty->IsFloatingPoint())
			{
				double Value = 0.0;
				if (!Node.TryGetFloat(Value))
				{
					OutError = FString::Printf(TEXT("Expected number, got '%s'"), *Node.GetString());
					return false;
				}
				if (!FMath::IsFinite(Value) || !NumericProperty->CanHoldValue(Value))
				{
					OutError = FString::Printf(TEXT("Number '%s' is out of range for %s"), *Node.GetString(),
											   *NumericProperty->GetCPPType());
					return false;
				}
				NumericProperty->SetFloatingPointPropertyValue(ValuePtr, Value);
				return true;
			}
			int64 Value = 0;
			if (!Node.TryGetInt(Value))
			{
				OutError = FString::Printf(TEXT("Expected integer, got '%s'"), *Node.GetString());
				return false;
			}
			if (!NumericProperty->CanHoldValue(Value))
			{
				OutError = FString::Printf(TEXT("Integer '%s' is out of range for %s"), *Node.GetString(),
										   *NumericProperty->GetCPPType());
				return false;
			}
			NumericProperty->SetIntPropertyValue(ValuePtr, Value);
			return true;
		}

		if (const FStrProperty* StrProperty = CastField<FStrProperty>(Property))
		{
			StrProperty->SetPropertyValue(ValuePtr, Node.GetString());
			return true;
		}
		if (const FNameProperty* NameProperty = CastField<FNameProperty>(Property))
		{
			NameProperty->SetPropertyValue(ValuePtr, FName(*Node.GetString()));
			return true;
		}
		if (const FTextProperty* TextProperty = CastField<FTextProperty>(Property))
		{
			return NodeToText(Node, TextProperty, ValuePtr, OutError);
		}

		if (const FSoftClassProperty* SoftClassProperty = CastField<FSoftClassProperty>(Property))
		{
			const FString Path = Node.GetString();
			if (Path.IsEmpty() || Path.Equals(TEXT("none"), ESearchCase::IgnoreCase))
			{
				SoftClassProperty->SetPropertyValue(ValuePtr, FSoftObjectPtr());
				return true;
			}
			const FSoftObjectPath SoftPath(Path);
			if (!SoftPath.IsValid())
			{
				OutError = FString::Printf(TEXT("'%s' is not a valid soft class path"), *Path);
				return false;
			}
			if (UObject* LoadedObject = SoftPath.ResolveObject())
			{
				const UClass* LoadedClass = Cast<UClass>(LoadedObject);
				if (LoadedClass == nullptr ||
					(SoftClassProperty->MetaClass != nullptr && !LoadedClass->IsChildOf(SoftClassProperty->MetaClass)))
				{
					OutError = FString::Printf(TEXT("Loaded class '%s' is not a subclass of %s"), *Path,
											   SoftClassProperty->MetaClass != nullptr
												   ? *SoftClassProperty->MetaClass->GetName()
												   : TEXT("UObject"));
					return false;
				}
			}
			SoftClassProperty->SetPropertyValue(ValuePtr, FSoftObjectPtr(SoftPath));
			return true;
		}

		if (const FSoftObjectProperty* SoftObjectProperty = CastField<FSoftObjectProperty>(Property))
		{
			const FString Path = Node.GetString();
			if (Path.IsEmpty() || Path.Equals(TEXT("none"), ESearchCase::IgnoreCase))
			{
				SoftObjectProperty->SetPropertyValue(ValuePtr, FSoftObjectPtr());
			}
			else
			{
				SoftObjectProperty->SetPropertyValue(ValuePtr, FSoftObjectPtr(FSoftObjectPath(Path)));
			}
			return true;
		}

		if (const FClassProperty* ClassProperty = CastField<FClassProperty>(Property))
		{
			const FString Path = Node.GetString();
			if (Path.IsEmpty() || Path.Equals(TEXT("none"), ESearchCase::IgnoreCase))
			{
				ClassProperty->SetObjectPropertyValue(ValuePtr, nullptr);
				return true;
			}
			UClass* Class = FKDFValueCodec::ResolveClass(Path, OutError);
			if (Class == nullptr)
			{
				return false;
			}
			if (ClassProperty->MetaClass != nullptr && !Class->IsChildOf(ClassProperty->MetaClass))
			{
				OutError = FString::Printf(TEXT("Class %s is not a subclass of %s"), *Class->GetPathName(),
										   *ClassProperty->MetaClass->GetName());
				return false;
			}
			ClassProperty->SetObjectPropertyValue(ValuePtr, Class);
			return true;
		}

		if (const FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Property))
		{
			const FString Path = Node.GetString();
			if (Path.IsEmpty() || Path.Equals(TEXT("none"), ESearchCase::IgnoreCase))
			{
				ObjectProperty->SetObjectPropertyValue(ValuePtr, nullptr);
				return true;
			}
			UObject* Object = FKDFValueCodec::ResolveObject(Path, ObjectProperty, OutError);
			if (Object == nullptr)
			{
				return false;
			}
			ObjectProperty->SetObjectPropertyValue(ValuePtr, Object);
			return true;
		}

		if (const FStructProperty* StructProperty = CastField<FStructProperty>(Property))
		{
			return NodeToStructValue(Node, StructProperty, ValuePtr, OutError);
		}

		if (const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
		{
			if (!Node.IsSequence())
			{
				if (TryImportNativeText(Node, Property, ValuePtr))
				{
					return true;
				}
				OutError = TEXT("Expected sequence for array property");
				return false;
			}
			FScriptArrayHelper Helper(ArrayProperty, ValuePtr);
			Helper.EmptyValues();
			for (const TSharedRef<FKDFNode>& Element : Node.Sequence)
			{
				const int32 NewIndex = Helper.AddValue();
				if (!FKDFValueCodec::NodeToProperty(Element.Get(), ArrayProperty->Inner, Helper.GetRawPtr(NewIndex),
													OutError))
				{
					return false;
				}
			}
			return true;
		}
		if (const FSetProperty* SetProperty = CastField<FSetProperty>(Property))
		{
			if (!Node.IsSequence())
			{
				if (TryImportNativeText(Node, Property, ValuePtr))
				{
					return true;
				}
				OutError = TEXT("Expected sequence for set property");
				return false;
			}
			FScriptSetHelper Helper(SetProperty, ValuePtr);
			Helper.EmptyElements();
			for (const TSharedRef<FKDFNode>& Element : Node.Sequence)
			{
				const int32 NewIndex = Helper.AddDefaultValue_Invalid_NeedsRehash();
				if (!FKDFValueCodec::NodeToProperty(Element.Get(), SetProperty->ElementProp,
													Helper.GetElementPtr(NewIndex), OutError))
				{
					Helper.Rehash();
					return false;
				}
			}
			Helper.Rehash();
			return true;
		}
		if (const FMapProperty* MapProperty = CastField<FMapProperty>(Property))
		{
			if (!Node.IsMap())
			{
				if (TryImportNativeText(Node, Property, ValuePtr))
				{
					return true;
				}
				OutError = TEXT("Expected map for map property");
				return false;
			}
			FScriptMapHelper Helper(MapProperty, ValuePtr);
			Helper.EmptyValues();
			for (const TTuple<FString, TSharedRef<FKDFNode>>& Pair : Node.Map)
			{
				const int32 NewIndex = Helper.AddDefaultValue_Invalid_NeedsRehash();
				const FKDFNode KeyNode = *FKDFNode::MakeScalar(Pair.Key, true);
				if (!FKDFValueCodec::NodeToProperty(KeyNode, MapProperty->KeyProp, Helper.GetKeyPtr(NewIndex),
													OutError) ||
					!FKDFValueCodec::NodeToProperty(Pair.Value.Get(), MapProperty->ValueProp,
													Helper.GetValuePtr(NewIndex), OutError))
				{
					Helper.Rehash();
					return false;
				}
			}
			Helper.Rehash();
			return true;
		}

		if (Node.IsScalar())
		{
			FOutputDeviceNull Errors;
			if (Property->ImportText_Direct(*Node.Scalar, ValuePtr, nullptr, PPF_None, &Errors) != nullptr)
			{
				return true;
			}
		}
		OutError =
			FString::Printf(TEXT("Unsupported conversion to property type %s"), *Property->GetClass()->GetName());
		return false;
	}
} // namespace

bool FKDFValueCodec::NodeToProperty(const FKDFNode& Node, const FProperty* Property, void* ValuePtr, FString& OutError)
{
	if (Property == nullptr || ValuePtr == nullptr)
	{
		OutError = TEXT("Cannot import into a null property or value");
		return false;
	}

	void* TemporaryValue = FMemory::Malloc(Property->GetSize(), Property->GetMinAlignment());
	Property->InitializeValue(TemporaryValue);
	Property->CopyCompleteValue(TemporaryValue, ValuePtr);
	const bool bImported = NodeToPropertyInternal(Node, Property, TemporaryValue, OutError);
	if (bImported)
	{
		Property->CopyCompleteValue(ValuePtr, TemporaryValue);
	}
	Property->DestroyValue(TemporaryValue);
	FMemory::Free(TemporaryValue);
	return bImported;
}

bool FKDFValueCodec::PropertyToNode(const FProperty* Property, const void* ValuePtr, FKDFNode& OutNode)
{
	if (const FBoolProperty* BoolProperty = CastField<FBoolProperty>(Property))
	{
		OutNode = *FKDFNode::MakeScalar(BoolProperty->GetPropertyValue(ValuePtr) ? TEXT("true") : TEXT("false"));
		return true;
	}
	if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
	{
		const int64 Value = EnumProperty->GetUnderlyingProperty()->GetSignedIntPropertyValue(ValuePtr);
		OutNode = *FKDFNode::MakeScalar(EnumProperty->GetEnum()->GetNameStringByValue(Value), true);
		return true;
	}
	if (const FNumericProperty* NumericProperty = CastField<FNumericProperty>(Property))
	{
		if (const UEnum* Enum = NumericProperty->GetIntPropertyEnum())
		{
			const int64 Value = NumericProperty->GetSignedIntPropertyValue(ValuePtr);
			OutNode = *FKDFNode::MakeScalar(Enum->GetNameStringByValue(Value), true);
			return true;
		}
		OutNode = *FKDFNode::MakeScalar(NumericProperty->GetNumericPropertyValueToString(ValuePtr));
		return true;
	}
	if (const FStrProperty* StrProperty = CastField<FStrProperty>(Property))
	{
		OutNode = *FKDFNode::MakeScalar(StrProperty->GetPropertyValue(ValuePtr), true);
		return true;
	}
	if (const FNameProperty* NameProperty = CastField<FNameProperty>(Property))
	{
		OutNode = *FKDFNode::MakeScalar(NameProperty->GetPropertyValue(ValuePtr).ToString(), true);
		return true;
	}
	if (const FTextProperty* TextProperty = CastField<FTextProperty>(Property))
	{
		const FText& Text = TextProperty->GetPropertyValue(ValuePtr);
		const TOptional<FString> Key = FTextInspector::GetKey(Text);
		if (Key.IsSet())
		{
			const TSharedRef<FKDFNode> MapNode = FKDFNode::MakeMap();
			MapNode->SetChild(TEXT("key"), FKDFNode::MakeScalar(Key.GetValue(), true));
			MapNode->SetChild(TEXT("ns"),
							  FKDFNode::MakeScalar(FTextInspector::GetNamespace(Text).Get(FString()), true));
			const FString* Source = FTextInspector::GetSourceString(Text);
			MapNode->SetChild(TEXT("source"), FKDFNode::MakeScalar(Source ? *Source : Text.ToString(), true));
			OutNode = *MapNode;
			return true;
		}
		OutNode = *FKDFNode::MakeScalar(Text.ToString(), true);
		return true;
	}
	if (const FSoftObjectProperty* SoftObjectProperty = CastField<FSoftObjectProperty>(Property))
	{
		OutNode = *FKDFNode::MakeScalar(SoftObjectProperty->GetPropertyValue(ValuePtr).ToString(), true);
		return true;
	}
	if (const FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Property))
	{
		const UObject* Object = ObjectProperty->GetObjectPropertyValue(ValuePtr);
		if (ObjectProperty->HasAnyPropertyFlags(CPF_InstancedReference | CPF_PersistentInstance) && Object != nullptr)
		{
			const TSharedRef<FKDFNode> ObjectNode = FKDFNode::MakeMap();
			ObjectNode->SetChild(TEXT("class"), FKDFNode::MakeScalar(Object->GetClass()->GetPathName(), true));
			const TSharedRef<FKDFNode> Properties = FKDFNode::MakeSequence();
			for (TFieldIterator<FProperty> It(Object->GetClass()); It; ++It)
			{
				const FProperty* ChildProperty = *It;
				if (!ChildProperty->HasAnyPropertyFlags(CPF_Edit) ||
					ChildProperty->HasAnyPropertyFlags(CPF_Deprecated | CPF_Transient | CPF_EditorOnly))
				{
					continue;
				}
				FKDFNode ChildNode;
				if (!PropertyToNode(ChildProperty, ChildProperty->ContainerPtrToValuePtr<void>(Object), ChildNode))
				{
					continue;
				}
				const TSharedRef<FKDFNode> Entry = FKDFNode::MakeMap();
				Entry->SetChild(TEXT("path"), FKDFNode::MakeScalar(ChildProperty->GetAuthoredName(), false));
				Entry->SetChild(TEXT("value"), MakeShared<FKDFNode>(MoveTemp(ChildNode)));
				Properties->AddChild(Entry);
			}
			ObjectNode->SetChild(TEXT("properties"), Properties);
			OutNode = *ObjectNode;
			return true;
		}
		OutNode = *FKDFNode::MakeScalar(Object ? Object->GetPathName() : FString(), true);
		return true;
	}
	if (const FStructProperty* StructProperty = CastField<FStructProperty>(Property))
	{
		UScriptStruct* Struct = StructProperty->Struct;
		const UScriptStruct::ICppStructOps* StructOps = Struct->GetCppStructOps();
		if (StructOps != nullptr && StructOps->HasExportTextItem())
		{
			// Compact native text form (FGameplayTag, FGuid, …).
			OutNode = *FKDFNode::MakeScalar(ExportText(Property, ValuePtr), true);
			return true;
		}
		const TSharedRef<FKDFNode> MapNode = FKDFNode::MakeMap();
		for (TFieldIterator<FProperty> It(Struct); It; ++It)
		{
			FKDFNode MemberNode;
			if (PropertyToNode(*It, It->ContainerPtrToValuePtr<void>(ValuePtr), MemberNode))
			{
				MapNode->SetChild(It->GetAuthoredName(), MakeShared<FKDFNode>(MoveTemp(MemberNode)));
			}
		}
		OutNode = *MapNode;
		return true;
	}
	if (const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
	{
		FScriptArrayHelper Helper(ArrayProperty, ValuePtr);
		const TSharedRef<FKDFNode> SequenceNode = FKDFNode::MakeSequence();
		for (int32 Index = 0; Index < Helper.Num(); ++Index)
		{
			FKDFNode ElementNode;
			if (PropertyToNode(ArrayProperty->Inner, Helper.GetRawPtr(Index), ElementNode))
			{
				SequenceNode->AddChild(MakeShared<FKDFNode>(MoveTemp(ElementNode)));
			}
		}
		OutNode = *SequenceNode;
		return true;
	}
	if (const FSetProperty* SetProperty = CastField<FSetProperty>(Property))
	{
		FScriptSetHelper Helper(SetProperty, ValuePtr);
		const TSharedRef<FKDFNode> SequenceNode = FKDFNode::MakeSequence();
		for (int32 Sparse = 0; Sparse < Helper.GetMaxIndex(); ++Sparse)
		{
			if (!Helper.IsValidIndex(Sparse))
			{
				continue;
			}
			FKDFNode ElementNode;
			if (PropertyToNode(SetProperty->ElementProp, Helper.GetElementPtr(Sparse), ElementNode))
			{
				SequenceNode->AddChild(MakeShared<FKDFNode>(MoveTemp(ElementNode)));
			}
		}
		OutNode = *SequenceNode;
		return true;
	}
	if (const FMapProperty* MapProperty = CastField<FMapProperty>(Property))
	{
		FScriptMapHelper Helper(MapProperty, ValuePtr);
		const TSharedRef<FKDFNode> MapNode = FKDFNode::MakeMap();
		for (int32 Sparse = 0; Sparse < Helper.GetMaxIndex(); ++Sparse)
		{
			if (!Helper.IsValidIndex(Sparse))
			{
				continue;
			}
			FKDFNode ValueNode;
			if (PropertyToNode(MapProperty->ValueProp, Helper.GetValuePtr(Sparse), ValueNode))
			{
				MapNode->SetChild(ExportText(MapProperty->KeyProp, Helper.GetKeyPtr(Sparse)),
								  MakeShared<FKDFNode>(MoveTemp(ValueNode)));
			}
		}
		OutNode = *MapNode;
		return true;
	}

	// Fallback: UE text export.
	OutNode = *FKDFNode::MakeScalar(ExportText(Property, ValuePtr), true);
	return true;
}

FString FKDFValueCodec::ExportText(const FProperty* Property, const void* ValuePtr)
{
	FString Result;
	Property->ExportTextItem_Direct(Result, ValuePtr, nullptr, nullptr, PPF_None);
	return Result;
}

bool FKDFValueCodec::ImportText(const FString& Text, const FProperty* Property, void* ValuePtr)
{
	FOutputDeviceNull Errors;
	return Property->ImportText_Direct(*Text, ValuePtr, nullptr, PPF_None, &Errors) != nullptr;
}

bool FKDFValueCodec::ValuesEqual(const FProperty* Property, const void* A, const void* B)
{
	return Property->Identical(A, B, PPF_None);
}

void FKDFValueCodec::SetDynamicContentRegistry(FKDFDynamicContentRegistry* Registry)
{
	GDynamicContentRegistry = Registry;
}

FKDFValueCodec::FPackScope::FPackScope(const FString& PackRef) : mPrevious(GPackScopeRef) { GPackScopeRef = PackRef; }

FKDFValueCodec::FPackScope::~FPackScope() { GPackScopeRef = mPrevious; }

FString FKDFValueCodec::GetCurrentPackScope() { return GPackScopeRef; }

UClass* FKDFValueCodec::ResolveClass(const FString& Path, FString& OutError)
{
	if (UClass* Found = Cast<UClass>(FSoftObjectPath(Path).TryLoad()))
	{
		return Found;
	}
	// Blueprint class paths are often written without the generated-class suffix — retry with `_C`.
	if (!Path.EndsWith(TEXT("_C")))
	{
		const FString WithSuffix = Path + TEXT("_C");
		if (UClass* Found = Cast<UClass>(FSoftObjectPath(WithSuffix).TryLoad()))
		{
			return Found;
		}
		// Asset path form `/Game/Foo/Bar` → object path `/Game/Foo/Bar.Bar_C`.
		FString PackageName = Path;
		FString AssetName;
		if (!Path.Contains(TEXT(".")) &&
			Path.Split(TEXT("/"), &PackageName, &AssetName, ESearchCase::CaseSensitive, ESearchDir::FromEnd))
		{
			const FString ObjectPath = FString::Printf(TEXT("%s.%s_C"), *Path, *AssetName);
			if (UClass* Found = Cast<UClass>(FSoftObjectPath(ObjectPath).TryLoad()))
			{
				return Found;
			}
		}
	}
	// Bare generated-content id / class-name shortcut. Only tried once the full-path forms above have
	// failed, and only for name-only input (no path separators at all).
	if (!Path.Contains(TEXT("/")) && !Path.Contains(TEXT(".")))
	{
		// e.g. "MySuperIngot" instead of "/KDataForge/Gen/MyPack.Gen_MyPack_MySuperIngot_C" — the id
		// given to an item/resource/building/class/... entry. Tried first: it's the more specific,
		// more common case for content authored through KDataForge itself.
		if (UClass* Generated = ResolveGeneratedClassById(Path))
		{
			return Generated;
		}
		// e.g. "Desc_Water" instead of "/Game/FactoryGame/Resource/RawResources/Water/Desc_Water.Desc_Water_C".
		TArray<UClass*> Matches;
		FindClassesByShortName(Path, Matches);
		if (!Matches.IsEmpty())
		{
			if (Matches.Num() > 1)
			{
				TArray<FString> MatchPaths;
				for (const UClass* Match : Matches)
				{
					MatchPaths.Add(Match->GetPathName());
				}
				UE_LOG(LogKDataForge, Error,
					   TEXT("Class shortcut '%s' matched %d classes (%s) — applying to the first match '%s'"), *Path,
					   Matches.Num(), *FString::Join(MatchPaths, TEXT(", ")), *MatchPaths[0]);
			}
			return Matches[0];
		}
	}
	OutError = FString::Printf(TEXT("Class not found: '%s'"), *Path);
	return nullptr;
}

UObject* FKDFValueCodec::ResolveObject(const FString& Path, const FObjectPropertyBase* Property, FString& OutError)
{
	UObject* Object = FSoftObjectPath(Path).TryLoad();
	// Bare generated-content asset id, e.g. "SuperIngotIcon" instead of
	// "/KDataForge/GenAssets/MyPack.Gen_MyPack_SuperIngotIcon" — a `dataasset`/`asset` entry's id.
	// Name-only input only, same gate as the class-id shortcut in ResolveClass.
	if (Object == nullptr && !Path.Contains(TEXT("/")) && !Path.Contains(TEXT(".")))
	{
		Object = ResolveGeneratedAssetById(Path);
	}
	if (Object == nullptr)
	{
		// Allow class references in plain object properties (common for descriptor/recipe fields) —
		// also covers a bare generated-content CLASS id (e.g. "MySuperIngot") via ResolveClass.
		FString ClassError;
		Object = ResolveClass(Path, ClassError);
	}
	if (Object == nullptr)
	{
		OutError = FString::Printf(TEXT("Object not found: '%s'"), *Path);
		return nullptr;
	}
	if (Property->PropertyClass != nullptr && !Object->IsA(Property->PropertyClass))
	{
		OutError =
			FString::Printf(TEXT("Object %s is not a %s"), *Object->GetPathName(), *Property->PropertyClass->GetName());
		return nullptr;
	}
	return Object;
}
