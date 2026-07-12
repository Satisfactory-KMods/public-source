#include "Reflection/KDFOpEngine.h"

#include "GameplayTagContainer.h"
#include "KDFNode.h"
#include "Reflection/KDFValueCodec.h"
#include "UObject/TextProperty.h"

namespace
{
	class FPropertyValueStorage
	{
	public:
		explicit FPropertyValueStorage(const FProperty* Property) : mProperty(Property)
		{
			mStorage = FMemory::Malloc(Property->GetSize(), Property->GetMinAlignment());
			mProperty->InitializeValue(mStorage);
		}

		~FPropertyValueStorage()
		{
			mProperty->DestroyValue(mStorage);
			FMemory::Free(mStorage);
		}

		void* GetData() { return mStorage; }

	private:
		const FProperty* mProperty = nullptr;
		void* mStorage = nullptr;
	};

	bool IsTagContainerProperty(const FProperty* Property)
	{
		const FStructProperty* StructProperty = CastField<FStructProperty>(Property);
		return StructProperty != nullptr && StructProperty->Struct == FGameplayTagContainer::StaticStruct();
	}

	bool IsPlainNumeric(const FProperty* Property)
	{
		const FNumericProperty* Numeric = CastField<FNumericProperty>(Property);
		return Numeric != nullptr && Numeric->GetIntPropertyEnum() == nullptr;
	}

	bool IsUnsignedNumeric(const FNumericProperty* Property)
	{
		return Property->IsA<FByteProperty>() || Property->IsA<FUInt16Property>() || Property->IsA<FUInt32Property>() ||
			Property->IsA<FUInt64Property>();
	}

	bool IsStringLike(const FProperty* Property)
	{
		return Property->IsA<FStrProperty>() || Property->IsA<FNameProperty>() || Property->IsA<FTextProperty>();
	}

	bool TryReadNumericNode(const FKDFNode* Node, double& OutValue, FString& OutError)
	{
		if (Node == nullptr || !Node->TryGetFloat(OutValue))
		{
			OutError = FString::Printf(TEXT("Expected numeric value, got '%s'"),
									   Node ? *Node->GetString() : TEXT("<missing>"));
			return false;
		}
		return true;
	}

