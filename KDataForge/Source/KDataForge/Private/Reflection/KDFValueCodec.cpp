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

	const UEnum* ResolveBitmaskEnumMetadata(const FProperty* Property)
	{
#if WITH_METADATA
		if (!Property->HasMetaData(TEXT("Bitmask")))
		{
			return nullptr;
		}
		const FString EnumPath = Property->GetMetaData(TEXT("BitmaskEnum"));
		if (EnumPath.IsEmpty())
		{
			return nullptr;
		}
		return FindObject<UEnum>(nullptr, *EnumPath);
#else
		return nullptr;
#endif
	}

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
		return Matches[0];
	}

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

	bool TryImportNativeText(const FKDFNode& Node, const FProperty* Property, void* ValuePtr)
	{
		if (!Node.IsScalar())
		{
			return false;
		}
		FOutputDeviceNull Errors;
		return Property->ImportText_Direct(*Node.Scalar, ValuePtr, nullptr, PPF_None, &Errors) != nullptr;
	}

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
}

namespace
{

	bool RequireObjectReferenceNode(const FKDFNode& Node, const FProperty* Property, FString& OutError)
	{
		if (Node.IsScalar() || Node.IsNull())
		{
			return true;
		}
		OutError = FString::Printf(TEXT("'%s' expects a scalar object path, got a %s"), *Property->GetAuthoredName(),
								   Node.IsMap() ? TEXT("map") : TEXT("sequence"));
		OutError += TEXT(". Instanced subobjects come from '{class: ..., properties: [...]}' entries, built by ");
		OutError += TEXT("'set'/'append'/'prepend'/'insert' on the property itself");
		return false;
	}

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
			if (const UEnum* BitmaskEnum = ResolveBitmaskEnumMetadata(Property))
			{
				int64 Value = 0;
				if (Node.TryGetInt(Value))
				{

					if (!IsValidEnumImportValue(BitmaskEnum, Value))
					{
						OutError = FString::Printf(TEXT("%lld includes bits outside %s's declared values"), Value,
												   *BitmaskEnum->GetName());
						return false;
					}
				}
				else if (!FKDFValueCodec::ResolveBitmaskFlags(Node, BitmaskEnum, Value, OutError))
				{
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
			if (!RequireObjectReferenceNode(Node, Property, OutError))
			{
				return false;
			}
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
			if (!RequireObjectReferenceNode(Node, Property, OutError))
			{
				return false;
			}
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
			if (!RequireObjectReferenceNode(Node, Property, OutError))
			{
				return false;
			}
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
			if (!RequireObjectReferenceNode(Node, Property, OutError))
			{
				return false;
			}
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
}

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

bool FKDFValueCodec::IsBitmaskProperty(const FProperty* Property)
{
	const FNumericProperty* Numeric = CastField<FNumericProperty>(Property);
	if (Numeric == nullptr || Numeric->IsFloatingPoint() || Numeric->GetIntPropertyEnum() != nullptr)
	{
		return false;
	}
	return ResolveBitmaskEnumMetadata(Property) != nullptr;
}

const UEnum* FKDFValueCodec::GetBitmaskEnum(const FProperty* Property)
{
	const FNumericProperty* Numeric = CastField<FNumericProperty>(Property);
	if (Numeric == nullptr || Numeric->IsFloatingPoint() || Numeric->GetIntPropertyEnum() != nullptr)
	{
		return nullptr;
	}
	return ResolveBitmaskEnumMetadata(Property);
}

bool FKDFValueCodec::ResolveBitmaskFlags(const FKDFNode& Node, const UEnum* Enum, int64& OutValue, FString& OutError)
{
	if (Enum == nullptr)
	{
		OutError = TEXT("No bitmask enum to resolve flag names against");
		return false;
	}
	TArray<FString> FlagNames;
	if (Node.IsScalar())
	{
		Node.Scalar.ParseIntoArray(FlagNames, TEXT("|"), true);
		for (FString& Name : FlagNames)
		{
			Name.TrimStartAndEndInline();
		}
	}
	else if (Node.IsSequence())
	{
		for (const TSharedRef<FKDFNode>& Element : Node.Sequence)
		{
			FlagNames.Add(Element->GetString());
		}
	}
	else
	{
		OutError = TEXT("Expected a flag name, '|'-joined flag names, or a sequence of flag names");
		return false;
	}
	if (FlagNames.IsEmpty())
	{
		OutError = TEXT("No flag names given");
		return false;
	}
	int64 Combined = 0;
	for (const FString& Name : FlagNames)
	{
		const int64 Value = Enum->GetValueByNameString(Name, EGetByNameFlags::CheckAuthoredName);
		if (Value == INDEX_NONE)
		{
			OutError = FString::Printf(TEXT("'%s' is not a value of enum %s"), *Name, *Enum->GetName());
			return false;
		}
		Combined |= Value;
	}
	if (!IsValidEnumImportValue(Enum, Combined))
	{
		OutError = FString::Printf(TEXT("Combined flags (%lld) include bits outside %s's declared values"), Combined,
								   *Enum->GetName());
		return false;
	}
	OutValue = Combined;
	return true;
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
		if (const UEnum* BitmaskEnum = FKDFValueCodec::GetBitmaskEnum(Property))
		{
			const int64 Value = NumericProperty->GetSignedIntPropertyValue(ValuePtr);
			const TSharedRef<FKDFNode> SequenceNode = FKDFNode::MakeSequence();
			for (int32 Index = 0; Index < BitmaskEnum->NumEnums(); ++Index)
			{
				if (BitmaskEnum->GetNameStringByIndex(Index).EndsWith(TEXT("_MAX")))
				{
					continue;
				}
				const int64 Enumerator = BitmaskEnum->GetValueByIndex(Index);
				if (Enumerator != 0 && (Value & Enumerator) == Enumerator)
				{
					SequenceNode->AddChild(FKDFNode::MakeScalar(BitmaskEnum->GetNameStringByIndex(Index), true));
				}
			}
			OutNode = *SequenceNode;
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

	if (!Path.EndsWith(TEXT("_C")))
	{
		const FString WithSuffix = Path + TEXT("_C");
		if (UClass* Found = Cast<UClass>(FSoftObjectPath(WithSuffix).TryLoad()))
		{
			return Found;
		}

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

	if (!Path.Contains(TEXT("/")) && !Path.Contains(TEXT(".")))
	{

		if (UClass* Generated = ResolveGeneratedClassById(Path))
		{
			return Generated;
		}

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

	if (Object == nullptr && !Path.Contains(TEXT("/")) && !Path.Contains(TEXT(".")))
	{
		Object = ResolveGeneratedAssetById(Path);
	}
	if (Object == nullptr)
	{

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
