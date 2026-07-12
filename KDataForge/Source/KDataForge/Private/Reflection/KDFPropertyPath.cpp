#include "Reflection/KDFPropertyPath.h"

namespace
{
	bool IsIdentifierChar(const TCHAR Char) { return FChar::IsAlnum(Char) || Char == TEXT('_'); }
} // namespace

bool FKDFPropertyPath::Parse(const FString& PathString, FKDFPropertyPath& OutPath, FString& OutError)
{
	OutPath.mOriginal = PathString;
	OutPath.mSegments.Reset();

	const TCHAR* Cursor = *PathString;
	const TCHAR* const End = Cursor + PathString.Len();

	while (Cursor < End)
	{
		const TCHAR* NameStart = Cursor;
		while (Cursor < End && IsIdentifierChar(*Cursor))
		{
			++Cursor;
		}
		if (Cursor == NameStart)
		{
			OutError = FString::Printf(TEXT("Invalid property path '%s': expected property name at offset %d"),
									   *PathString, static_cast<int32>(NameStart - *PathString));
			return false;
		}
		FKDFPathSegment& NameSegment = OutPath.mSegments.AddDefaulted_GetRef();
		NameSegment.mKind = EKDFPathSegmentKind::Property;
		NameSegment.mName = FString(static_cast<int32>(Cursor - NameStart), NameStart);

		while (Cursor < End && *Cursor == TEXT('['))
		{
			++Cursor; // consume '['
			if (Cursor < End && (*Cursor == TEXT('"') || *Cursor == TEXT('\'')))
			{
				const TCHAR Quote = *Cursor;
				++Cursor;
				FString DecodedKey;
				bool bClosed = false;
				while (Cursor < End)
				{
					if (*Cursor == Quote)
					{
						++Cursor;
						bClosed = true;
						break;
					}
					if (*Cursor == TEXT('\\'))
					{
						++Cursor;
						if (Cursor >= End)
						{
							break;
						}
						switch (*Cursor)
						{
						case TEXT('n'): DecodedKey.AppendChar(TEXT('\n')); break;
						case TEXT('r'): DecodedKey.AppendChar(TEXT('\r')); break;
						case TEXT('t'): DecodedKey.AppendChar(TEXT('	')); break;
						default: DecodedKey.AppendChar(*Cursor); break;
						}
						++Cursor;
						continue;
					}
					DecodedKey.AppendChar(*Cursor);
					++Cursor;
				}
				if (!bClosed)
				{
					OutError =
						FString::Printf(TEXT("Invalid property path '%s': unterminated quoted key"), *PathString);
					return false;
				}
				FKDFPathSegment& KeySegment = OutPath.mSegments.AddDefaulted_GetRef();
				KeySegment.mKind = EKDFPathSegmentKind::Key;
				KeySegment.mKey = MoveTemp(DecodedKey);
			}
			else
			{
				const TCHAR* TokenStart = Cursor;
				while (Cursor < End && *Cursor != TEXT(']'))
				{
					++Cursor;
				}
				const FString Token(static_cast<int32>(Cursor - TokenStart), TokenStart);
				if (Token.IsEmpty())
				{
					OutError = FString::Printf(TEXT("Invalid property path '%s': empty [] accessor"), *PathString);
					return false;
				}
				if (Token.IsNumeric())
				{
					FKDFPathSegment& IndexSegment = OutPath.mSegments.AddDefaulted_GetRef();
					IndexSegment.mKind = EKDFPathSegmentKind::Index;
					IndexSegment.mIndex = FCString::Atoi(*Token);
				}
				else
				{
					// Unquoted non-numeric token is treated as a map key.
					FKDFPathSegment& KeySegment = OutPath.mSegments.AddDefaulted_GetRef();
					KeySegment.mKind = EKDFPathSegmentKind::Key;
					KeySegment.mKey = Token;
				}
			}
			if (Cursor >= End || *Cursor != TEXT(']'))
			{
				OutError = FString::Printf(TEXT("Invalid property path '%s': expected ']'"), *PathString);
				return false;
			}
			++Cursor; // consume ']'
		}

		if (Cursor < End)
		{
			if (*Cursor != TEXT('.'))
			{
				OutError = FString::Printf(TEXT("Invalid property path '%s': unexpected character '%c'"), *PathString,
										   *Cursor);
				return false;
			}
			++Cursor; // consume '.'
			if (Cursor >= End)
			{
				OutError = FString::Printf(TEXT("Invalid property path '%s': trailing '.'"), *PathString);
				return false;
			}
		}
	}

	if (OutPath.mSegments.IsEmpty())
	{
		OutError = TEXT("Empty property path");
		return false;
	}
	return true;
}

