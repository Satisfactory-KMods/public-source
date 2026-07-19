//

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "KBFLCDOOverwriteBase.h"

#include "KBFLCDOTagModifier.generated.h"

UENUM(BlueprintType)
enum class EKBFLTagModifierAction : uint8
{
	/** Adds mTags to the property's tag container. */
	Add,
	/** Removes mTags from the property's tag container. */
	Remove,
	/** Removes mTags, then adds mReplacementTags to the property's tag container. */
	Replace
};

USTRUCT(BlueprintType)
struct FKBFLTagModifierRule
{
	GENERATED_BODY()

	/** What to do with mTags on every matched gameplay tag property. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tag Modifier")
	EKBFLTagModifierAction mAction = EKBFLTagModifierAction::Add;

	/** Tags to add (Add), remove (Remove), or remove before applying mReplacementTags (Replace). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tag Modifier")
	FGameplayTagContainer mTags;

	/** Tags added after mTags is removed. Only used when mAction is Replace. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tag Modifier",
			  meta = (EditCondition = "mAction == EKBFLTagModifierAction::Replace", EditConditionHides))
	FGameplayTagContainer mReplacementTags;
};

/**
 * CDO overwrite that adds, removes, or replaces gameplay tags on any vanilla game class exposing a
 * FGameplayTagContainer UPROPERTY (e.g. UFGItemDescriptor::mGameplayTags, UFGRecipe, UFGSchematic,
 * AFGBuildable, UFGResearchTree, ...). The target property is found via reflection, so no per-class
 * property name needs to be known ahead of time.
 */
UCLASS()
class KBFL_API UKBFLCDOTagModifier : public UKBFLCDOOverwriteBase
{
	GENERATED_BODY()

public:
	/** Applies mRules to every gameplay tag container property found on each configured target class CDO. */
	virtual void ApplyToInstances() override;

	// Lazy-load path: re-apply to a single class loaded after the initial CDO pass.
	virtual bool ShouldCallForInstance(UClass* NewClass) override;

	/** Lazy-load handler: applies mRules to a single loaded instance. */
	virtual void ApplyToInstance(UObject* Instance) override;

	// ===== Target Settings =====
	/** Classes to scan for gameplay tag container properties. Works with any vanilla game class. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Target Settings", meta = (AllowAbstract = "true"))
	TArray<TSoftClassPtr<UObject>> mTargetClasses;

	/** If true, also applies to every loaded subclass of each configured target class. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Target Settings")
	bool bApplyOnSubclasses = false;

	/** Optional property name filter. Empty = apply to every FGameplayTagContainer property found on the class. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Target Settings")
	TArray<FName> mOnlyPropertyNames;

	// ===== Rules =====
	/** Tag modifications to apply, in order, to every matched property. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rules")
	TArray<FKBFLTagModifierRule> mRules;

private:
	/** Resolves mTargetClasses (+ subclasses, if enabled) into the concrete set of classes to process. */
	void CollectClassesToProcess(TSet<UClass*>& OutClasses) const;

	/** Runs every configured rule against every matching gameplay tag container property on Instance. */
	void ApplyRulesToInstance(UObject* Instance) const;

	/** Returns true if PropertyName passes the mOnlyPropertyNames filter (empty filter = always true). */
	bool PassesPropertyNameFilter(FName PropertyName) const;

	/** Mutates a single tag container in place according to Rule. */
	static void ApplyRuleToContainer(FGameplayTagContainer& Container, const FKBFLTagModifierRule& Rule);
};
