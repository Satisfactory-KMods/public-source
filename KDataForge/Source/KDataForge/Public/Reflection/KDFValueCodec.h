// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/UnrealType.h"

#include "KDFNode.h"

class FKDFDynamicContentRegistry;

class KDATAFORGE_API FKDFValueCodec
{
public:

	static void SetDynamicContentRegistry(FKDFDynamicContentRegistry* Registry);

	class KDATAFORGE_API FPackScope
	{
	public:
		explicit FPackScope(const FString& PackRef);
		~FPackScope();

		FPackScope(const FPackScope&) = delete;
		FPackScope& operator=(const FPackScope&) = delete;

	private:
		FString mPrevious;
	};

	static FString GetCurrentPackScope();

	static bool NodeToProperty(const FKDFNode& Node, const FProperty* Property, void* ValuePtr, FString& OutError);

	static bool PropertyToNode(const FProperty* Property, const void* ValuePtr, FKDFNode& OutNode);

	static FString ExportText(const FProperty* Property, const void* ValuePtr);

	static bool ImportText(const FString& Text, const FProperty* Property, void* ValuePtr);

	static bool ValuesEqual(const FProperty* Property, const void* A, const void* B);

	static UClass* ResolveClass(const FString& Path, FString& OutError);

	static UObject* ResolveObject(const FString& Path, const FObjectPropertyBase* Property, FString& OutError);

	static bool IsBitmaskProperty(const FProperty* Property);

	static const UEnum* GetBitmaskEnum(const FProperty* Property);

	static bool ResolveBitmaskFlags(const FKDFNode& Node, const UEnum* Enum, int64& OutValue, FString& OutError);
};