	bool CollectTags(const FKDFNode* Node, TArray<FGameplayTag>& OutTags, FString& OutError)
	{
		if (Node == nullptr)
		{
			OutError = TEXT("Missing 'value' for gameplay tag operation");
			return false;
		}
		TArray<FString> TagNames;
		if (Node->IsScalar())
		{
			TagNames.Add(Node->Scalar);
		}
		else if (Node->IsSequence())
		{
			for (const TSharedRef<FKDFNode>& Element : Node->Sequence)
			{
				TagNames.Add(Element->GetString());
			}
		}
		else
		{
			OutError = TEXT("Expected tag name or sequence of tag names");
			return false;
		}
		for (const FString& TagName : TagNames)
		{
			const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*TagName), false);
			if (!Tag.IsValid())
			{
				OutError = FString::Printf(TEXT("Unknown gameplay tag '%s' (declare it in a gametag document first)"),
										   *TagName);
				return false;
			}
			OutTags.Add(Tag);
		}
		return true;
	}

	bool ApplyTagContainerOp(EKDFOp Op, void* ValuePtr, const FKDFOpArgs& Args, FString& OutError)
	{
		FGameplayTagContainer& Container = *static_cast<FGameplayTagContainer*>(ValuePtr);
		switch (Op)
		{
		case EKDFOp::Clear:
			Container.Reset();
			return true;
		case EKDFOp::Set:
			{
				TArray<FGameplayTag> Tags;
				if (!CollectTags(Args.mValue, Tags, OutError))
				{
					return false;
				}
				Container.Reset();
				for (const FGameplayTag& Tag : Tags)
				{
					Container.AddTag(Tag);
				}
				return true;
			}
		case EKDFOp::Append:
		case EKDFOp::Prepend: // order is irrelevant for tag containers
			{
				TArray<FGameplayTag> Tags;
				if (!CollectTags(Args.mValue, Tags, OutError))
				{
					return false;
				}
				for (const FGameplayTag& Tag : Tags)
				{
					Container.AddTag(Tag);
				}
				return true;
			}
		case EKDFOp::Remove:
			{
				TArray<FGameplayTag> Tags;
				if (!CollectTags(Args.mValue, Tags, OutError))
				{
					return false;
				}
				for (const FGameplayTag& Tag : Tags)
				{
					Container.RemoveTag(Tag);
				}
				return true;
			}
		default:
			OutError = FString::Printf(TEXT("Op '%s' is not supported on gameplay tag containers"),
									   *FKDFOpEngine::OpToString(Op));
			return false;
		}
	}

	bool ApplyNumericOp(EKDFOp Op, const FNumericProperty* Numeric, void* ValuePtr, const FKDFOpArgs& Args,
						FString& OutError)
	{
		const double Current = Numeric->IsFloatingPoint() ? Numeric->GetFloatingPointPropertyValue(ValuePtr)
			: IsUnsignedNumeric(Numeric) ? static_cast<double>(Numeric->GetUnsignedIntPropertyValue(ValuePtr))
										 : static_cast<double>(Numeric->GetSignedIntPropertyValue(ValuePtr));
		double Result = Current;

		if (Op == EKDFOp::Clamp)
		{
			double MinValue = 0.0;
			double MaxValue = 0.0;
			if (!TryReadNumericNode(Args.mMin, MinValue, OutError) || !TryReadNumericNode(Args.mMax, MaxValue, OutError))
			{
				return false;
			}
			Result = FMath::Clamp(Current, MinValue, MaxValue);
		}
		else
		{
			double Operand = 0.0;
			if (!TryReadNumericNode(Args.mValue, Operand, OutError))
			{
				return false;
			}
			switch (Op)
			{
			case EKDFOp::Add:
				Result = Current + Operand;
				break;
			case EKDFOp::Subtract:
				Result = Current - Operand;
				break;
			case EKDFOp::Multiply:
				Result = Current * Operand;
				break;
			case EKDFOp::Divide:
				if (Operand == 0.0)
				{
					OutError = TEXT("Division by zero");
					return false;
				}
				Result = Current / Operand;
				break;
			case EKDFOp::Min:
				Result = FMath::Min(Current, Operand);
				break;
			case EKDFOp::Max:
				Result = FMath::Max(Current, Operand);
				break;
			default:
				OutError = FString::Printf(TEXT("Op '%s' is not numeric"), *FKDFOpEngine::OpToString(Op));
				return false;
			}
		}

		if (!FMath::IsFinite(Result))
		{
			OutError = TEXT("Numeric operation produced a non-finite result");
			return false;
		}
		if (Numeric->IsFloatingPoint())
		{
			if (!Numeric->CanHoldValue(Result))
			{
				OutError = FString::Printf(TEXT("Numeric result is out of range for %s"), *Numeric->GetCPPType());
				return false;
			}
			Numeric->SetFloatingPointPropertyValue(ValuePtr, Result);
		}
		else
		{
			const int64 Rounded = static_cast<int64>(FMath::RoundToDouble(Result));
			if (!Numeric->CanHoldValue(Rounded))
			{
				OutError = FString::Printf(TEXT("Numeric result is out of range for %s"), *Numeric->GetCPPType());
				return false;
			}
			Numeric->SetIntPropertyValue(ValuePtr, Rounded);
		}
		return true;
	}

	bool ApplyStringOp(EKDFOp Op, const FProperty* Property, void* ValuePtr, const FKDFOpArgs& Args, FString& OutError)
	{
		FString Current;
		if (const FStrProperty* StrProperty = CastField<FStrProperty>(Property))
		{
			Current = StrProperty->GetPropertyValue(ValuePtr);
		}
		else if (const FNameProperty* NameProperty = CastField<FNameProperty>(Property))
		{
			Current = NameProperty->GetPropertyValue(ValuePtr).ToString();
		}
		else if (const FTextProperty* TextProperty = CastField<FTextProperty>(Property))
		{
			Current = TextProperty->GetPropertyValue(ValuePtr).ToString();
		}

		FString Result;
		switch (Op)
		{
		case EKDFOp::Clear:
			break;
		case EKDFOp::Append:
		case EKDFOp::Prepend:
			{
				if (Args.mValue == nullptr || !Args.mValue->IsScalar())
				{
					OutError = TEXT("String append/prepend requires a scalar 'value'");
					return false;
				}
				Result = Op == EKDFOp::Append ? Current + Args.mValue->Scalar : Args.mValue->Scalar + Current;
				break;
			}
		default:
			OutError = FString::Printf(TEXT("Op '%s' is not a string op"), *FKDFOpEngine::OpToString(Op));
			return false;
		}

		if (const FStrProperty* StrProperty = CastField<FStrProperty>(Property))
		{
			StrProperty->SetPropertyValue(ValuePtr, Result);
		}
		else if (const FNameProperty* NameProperty = CastField<FNameProperty>(Property))
		{
			NameProperty->SetPropertyValue(ValuePtr, FName(*Result));
		}
		else if (const FTextProperty* TextProperty = CastField<FTextProperty>(Property))
		{
			// Note: string ops on FText produce culture-invariant text (localization keys are dropped).
			TextProperty->SetPropertyValue(ValuePtr, FText::FromString(Result));
		}
		return true;
	}

	/** Elements to add for append/prepend/insert: a sequence node contributes each element, anything else one. */
	TArray<const FKDFNode*> GatherElementNodes(const FKDFNode* Node)
	{
		TArray<const FKDFNode*> Result;
		if (Node == nullptr)
		{
			return Result;
		}
		if (Node->IsSequence())
		{
			for (const TSharedRef<FKDFNode>& Element : Node->Sequence)
			{
				Result.Add(&Element.Get());
			}
		}
		else
		{
			Result.Add(Node);
		}
		return Result;
	}

	bool CompareArrayElements(const FProperty* Inner, const void* A, const void* B)
	{
		if (const FNumericProperty* Numeric = CastField<FNumericProperty>(Inner))
		{
			const double ValueA = Numeric->IsFloatingPoint() ? Numeric->GetFloatingPointPropertyValue(A)
				: IsUnsignedNumeric(Numeric) ? static_cast<double>(Numeric->GetUnsignedIntPropertyValue(A))
											 : static_cast<double>(Numeric->GetSignedIntPropertyValue(A));
			const double ValueB = Numeric->IsFloatingPoint() ? Numeric->GetFloatingPointPropertyValue(B)
				: IsUnsignedNumeric(Numeric) ? static_cast<double>(Numeric->GetUnsignedIntPropertyValue(B))
											 : static_cast<double>(Numeric->GetSignedIntPropertyValue(B));
			return ValueA <= ValueB;
		}
		return FKDFValueCodec::ExportText(Inner, A) <= FKDFValueCodec::ExportText(Inner, B);
	}

	bool ApplyArrayOp(EKDFOp Op, const FArrayProperty* ArrayProperty, void* ValuePtr, const FKDFOpArgs& Args,
					  FString& OutError)
	{
		FScriptArrayHelper Helper(ArrayProperty, ValuePtr);
		const FProperty* Inner = ArrayProperty->Inner;

		switch (Op)
		{
		case EKDFOp::Clear:
			Helper.EmptyValues();
			return true;
		case EKDFOp::Append:
		case EKDFOp::Prepend:
		case EKDFOp::Insert:
			{
				const TArray<const FKDFNode*> Elements = GatherElementNodes(Args.mValue);
				if (Elements.IsEmpty())
				{
					OutError = TEXT("Missing 'value' for array element operation");
					return false;
				}
				int32 InsertAt = 0;
				if (Op == EKDFOp::Append)
				{
					InsertAt = Helper.Num();
				}
				else if (Op == EKDFOp::Insert)
				{
					InsertAt = Args.mIndexA;
					if (InsertAt < 0 || InsertAt > Helper.Num())
					{
						OutError =
							FString::Printf(TEXT("Insert index %d out of range (size %d)"), InsertAt, Helper.Num());
						return false;
					}
				}
				Helper.InsertValues(InsertAt, Elements.Num());
				for (int32 Offset = 0; Offset < Elements.Num(); ++Offset)
				{
					if (!FKDFValueCodec::NodeToProperty(*Elements[Offset], Inner, Helper.GetRawPtr(InsertAt + Offset),
														OutError))
					{
						return false;
					}
				}
				return true;
			}
		case EKDFOp::Remove:
			{
				const TArray<const FKDFNode*> Elements = GatherElementNodes(Args.mValue);
				if (Elements.IsEmpty())
				{
					OutError = TEXT("Missing 'value' for array remove");
					return false;
				}
				int32 RemovedCount = 0;
				for (const FKDFNode* ElementNode : Elements)
				{
					FPropertyValueStorage ElementValue(Inner);
					if (!FKDFValueCodec::NodeToProperty(*ElementNode, Inner, ElementValue.GetData(), OutError))
					{
						return false;
					}
					for (int32 Index = Helper.Num() - 1; Index >= 0; --Index)
					{
						if (Inner->Identical(Helper.GetRawPtr(Index), ElementValue.GetData(), PPF_None))
						{
							Helper.RemoveValues(Index, 1);
							++RemovedCount;
						}
					}
				}
				if (RemovedCount == 0)
				{
					OutError = TEXT("Array remove matched no elements");
					return false;
				}
				return true;
			}
		case EKDFOp::RemoveAt:
			{
				if (!Helper.IsValidIndex(Args.mIndexA))
				{
					OutError =
						FString::Printf(TEXT("remove_at index %d out of range (size %d)"), Args.mIndexA, Helper.Num());
					return false;
				}
				Helper.RemoveValues(Args.mIndexA, 1);
				return true;
			}
		case EKDFOp::Swap:
			{
				if (!Helper.IsValidIndex(Args.mIndexA) || !Helper.IsValidIndex(Args.mIndexB))
				{
					OutError = FString::Printf(TEXT("swap indices (%d, %d) out of range (size %d)"), Args.mIndexA,
											   Args.mIndexB, Helper.Num());
					return false;
				}
				Helper.SwapValues(Args.mIndexA, Args.mIndexB);
				return true;
			}
		case EKDFOp::Replace:
			{
				if (!Helper.IsValidIndex(Args.mIndexA))
				{
					OutError =
						FString::Printf(TEXT("replace index %d out of range (size %d)"), Args.mIndexA, Helper.Num());
					return false;
				}
				if (Args.mValue == nullptr)
				{
					OutError = TEXT("Missing 'value' for array replace");
					return false;
				}
				return FKDFValueCodec::NodeToProperty(*Args.mValue, Inner, Helper.GetRawPtr(Args.mIndexA), OutError);
			}
		case EKDFOp::Duplicate:
			{
				if (!Helper.IsValidIndex(Args.mIndexA))
				{
					OutError =
						FString::Printf(TEXT("duplicate index %d out of range (size %d)"), Args.mIndexA, Helper.Num());
					return false;
				}
				const int32 NewIndex = Helper.AddValue();
				Inner->CopyCompleteValue(Helper.GetRawPtr(NewIndex), Helper.GetRawPtr(Args.mIndexA));
				return true;
			}
		case EKDFOp::Sort:
			{
				// Insertion sort through SwapValues — element counts here are small (recipe lists etc.).
				for (int32 OuterIndex = 1; OuterIndex < Helper.Num(); ++OuterIndex)
				{
					for (int32 Index = OuterIndex; Index > 0 &&
						 !CompareArrayElements(Inner, Helper.GetRawPtr(Index - 1), Helper.GetRawPtr(Index));
						 --Index)
					{
						Helper.SwapValues(Index - 1, Index);
					}
				}
				return true;
			}
		case EKDFOp::Unique:
			{
				for (int32 Index = Helper.Num() - 1; Index > 0; --Index)
				{
					for (int32 Earlier = 0; Earlier < Index; ++Earlier)
					{
						if (Inner->Identical(Helper.GetRawPtr(Index), Helper.GetRawPtr(Earlier), PPF_None))
						{
							Helper.RemoveValues(Index, 1);
							break;
						}
					}
				}
				return true;
			}
		case EKDFOp::Reverse:
			{
				for (int32 Index = 0; Index < Helper.Num() / 2; ++Index)
				{
					Helper.SwapValues(Index, Helper.Num() - 1 - Index);
				}
				return true;
			}
		default:
			OutError = FString::Printf(TEXT("Op '%s' is not an array op"), *FKDFOpEngine::OpToString(Op));
			return false;
		}
	}

	bool ApplySetOp(EKDFOp Op, const FSetProperty* SetProperty, void* ValuePtr, const FKDFOpArgs& Args,
					FString& OutError)
	{
		FScriptSetHelper Helper(SetProperty, ValuePtr);
		switch (Op)
		{
		case EKDFOp::Clear:
			Helper.EmptyElements();
			return true;
		case EKDFOp::Append:
		case EKDFOp::Prepend:
			{
				for (const FKDFNode* ElementNode : GatherElementNodes(Args.mValue))
				{
					FPropertyValueStorage ElementValue(SetProperty->ElementProp);
					if (!FKDFValueCodec::NodeToProperty(*ElementNode, SetProperty->ElementProp, ElementValue.GetData(),
														OutError))
					{
						return false;
					}
					Helper.AddElement(ElementValue.GetData());
				}
				return true;
			}
		case EKDFOp::Remove:
			{
				for (const FKDFNode* ElementNode : GatherElementNodes(Args.mValue))
				{
					FPropertyValueStorage ElementValue(SetProperty->ElementProp);
					if (!FKDFValueCodec::NodeToProperty(*ElementNode, SetProperty->ElementProp, ElementValue.GetData(),
														OutError))
					{
						return false;
					}
					Helper.RemoveElement(ElementValue.GetData());
				}
				return true;
			}
		case EKDFOp::Unique:
			return true; // sets are inherently unique
		default:
			OutError = FString::Printf(TEXT("Op '%s' is not supported on sets"), *FKDFOpEngine::OpToString(Op));
			return false;
		}
	}

	bool ApplyMapOp(EKDFOp Op, const FMapProperty* MapProperty, void* ValuePtr, const FKDFOpArgs& Args,
					FString& OutError)
	{
		FScriptMapHelper Helper(MapProperty, ValuePtr);
		switch (Op)
		{
		case EKDFOp::Clear:
			Helper.EmptyValues();
			return true;
		case EKDFOp::Remove:
			{
				if (Args.mValue == nullptr || !Args.mValue->IsScalar())
				{
					OutError = TEXT("Map remove requires the key as scalar 'value'");
					return false;
				}
				FPropertyValueStorage MapKey(MapProperty->KeyProp);
				if (!FKDFValueCodec::NodeToProperty(*Args.mValue, MapProperty->KeyProp, MapKey.GetData(), OutError))
				{
					return false;
				}
				if (!Helper.RemovePair(MapKey.GetData()))
				{
					OutError = FString::Printf(TEXT("Map key '%s' not found"), *Args.mValue->Scalar);
					return false;
				}
				return true;
			}
		case EKDFOp::Append: // merge entries from a map value
			{
				if (Args.mValue == nullptr || !Args.mValue->IsMap())
				{
					OutError = TEXT("Map append requires a map 'value'");
					return false;
				}
				for (const TTuple<FString, TSharedRef<FKDFNode>>& Pair : Args.mValue->Map)
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
		default:
			OutError = FString::Printf(TEXT("Op '%s' is not supported on maps"), *FKDFOpEngine::OpToString(Op));
			return false;
		}
	}

	bool SetInstancedObjectArray(UObject* Outer, const FArrayProperty* ArrayProperty, void* ValuePtr,
								 const FKDFNode& Value, FString& OutError)
	{
		const FObjectProperty* Inner = CastField<FObjectProperty>(ArrayProperty->Inner);
		if (Inner == nullptr || !Value.IsSequence())
			return false;
		for (const TSharedRef<FKDFNode>& EntryRef : Value.Sequence)
		{
			if (!EntryRef->IsMap() || EntryRef->GetString(TEXT("class"), FString()).IsEmpty())
				return false;
		}
		FScriptArrayHelper Array(ArrayProperty, ValuePtr);
		Array.EmptyValues();
		for (const TSharedRef<FKDFNode>& EntryRef : Value.Sequence)
		{
			const FKDFNode& Entry = EntryRef.Get();
			UClass* Class = FKDFValueCodec::ResolveClass(Entry.GetString(TEXT("class"), FString()), OutError);
			if (Class == nullptr || !Class->IsChildOf(Inner->PropertyClass))
			{
				OutError = TEXT("Instanced object class is incompatible with the array element type");
				return false;
			}
			UObject* Instance = NewObject<UObject>(Outer, Class);
			if (const FKDFNode* Properties = Entry.Find(TEXT("properties")))
			{
				for (const TSharedRef<FKDFNode>& PropertyRef : Properties->Sequence)
				{
					FString PathText;
					EKDFOp Op = EKDFOp::Set;
					FKDFOpArgs Args;
					if (!FKDFOpEngine::ParseOpEntry(PropertyRef.Get(), PathText, Op, Args, OutError))
						return false;
					FKDFPropertyPath Path;
					if (!FKDFPropertyPath::Parse(PathText, Path, OutError) ||
						!FKDFOpEngine::ApplyOp(Instance, Path, Op, Args, OutError))
						return false;
				}
			}
			const int32 Index = Array.AddValue();
			Inner->SetObjectPropertyValue(Array.GetRawPtr(Index), Instance);
		}
		return true;
	}

	bool SetInstancedObjectProperty(UObject* Outer, const FObjectProperty* Property, void* ValuePtr,
									const FKDFNode& Value, FString& OutError)
	{
		if (Property == nullptr || !Value.IsScalar())
		{
			return false;
		}
		if (Value.Scalar.IsEmpty() || Value.Scalar.Equals(TEXT("none"), ESearchCase::IgnoreCase))
		{
			Property->SetObjectPropertyValue(ValuePtr, nullptr);
			return true;
		}
		UClass* Class = FKDFValueCodec::ResolveClass(Value.Scalar, OutError);
		if (Class == nullptr || !Class->IsChildOf(Property->PropertyClass))
		{
			OutError = TEXT("Instanced object class is incompatible with the property type");
			return false;
		}
		Property->SetObjectPropertyValue(ValuePtr, NewObject<UObject>(Outer, Class));
		return true;
	}

	bool ApplyCopyMove(EKDFOp Op, UObject* RootObject, const FKDFResolvedProperty& Dest, const FKDFOpArgs& Args,
					   FString& OutError)
	{
		if (Args.mFromPath.IsEmpty())
		{
			OutError = TEXT("copy/move requires a 'from' property path");
			return false;
		}
		FKDFPropertyPath FromPath;
		if (!FKDFPropertyPath::Parse(Args.mFromPath, FromPath, OutError))
		{
			return false;
		}
		FKDFResolvedProperty Source;
		if (!FKDFPropertyResolver::Resolve(RootObject, FromPath, Source, OutError))
		{
			return false;
		}
		if (Source.mProperty->SameType(Dest.mProperty))
		{
			Dest.mProperty->CopyCompleteValue(Dest.mValuePtr, Source.mValuePtr);
		}
		else
		{
			const FString Exported = FKDFValueCodec::ExportText(Source.mProperty, Source.mValuePtr);
			if (!FKDFValueCodec::ImportText(Exported, Dest.mProperty, Dest.mValuePtr))
			{
				OutError =
					FString::Printf(TEXT("copy/move between incompatible types (%s → %s)"),
									*Source.mProperty->GetClass()->GetName(), *Dest.mProperty->GetClass()->GetName());
				return false;
			}
		}
		if (Op == EKDFOp::Move)
		{
			Source.mProperty->ClearValue(Source.mValuePtr);
			if (Source.mOwningSetProperty != nullptr && Source.mOwningSetValuePtr != nullptr)
			{
				FScriptSetHelper(Source.mOwningSetProperty, Source.mOwningSetValuePtr).Rehash();
			}
		}
		return true;
	}
} // namespace

