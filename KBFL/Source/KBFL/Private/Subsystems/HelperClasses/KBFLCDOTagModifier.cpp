//
#include "Subsystems/HelperClasses/KBFLCDOTagModifier.h"

#include "Subsystems/KBFLContentCDOHelperSubsystem.h"
#include "UObject/UObjectIterator.h"
#include "UObject/UnrealType.h"

void UKBFLCDOTagModifier::ApplyToInstances()
{
	Super::ApplyToInstances();

	TSet<UClass*> Classes;
	CollectClassesToProcess(Classes);

	for (UClass* Class : Classes)
	{
		UObject* DefaultObject = mSubsystem->GetAndStoreDefaultObject(Class);
		if (!IsValid(DefaultObject) || !Requirements_IsMet(DefaultObject))
		{
			continue;
		}

		Requirements_NotifyOnModify(DefaultObject);
		ApplyRulesToInstance(DefaultObject);
		Requirements_NotifyOnModified(DefaultObject);
	}
}

bool UKBFLCDOTagModifier::ShouldCallForInstance(UClass* NewClass)
{
	if (!IsValid(NewClass))
	{
		return false;
	}

	bool bMatch = false;
	for (const TSoftClassPtr<UObject>& TargetClass : mTargetClasses)
	{
		UClass* Target = TargetClass.Get();
		if (!IsValid(Target))
		{
			continue;
		}
		if (Target == NewClass || (bApplyOnSubclasses && NewClass->IsChildOf(Target)))
		{
			bMatch = true;
			break;
		}
	}
	if (!bMatch)
	{
		return false;
	}

	UObject* CDO = NewClass->GetDefaultObject();
	return IsValid(CDO) && Requirements_IsMet(CDO);
}

void UKBFLCDOTagModifier::ApplyToInstance(UObject* Instance)
{
	if (!IsValid(Instance) || !Requirements_IsMet(Instance))
	{
		return;
	}

	ApplyRulesToInstance(Instance);
}

void UKBFLCDOTagModifier::CollectClassesToProcess(TSet<UClass*>& OutClasses) const
{
	for (const TSoftClassPtr<UObject>& TargetClass : mTargetClasses)
	{
		UClass* Target = TargetClass.LoadSynchronous();
		if (!IsValid(Target))
		{
			continue;
		}

		OutClasses.Add(Target);

		if (bApplyOnSubclasses)
		{
			for (TObjectIterator<UClass> It; It; ++It)
			{
				UClass* Candidate = *It;
				if (Candidate != Target && Candidate->IsChildOf(Target) && !Candidate->HasAnyClassFlags(CLASS_Abstract))
				{
					OutClasses.Add(Candidate);
				}
			}
		}
	}
}

bool UKBFLCDOTagModifier::PassesPropertyNameFilter(FName PropertyName) const
{
	return mOnlyPropertyNames.IsEmpty() || mOnlyPropertyNames.Contains(PropertyName);
}

void UKBFLCDOTagModifier::ApplyRulesToInstance(UObject* Instance) const
{
	if (mRules.IsEmpty())
	{
		return;
	}

	for (TFieldIterator<FStructProperty> PropertyIt(Instance->GetClass()); PropertyIt; ++PropertyIt)
	{
		FStructProperty* StructProperty = *PropertyIt;
		if (StructProperty->Struct != FGameplayTagContainer::StaticStruct() ||
			!PassesPropertyNameFilter(StructProperty->GetFName()))
		{
			continue;
		}

		FGameplayTagContainer* Container = StructProperty->ContainerPtrToValuePtr<FGameplayTagContainer>(Instance);
		if (!Container)
		{
			continue;
		}

		for (const FKBFLTagModifierRule& Rule : mRules)
		{
			ApplyRuleToContainer(*Container, Rule);
		}

		UE_LOGFMT(LogKBFLCDOOverwrite, Verbose, "KBFLCDOTagModifier: Applied {0} rule(s) to {1}::{2} | Asset: {3}",
				  mRules.Num(), *Instance->GetClass()->GetName(), *StructProperty->GetName(), *GetPathName());
	}
}

void UKBFLCDOTagModifier::ApplyRuleToContainer(FGameplayTagContainer& Container, const FKBFLTagModifierRule& Rule)
{
	switch (Rule.mAction)
	{
	case EKBFLTagModifierAction::Add:
		Container.AppendTags(Rule.mTags);
		break;
	case EKBFLTagModifierAction::Remove:
		Container.RemoveTags(Rule.mTags);
		break;
	case EKBFLTagModifierAction::Replace:
		Container.RemoveTags(Rule.mTags);
		Container.AppendTags(Rule.mReplacementTags);
		break;
	}
}
