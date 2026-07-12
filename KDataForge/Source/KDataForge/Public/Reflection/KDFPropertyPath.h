#pragma once

#include "CoreMinimal.h"
#include "UObject/UnrealType.h"

/** Kind of one parsed path segment. */
enum class EKDFPathSegmentKind : uint8
{
	/** A property (or struct member / component object member) referenced by name. */
	Property,
	/** An array/set element referenced by numeric index, e.g. `mIngredients[2]`. */
	Index,
	/** A map value referenced by key, e.g. `mMap["SomeKey"]`. */
	Key
};

struct KDATAFORGE_API FKDFPathSegment
{
	EKDFPathSegmentKind mKind = EKDFPathSegmentKind::Property;

	/** Property name (Property kind). */
	FString mName;

	/** Element index (Index kind). */
	int32 mIndex = INDEX_NONE;

	/** Map key text (Key kind). */
	FString mKey;
};

/**
 * Parsed property path.
 * Grammar: `Segment(.Segment)*` where Segment = `Name([<int>]|["key"]|[key])*`.
 * Examples: `mManufactoringDuration`, `mIngredients[0].Amount`, `mMap["Foo"].Value`.
 * Object-typed segments are dereferenced, so default-subobject/component members resolve too.
 */
struct KDATAFORGE_API FKDFPropertyPath
{
	FString mOriginal;
	TArray<FKDFPathSegment> mSegments;

	static bool Parse(const FString& PathString, FKDFPropertyPath& OutPath, FString& OutError);

	FString ToString() const { return mOriginal; }

	bool IsValid() const { return mSegments.Num() > 0; }
};

/** Result of resolving a path against an object/struct instance. */
struct KDATAFORGE_API FKDFResolvedProperty
{
	/** Leaf property. For container elements this is the inner/value property. */
	FProperty* mProperty = nullptr;

	/** Address of the leaf value. */
	void* mValuePtr = nullptr;

	/** Owning set when the path descends through a set element; used to restore its hash after writes. */
	const FSetProperty* mOwningSetProperty = nullptr;
	void* mOwningSetValuePtr = nullptr;

	bool IsValid() const { return mProperty != nullptr && mValuePtr != nullptr; }
};

class KDATAFORGE_API FKDFPropertyResolver
{
public:
	static bool Resolve(UObject* RootObject, const FKDFPropertyPath& Path, FKDFResolvedProperty& OutResolved,
						FString& OutError);

	static bool Resolve(void* ContainerPtr, UStruct* ContainerStruct, const FKDFPropertyPath& Path,
						FKDFResolvedProperty& OutResolved, FString& OutError);

	/** Name lookup that also matches Blueprint authored names (case-insensitive). */
	static FProperty* FindPropertyByNameFlexible(UStruct* Struct, const FString& Name);
};