bool FKDFOpEngine::ParseOpName(const FString& Name, EKDFOp& OutOp)
{
	static const TMap<FString, EKDFOp> OpNames = {{TEXT("set"), EKDFOp::Set},
												  {TEXT("add"), EKDFOp::Add},
												  {TEXT("subtract"), EKDFOp::Subtract},
												  {TEXT("multiply"), EKDFOp::Multiply},
												  {TEXT("divide"), EKDFOp::Divide},
												  {TEXT("min"), EKDFOp::Min},
												  {TEXT("max"), EKDFOp::Max},
												  {TEXT("clamp"), EKDFOp::Clamp},
												  {TEXT("append"), EKDFOp::Append},
												  {TEXT("prepend"), EKDFOp::Prepend},
												  {TEXT("insert"), EKDFOp::Insert},
												  {TEXT("remove"), EKDFOp::Remove},
												  {TEXT("remove_at"), EKDFOp::RemoveAt},
												  {TEXT("clear"), EKDFOp::Clear},
												  {TEXT("swap"), EKDFOp::Swap},
												  {TEXT("replace"), EKDFOp::Replace},
												  {TEXT("copy"), EKDFOp::Copy},
												  {TEXT("move"), EKDFOp::Move},
												  {TEXT("duplicate"), EKDFOp::Duplicate},
												  {TEXT("sort"), EKDFOp::Sort},
												  {TEXT("unique"), EKDFOp::Unique},
												  {TEXT("reverse"), EKDFOp::Reverse}};
	const EKDFOp* Found = OpNames.Find(Name.ToLower());
	if (Found == nullptr)
	{
		return false;
	}
	OutOp = *Found;
	return true;
}

