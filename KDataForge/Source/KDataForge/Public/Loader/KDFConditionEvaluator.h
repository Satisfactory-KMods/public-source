// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FKDFNode;
class UGameInstance;

class KDATAFORGE_API FKDFConditionEvaluator
{
public:

	static bool Evaluate(const FKDFNode* ConditionsNode, const FKDFNode* ConditionBehaviorNode,
						 UGameInstance* GameInstance, FString& OutFailReason);
};