FProperty* FKDFPropertyResolver::FindPropertyByNameFlexible(UStruct* Struct, const FString& Name)
{
	if (FProperty* Direct = FindFProperty<FProperty>(Struct, *Name))
	{
		return Direct;
	}
	// Blueprint-defined members carry mangled names — match on the authored name too.
	for (TFieldIterator<FProperty> It(Struct); It; ++It)
	{
		if (It->GetAuthoredName().Equals(Name, ESearchCase::IgnoreCase) ||
			It->GetName().Equals(Name, ESearchCase::IgnoreCase))
		{
			return *It;
		}
	}
	return nullptr;
}

namespace
{
	bool ImportMapKey(const FMapProperty* MapProperty, const FString& KeyText, void* KeyPtr, FString& OutError)
	{
		FProperty* KeyProperty = MapProperty->KeyProp;
		if (const FStrProperty* StrKey = CastField<FStrProperty>(KeyProperty))
		{
			StrKey->SetPropertyValue(KeyPtr, KeyText);
			return true;
		}
		if (const FNameProperty* NameKey = CastField<FNameProperty>(KeyProperty))
		{
			NameKey->SetPropertyValue(KeyPtr, FName(*KeyText));
			return true;
		}
		if (KeyProperty->ImportText_Direct(*KeyText, KeyPtr, nullptr, PPF_None) == nullptr)
		{
			OutError = FString::Printf(TEXT("Could not convert '%s' to map key type %s"), *KeyText,
									   *KeyProperty->GetCPPType());
			return false;
		}
		return true;
	}
} // namespace

bool FKDFPropertyResolver::Resolve(UObject* RootObject, const FKDFPropertyPath& Path, FKDFResolvedProperty& OutResolved,
								   FString& OutError)
{
	if (!IsValid(RootObject))
	{
		OutError = TEXT("Invalid root object");
		return false;
	}
	return Resolve(RootObject, RootObject->GetClass(), Path, OutResolved, OutError);
}