bool FKDFOpEngine::ParseOpEntry(const FKDFNode& Entry, FString& OutPathString, EKDFOp& OutOp, FKDFOpArgs& OutArgs,
								FString& OutError)
{
	if (!Entry.IsMap())
	{
		OutError = TEXT("Property entry must be a map with at least a 'path' key");
		return false;
	}
	OutPathString = Entry.GetString(TEXT("path"), FString());
	if (OutPathString.IsEmpty())
	{
		OutError = TEXT("Property entry is missing 'path'");
		return false;
	}
	const FString OpName = Entry.GetString(TEXT("op"), TEXT("set"));
	if (!ParseOpName(OpName, OutOp))
	{
		OutError = FString::Printf(TEXT("Unknown op '%s'"), *OpName);
		return false;
	}
	OutArgs = FKDFOpArgs();
	OutArgs.mValue = Entry.Find(TEXT("value"));
	OutArgs.mIndexA = static_cast<int32>(Entry.GetInt(TEXT("index"), INDEX_NONE));
	OutArgs.mIndexB = static_cast<int32>(Entry.GetInt(TEXT("with"), INDEX_NONE));
	OutArgs.mMin = Entry.Find(TEXT("min"));
	OutArgs.mMax = Entry.Find(TEXT("max"));
	OutArgs.mFromPath = Entry.GetString(TEXT("from"), FString());
	return true;
}

