#pragma once

#include "CoreMinimal.h"

#include "KDFTypes.h"
#include "Reflection/KDFPropertyPath.h"

struct FKDFNode;

struct KDATAFORGE_API FKDFOpArgs
{

	const FKDFNode* mValue = nullptr;

	int32 mIndexA = INDEX_NONE;

	int32 mIndexB = INDEX_NONE;

	const FKDFNode* mMin = nullptr;
	const FKDFNode* mMax = nullptr;

	FString mFromPath;
};

class KDATAFORGE_API FKDFOpEngine
{
public:
	static bool ParseOpName(const FString& Name, EKDFOp& OutOp);
	static FString OpToString(EKDFOp Op);

	static bool ParseOpEntry(const FKDFNode& Entry, FString& OutPathString, EKDFOp& OutOp, FKDFOpArgs& OutArgs,
							 FString& OutError);

	static bool IsOpSupported(EKDFOp Op, const FProperty* LeafProperty);

	static bool ApplyOp(UObject* RootObject, const FKDFPropertyPath& Path, EKDFOp Op, const FKDFOpArgs& Args,
						FString& OutError);
};
