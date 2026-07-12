#pragma once

#include "CoreMinimal.h"

struct FKDFNode;
class UGameInstance;

/**
 * Evaluates a document's `conditions:` block. All listed conditions must pass (logical AND).
 * Supported keys: `gameVersion` (semver range vs FactoryGame version), `hasMod` (mod ref, optional
 * `Ref@<range>` version constraint, scalar or sequence), `modVersion` (map of mod ref to version
 * range), `hasClass` (class path, scalar or sequence).
 */
class KDATAFORGE_API FKDFConditionEvaluator
{
public:
	/**
	 * @param ConditionsNode  The `conditions:` map node; nullptr means "no conditions" → passes.
	 * @param OutFailReason   Human-readable reason for the first failing condition.
	 * @return true when the document should be applied.
	 */
	static bool Evaluate(const FKDFNode* ConditionsNode, UGameInstance* GameInstance, FString& OutFailReason);
};