FString FKDFOpEngine::OpToString(EKDFOp Op)
{
	switch (Op)
	{
	case EKDFOp::Set:
		return TEXT("set");
	case EKDFOp::Add:
		return TEXT("add");
	case EKDFOp::Subtract:
		return TEXT("subtract");
	case EKDFOp::Multiply:
		return TEXT("multiply");
	case EKDFOp::Divide:
		return TEXT("divide");
	case EKDFOp::Min:
		return TEXT("min");
	case EKDFOp::Max:
		return TEXT("max");
	case EKDFOp::Clamp:
		return TEXT("clamp");
	case EKDFOp::Append:
		return TEXT("append");
	case EKDFOp::Prepend:
		return TEXT("prepend");
	case EKDFOp::Insert:
		return TEXT("insert");
	case EKDFOp::Remove:
		return TEXT("remove");
	case EKDFOp::RemoveAt:
		return TEXT("remove_at");
	case EKDFOp::Clear:
		return TEXT("clear");
	case EKDFOp::Swap:
		return TEXT("swap");
	case EKDFOp::Replace:
		return TEXT("replace");
	case EKDFOp::Copy:
		return TEXT("copy");
	case EKDFOp::Move:
		return TEXT("move");
	case EKDFOp::Duplicate:
		return TEXT("duplicate");
	case EKDFOp::Sort:
		return TEXT("sort");
	case EKDFOp::Unique:
		return TEXT("unique");
	case EKDFOp::Reverse:
		return TEXT("reverse");
	default:
		return TEXT("unknown");
	}
}