bool FKDFPropertyResolver::Resolve(void* ContainerPtr, UStruct* ContainerStruct, const FKDFPropertyPath& Path,
								   FKDFResolvedProperty& OutResolved, FString& OutError)
{
	OutResolved = FKDFResolvedProperty();
	if (ContainerPtr == nullptr || ContainerStruct == nullptr || !Path.IsValid())
	{
		OutError = TEXT("Invalid resolve arguments");
		return false;
	}

	void* CurrentValuePtr = ContainerPtr; // container for Property segments, value storage otherwise
	UStruct* LookupStruct = ContainerStruct; // struct to look names up in (valid for Property segments)
	FProperty* CurrentProperty = nullptr; // property describing the value at CurrentValuePtr (null before first segment)

	for (int32 SegmentIndex = 0; SegmentIndex < Path.mSegments.Num(); ++SegmentIndex)
	{
		const FKDFPathSegment& Segment = Path.mSegments[SegmentIndex];
		switch (Segment.mKind)
		{
		case EKDFPathSegmentKind::Property:
			{
				if (CurrentProperty != nullptr)
				{
					// Descend from the current value into a member: struct or object dereference.
					if (const FStructProperty* StructProperty = CastField<FStructProperty>(CurrentProperty))
					{
						LookupStruct = StructProperty->Struct;
					}
					else if (const FObjectPropertyBase* ObjectProperty =
								 CastField<FObjectPropertyBase>(CurrentProperty))
					{
						UObject* Object = ObjectProperty->GetObjectPropertyValue(CurrentValuePtr);
						if (!IsValid(Object))
						{
							OutError = FString::Printf(TEXT("Path '%s': object at '%s' is null"), *Path.mOriginal,
													   *Segment.mName);
							return false;
						}
						CurrentValuePtr = Object;
						LookupStruct = Object->GetClass();
					}
					else
					{
						OutError = FString::Printf(TEXT("Path '%s': cannot descend into non-struct property '%s'"),
												   *Path.mOriginal, *CurrentProperty->GetName());
						return false;
					}
				}
				FProperty* ResolvedProperty = FindPropertyByNameFlexible(LookupStruct, Segment.mName);
				if (ResolvedProperty == nullptr)
				{
					OutError = FString::Printf(TEXT("Path '%s': property '%s' not found on %s"), *Path.mOriginal,
											   *Segment.mName, *LookupStruct->GetName());
					return false;
				}
				CurrentValuePtr = ResolvedProperty->ContainerPtrToValuePtr<void>(CurrentValuePtr);
				CurrentProperty = ResolvedProperty;
				break;
			}
		case EKDFPathSegmentKind::Index:
			{
				if (const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(CurrentProperty))
				{
					FScriptArrayHelper Helper(ArrayProperty, CurrentValuePtr);
					if (!Helper.IsValidIndex(Segment.mIndex))
					{
						OutError = FString::Printf(TEXT("Path '%s': index %d out of range (size %d)"), *Path.mOriginal,
												   Segment.mIndex, Helper.Num());
						return false;
					}
					CurrentValuePtr = Helper.GetRawPtr(Segment.mIndex);
					CurrentProperty = ArrayProperty->Inner;
				}
				else if (const FSetProperty* SetProperty = CastField<FSetProperty>(CurrentProperty))
				{
					if (OutResolved.mOwningSetProperty == nullptr)
					{
						OutResolved.mOwningSetProperty = SetProperty;
						OutResolved.mOwningSetValuePtr = CurrentValuePtr;
					}
					FScriptSetHelper Helper(SetProperty, CurrentValuePtr);
					int32 LogicalIndex = Segment.mIndex;
					int32 InternalIndex = INDEX_NONE;
					for (int32 Sparse = 0; Sparse < Helper.GetMaxIndex(); ++Sparse)
					{
						if (Helper.IsValidIndex(Sparse) && LogicalIndex-- == 0)
						{
							InternalIndex = Sparse;
							break;
						}
					}
					if (InternalIndex == INDEX_NONE)
					{
						OutError = FString::Printf(TEXT("Path '%s': set index %d out of range (size %d)"),
												   *Path.mOriginal, Segment.mIndex, Helper.Num());
						return false;
					}
					CurrentValuePtr = Helper.GetElementPtr(InternalIndex);
					CurrentProperty = SetProperty->ElementProp;
				}
				else
				{
					OutError = FString::Printf(TEXT("Path '%s': [%d] used on non-array property"), *Path.mOriginal,
											   Segment.mIndex);
					return false;
				}
				break;
			}
		case EKDFPathSegmentKind::Key:
			{
				const FMapProperty* MapProperty = CastField<FMapProperty>(CurrentProperty);
				if (MapProperty == nullptr)
				{
					OutError = FString::Printf(TEXT("Path '%s': [\"%s\"] used on non-map property"), *Path.mOriginal,
											   *Segment.mKey);
					return false;
				}
				FScriptMapHelper Helper(MapProperty, CurrentValuePtr);
				// Build a temporary key value to search with.
				void* KeyStorage = FMemory::Malloc(MapProperty->KeyProp->GetSize(), MapProperty->KeyProp->GetMinAlignment());
				MapProperty->KeyProp->InitializeValue(KeyStorage);
				FString KeyError;
				if (!ImportMapKey(MapProperty, Segment.mKey, KeyStorage, KeyError))
				{
					MapProperty->KeyProp->DestroyValue(KeyStorage);
					FMemory::Free(KeyStorage);
					OutError = FString::Printf(TEXT("Path '%s': %s"), *Path.mOriginal, *KeyError);
					return false;
				}
				uint8* ValuePtr = Helper.FindValueFromHash(KeyStorage);
				MapProperty->KeyProp->DestroyValue(KeyStorage);
				FMemory::Free(KeyStorage);
				if (ValuePtr == nullptr)
				{
					OutError =
						FString::Printf(TEXT("Path '%s': map key '%s' not found"), *Path.mOriginal, *Segment.mKey);
					return false;
				}
				CurrentValuePtr = ValuePtr;
				CurrentProperty = MapProperty->ValueProp;
				break;
			}
		}
	}

	OutResolved.mProperty = CurrentProperty;
	OutResolved.mValuePtr = CurrentValuePtr;
	return OutResolved.IsValid();
}
