//

#include "Subsystems/HelperClasses/KBFLCDOOverwriteBase.h"
#include "Logging/StructuredLog.h"
#include "Subsystems/HelperClasses/KBFLWorldCDOCallRequirement.h"
#include "Subsystems/KBFLContentCDOHelperSubsystem.h"

void UKBFLCDOOverwriteBase::TryApplyToClass(UClass* NewClass)
{
	if (!bEnabled || !bWasApplied || !IsValid(NewClass) || !IsValid(mSubsystem))
	{
		return;
	}
	if (!NewClass->HasAnyClassFlags(CLASS_CompiledFromBlueprint))
	{
		return;
	}
	if (!ShouldCallForInstance(NewClass))
	{
		return;
	}
	UObject* CDO = NewClass->GetDefaultObject();
	if (!IsValid(CDO))
	{
		return;
	}

	Requirements_NotifyOnModify(CDO);
	ApplyToInstance(CDO);
	Requirements_NotifyOnModified(CDO);
	mSubsystem->StoreObject(CDO);
}

void UKBFLCDOOverwriteBase::Start()
{
	if (!bEnabled)
	{
		const FString AssetPath = GetPathName();
		UE_LOGFMT(LogKBFLCDOOverwrite, Warning,
				  "ApplyToInstance: Skip {0} because it is disabled. | Asset: {AssetPath}", *GetName(), AssetPath);
		return;
	}

	if (bWasApplied)
	{
		const FString AssetPath = GetPathName();
		UE_LOGFMT(LogKBFLCDOOverwrite, Warning,
				  "ApplyToInstances: Overrides have already been applied for '{0}'. Skipping re-application. | Asset: "
				  "{AssetPath}",
				  *GetName(), AssetPath);
	}

	LoadRequirements();
	ApplyToInstances();
	bWasApplied = true;

	for (UObject* Instance : mAppliedInstances)
	{
		mSubsystem->StoreObject(Instance);
	}

	Requirements_NotifyOnFinishedAll();
	mAppliedInstances.Empty();
}

void UKBFLCDOOverwriteBase::Clear()
{
	mAppliedInstances.Empty();
	mCachedRequirements.Empty();
	mSubsystem = nullptr;
}

bool UKBFLCDOOverwriteBase::Requirements_IsMet(UObject* TargetInstance)
{
	for (UKBFLCDOCallRequirement* Requirement : mCachedRequirements)
	{
		if (Requirement && !Requirement->IsRequirementMet(mSubsystem, this, TargetInstance))
		{
			const FString AssetPath = GetPathName();
			const FString TargetName = TargetInstance ? TargetInstance->GetName() : TEXT("Invalid");
			UE_LOGFMT(LogKBFLCDOOverwrite, Verbose,
					  "Requirements_IsMet: Requirement '{RequirementName}' not met for target '{TargetName}'. Skipping "
					  "application. | Asset: {AssetPath}",
					  *Requirement->GetName(), TargetName, AssetPath);
			return false;
		}
	}
	return true;
}

void UKBFLCDOOverwriteBase::Requirements_NotifyOnModify(UObject* TargetInstance)
{
	for (UKBFLCDOCallRequirement* Requirement : mCachedRequirements)
	{
		if (IsValid(Requirement))
		{
			Requirement->OnModify(mSubsystem, this, TargetInstance);
		}
	}
}

void UKBFLCDOOverwriteBase::Requirements_NotifyOnModified(UObject* TargetInstance)
{
	for (UKBFLCDOCallRequirement* Requirement : mCachedRequirements)
	{
		if (IsValid(Requirement))
		{
			Requirement->OnModified(mSubsystem, this, TargetInstance);
		}
	}
}

void UKBFLCDOOverwriteBase::Requirements_NotifyOnFinishedAll()
{
	for (UKBFLCDOCallRequirement* Requirement : mCachedRequirements)
	{
		if (IsValid(Requirement))
		{
			Requirement->OnFinishedAll(mSubsystem, this);
		}
	}
}

void UKBFLCDOOverwriteBase::LoadRequirements()
{
	mCachedRequirements.Empty();
	for (TSubclassOf<UKBFLCDOCallRequirement> Requirement : mRequirements)
	{
		if (!IsValid(Requirement))
		{
			continue;
		}
		UKBFLCDOCallRequirement* RequirementInstance =
			NewObject<UKBFLCDOCallRequirement>(this, Requirement, NAME_None, RF_Public | RF_Transactional);
		RequirementInstance->mSubsystem = mSubsystem;
		mCachedRequirements.Add(RequirementInstance);
	}
}

void UKBFLCDOOverwriteWorldBasedBase::Start()
{
	const FString AssetPath = GetPathName();
	if (!bEnabled)
	{
		UE_LOGFMT(LogKBFLCDOOverwrite, Warning,
				  "ApplyToInstance: Skip {0} because it is disabled. | Asset: {AssetPath}", *GetName(), AssetPath);
		return;
	}

	// We only want to run this if we have a valid world
	if (!IsValid(mWorld))
	{
		UE_LOGFMT(LogKBFLCDOOverwrite, Warning,
				  "ApplyToInstances: Skip {0} because no valid world is set. | Asset: {AssetPath}", *GetName(),
				  AssetPath);
		return;
	}

	if (!bWasApplied)
	{
		Super::Start();
	}

	ApplyToActorsInWorld();
}

void UKBFLCDOOverwriteWorldBasedBase::Clear()
{
	Super::Clear();
	mWorld = nullptr;
	bWasApplied = false;
	mTickableRequirements.Empty();
}

void UKBFLCDOOverwriteWorldBasedBase::LoadRequirements()
{
	mCachedRequirements.Empty();
	for (TSubclassOf<UKBFLCDOCallRequirement> Requirement : mRequirements)
	{
		if (!IsValid(Requirement))
		{
			continue;
		}
		UKBFLCDOCallRequirement* RequirementInstance =
			NewObject<UKBFLCDOCallRequirement>(this, Requirement, NAME_None, RF_Public | RF_Transactional);
		RequirementInstance->mSubsystem = mSubsystem;
		mCachedRequirements.Add(RequirementInstance);

		if (UKBFLWorldCDOCallRequirement* World = Cast<UKBFLWorldCDOCallRequirement>(RequirementInstance))
		{
			World->mWorld = GetWorld();
			mTickableRequirements.Add(World);
		}
	}
}

void UKBFLCDOOverwriteWorldBasedBase::Tick(float dt)
{
	for (UKBFLWorldCDOCallRequirement* CachedRequirement : mTickableRequirements)
	{
		CachedRequirement->Tick(dt, this);
	}
}

UWorld* UKBFLCDOOverwriteWorldBasedBase::GetWorld() const { return mWorld; }
