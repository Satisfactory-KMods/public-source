#pragma once
#include "Subsystems/HelperClasses/KBFL_CDOHelperClass_Base.h"

#include "Equipment/FGBuildGun.h"
#include "Subsystems/KBFLContentCDOHelperSubsystem.h"

#if WITH_ENGINE
UWorld* UKBFL_CDOHelperClass_Base::GetWorld() const
{
	if (mSubsystem)
	{
		if (mSubsystem->GetWorld())
		{
			return mSubsystem->GetWorld();
		}
	}
	return Super::GetWorld();
}
#endif

void UKBFL_CDOHelperClass_Base::DoCDO()
{
	// Store all CDOs in Subsystem
	for (UClass* Class : GetClasses())
	{
		if (IsValid(Class))
		{
			mSubsystem->GetAndStoreDefaultObject_Native<UObject>(Class);
		}
	}
}

TArray<UClass*> UKBFL_CDOHelperClass_Base::GetClasses() { return {}; }

void UKBFL_CDOHelperClass_Base::GetDefaultObjects(TArray<UObject*>& CDOs)
{
	for (UClass* Class : GetClasses())
	{
		if (IsValid(Class))
		{
			CDOs.Add(mSubsystem->GetAndStoreDefaultObject_Native<UObject>(Class));
		}
	}
}

bool UKBFL_CDOHelperClass_Base::IsValidSoftClass(TSoftClassPtr<> Class)
{
	return !Class.IsNull() || Class.IsPending() || Class.IsValid();
}

bool UKBFL_CDOHelperClass_Base::HasAuth()
{
	if (ensureAlways(mSubsystem && mSubsystem->GetWorld()))
	{
		return mSubsystem->GetWorld()->GetAuthGameMode() != nullptr;
	}
	return false;
}

bool UKBFL_CDOHelperClass_Base::ContainBuildGun(TSubclassOf<UFGRecipe> Subclass)
{
	if (Subclass)
	{
		TArray<TSubclassOf<UObject>> ProductedIn = UFGRecipe::GetProducedIn(Subclass);

		for (const auto Object : ProductedIn)
		{
			if (Object->IsChildOf(AFGBuildGun::StaticClass()))
			{
				return true;
			}
		}
	}

	return false;
}

bool UKBFL_CDOHelperClass_Base::ExecuteAllowed_Implementation() const { return true; }
