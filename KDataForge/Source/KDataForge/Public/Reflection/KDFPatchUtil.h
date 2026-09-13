// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "KDFTypes.h"

struct FKDFNode;

class KDATAFORGE_API FKDFPatchUtil
{
public:

	static bool ApplyOpsToObject(UObject* Target, const FKDFNode& PropertiesNode, FKDFApplyContext& Context);

	static bool IsDebugEnabled(bool bContextDebug);

	static void LogAppliedOp(const FString& SourceFile, const UObject* Target, const FString& PropertyPath,
							 const FString& OpName, const FString& PreValue, const FString& PostValue);

	static void PostWriteFixups(UObject* Target, const FString& PropertyPath);

	static bool ObjectHasAllTags(UObject* Object, const TArray<FGameplayTag>& RequiredTags,
								 const FString& TagPropertyName);

	static TArray<FString> CollectTargetPaths(const FKDFNode& Patch);
};
