// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/UnrealType.h"

enum class EKDFPathSegmentKind : uint8
{

	Property,

	Index,

	Key
};

struct KDATAFORGE_API FKDFPathSegment
{
	EKDFPathSegmentKind mKind = EKDFPathSegmentKind::Property;

	FString mName;

	int32 mIndex = INDEX_NONE;

	FString mKey;
};

struct KDATAFORGE_API FKDFPropertyPath
{
	FString mOriginal;
	TArray<FKDFPathSegment> mSegments;

	static bool Parse(const FString& PathString, FKDFPropertyPath& OutPath, FString& OutError);

	FString ToString() const { return mOriginal; }

	bool IsValid() const { return mSegments.Num() > 0; }
};

struct KDATAFORGE_API FKDFResolvedProperty
{

	FProperty* mProperty = nullptr;

	void* mValuePtr = nullptr;

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

	static FProperty* FindPropertyByNameFlexible(UStruct* Struct, const FString& Name);
};