bool FKDFOpEngine::IsOpSupported(EKDFOp Op, const FProperty* LeafProperty)
{
	if (LeafProperty == nullptr)
	{
		return false;
	}
	switch (Op)
	{
	case EKDFOp::Set:
	case EKDFOp::Copy:
	case EKDFOp::Move:
		return true;
	case EKDFOp::Add:
	case EKDFOp::Subtract:
	case EKDFOp::Multiply:
	case EKDFOp::Divide:
	case EKDFOp::Min:
	case EKDFOp::Max:
	case EKDFOp::Clamp:
		return IsPlainNumeric(LeafProperty);
	case EKDFOp::Append:
	case EKDFOp::Prepend:
		return LeafProperty->IsA<FArrayProperty>() || LeafProperty->IsA<FSetProperty>() ||
			LeafProperty->IsA<FMapProperty>() || IsStringLike(LeafProperty) || IsTagContainerProperty(LeafProperty);
	case EKDFOp::Insert:
	case EKDFOp::RemoveAt:
	case EKDFOp::Swap:
	case EKDFOp::Replace:
	case EKDFOp::Duplicate:
	case EKDFOp::Sort:
	case EKDFOp::Reverse:
		return LeafProperty->IsA<FArrayProperty>();
	case EKDFOp::Remove:
		return LeafProperty->IsA<FArrayProperty>() || LeafProperty->IsA<FSetProperty>() ||
			LeafProperty->IsA<FMapProperty>() || IsTagContainerProperty(LeafProperty);
	case EKDFOp::Clear:
		return LeafProperty->IsA<FArrayProperty>() || LeafProperty->IsA<FSetProperty>() ||
			LeafProperty->IsA<FMapProperty>() || IsStringLike(LeafProperty) || IsTagContainerProperty(LeafProperty);
	case EKDFOp::Unique:
		return LeafProperty->IsA<FArrayProperty>() || LeafProperty->IsA<FSetProperty>();
	default:
		return false;
	}
}

