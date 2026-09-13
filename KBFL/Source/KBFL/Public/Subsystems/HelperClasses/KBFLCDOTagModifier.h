// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "KBFLCDOOverwriteBase.h"

#include "KBFLCDOTagModifier.generated.h"

UENUM(BlueprintType)
enum class EKBFLTagModifierAction : uint8
{

	Add,

	Remove,

	Replace
};

USTRUCT(BlueprintType)
struct FKBFLTagModifierRule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tag Modifier")
	EKBFLTagModifierAction mAction = EKBFLTagModifierAction::Add;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tag Modifier")
	FGameplayTagContainer mTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tag Modifier",
			  meta = (EditCondition = "mAction == EKBFLTagModifierAction::Replace", EditConditionHides))
	FGameplayTagContainer mReplacementTags;
};

UCLASS()
class KBFL_API UKBFLCDOTagModifier : public UKBFLCDOOverwriteBase
{
	GENERATED_BODY()

public:

	virtual void ApplyToInstances() override;

	virtual bool ShouldCallForInstance(UClass* NewClass) override;

	virtual void ApplyToInstance(UObject* Instance) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Target Settings", meta = (AllowAbstract = "true"))
	TArray<TSoftClassPtr<UObject>> mTargetClasses;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Target Settings")
	bool bApplyOnSubclasses = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Target Settings")
	TArray<FName> mOnlyPropertyNames;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rules")
	TArray<FKBFLTagModifierRule> mRules;

private:

	void CollectClassesToProcess(TSet<UClass*>& OutClasses) const;

	void ApplyRulesToInstance(UObject* Instance) const;

	bool PassesPropertyNameFilter(FName PropertyName) const;

	static void ApplyRuleToContainer(FGameplayTagContainer& Container, const FKBFLTagModifierRule& Rule);
};