bool FKDFOpEngine::ApplyOp(UObject* RootObject, const FKDFPropertyPath& Path, EKDFOp Op, const FKDFOpArgs& Args,
						   FString& OutError)
{
	FKDFResolvedProperty Resolved;
	if (!FKDFPropertyResolver::Resolve(RootObject, Path, Resolved, OutError))
	{
		return false;
	}
	if (!IsOpSupported(Op, Resolved.mProperty))
	{
		OutError = FString::Printf(TEXT("Op '%s' is not supported on property '%s' (%s)"), *OpToString(Op),
								   *Path.ToString(), *Resolved.mProperty->GetClass()->GetName());
		return false;
	}
	FPropertyValueStorage OriginalPropertyValue(Resolved.mProperty);
	Resolved.mProperty->CopyCompleteValue(OriginalPropertyValue.GetData(), Resolved.mValuePtr);
	const auto FinishWrite = [&Resolved, &OriginalPropertyValue](const bool bSucceeded)
	{
		if (!bSucceeded)
		{
			Resolved.mProperty->CopyCompleteValue(Resolved.mValuePtr, OriginalPropertyValue.GetData());
			return false;
		}
		if (bSucceeded && Resolved.mOwningSetProperty != nullptr && Resolved.mOwningSetValuePtr != nullptr)
		{
			FScriptSetHelper(Resolved.mOwningSetProperty, Resolved.mOwningSetValuePtr).Rehash();
		}
		return bSucceeded;
	};

	switch (Op)
	{
	case EKDFOp::Set:
		if (IsTagContainerProperty(Resolved.mProperty) && Args.mValue != nullptr && Args.mValue->IsSequence())
		{
			return FinishWrite(ApplyTagContainerOp(Op, Resolved.mValuePtr, Args, OutError));
		}
		if (Args.mValue == nullptr)
		{
			OutError = TEXT("Missing 'value' for set");
			return false;
		}
		if (const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Resolved.mProperty);
			ArrayProperty != nullptr &&
			SetInstancedObjectArray(RootObject, ArrayProperty, Resolved.mValuePtr, *Args.mValue, OutError))
		{
			return FinishWrite(true);
		}
		if (const FObjectProperty* ObjectProperty = CastField<FObjectProperty>(Resolved.mProperty);
			ObjectProperty != nullptr &&
			ObjectProperty->HasAnyPropertyFlags(CPF_InstancedReference | CPF_PersistentInstance) &&
			SetInstancedObjectProperty(RootObject, ObjectProperty, Resolved.mValuePtr, *Args.mValue, OutError))
		{
			return FinishWrite(true);
		}
		return FinishWrite(
			FKDFValueCodec::NodeToProperty(*Args.mValue, Resolved.mProperty, Resolved.mValuePtr, OutError));
	case EKDFOp::Copy:
	case EKDFOp::Move:
		return FinishWrite(ApplyCopyMove(Op, RootObject, Resolved, Args, OutError));
	default:
		break;
	}

	if (IsTagContainerProperty(Resolved.mProperty))
	{
		return FinishWrite(ApplyTagContainerOp(Op, Resolved.mValuePtr, Args, OutError));
	}
	if (const FNumericProperty* Numeric = CastField<FNumericProperty>(Resolved.mProperty))
	{
		return FinishWrite(ApplyNumericOp(Op, Numeric, Resolved.mValuePtr, Args, OutError));
	}
	if (IsStringLike(Resolved.mProperty))
	{
		return FinishWrite(ApplyStringOp(Op, Resolved.mProperty, Resolved.mValuePtr, Args, OutError));
	}
	if (const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Resolved.mProperty))
	{
		return FinishWrite(ApplyArrayOp(Op, ArrayProperty, Resolved.mValuePtr, Args, OutError));
	}
	if (const FSetProperty* SetProperty = CastField<FSetProperty>(Resolved.mProperty))
	{
		return FinishWrite(ApplySetOp(Op, SetProperty, Resolved.mValuePtr, Args, OutError));
	}
	if (const FMapProperty* MapProperty = CastField<FMapProperty>(Resolved.mProperty))
	{
		return FinishWrite(ApplyMapOp(Op, MapProperty, Resolved.mValuePtr, Args, OutError));
	}

	OutError =
		FString::Printf(TEXT("Op '%s' could not be dispatched for property '%s'"), *OpToString(Op), *Path.ToString());
	return false;
}
