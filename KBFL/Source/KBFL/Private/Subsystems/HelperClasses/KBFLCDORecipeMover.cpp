//
#include "Subsystems/HelperClasses/KBFLCDORecipeMover.h"

#include "Subsystems/KBFLAssetDataSubsystem.h"
#include "Subsystems/KBFLContentCDOHelperSubsystem.h"

void UKBFLCDORecipeMover::ApplyToInstances()
{
	Super::ApplyToInstances();

	const FString AssetPath = GetPathName();

	TArray<TSubclassOf<UObject>> ProduceIn = LoadSoftClassesArray(mProducedIn);
	TArray<TSubclassOf<UFGRecipe>> RecipesToIgnore = LoadSoftClassesArray(mRecipesToIgnore);
	TSubclassOf<UObject> Target = LoadSoftClass(mSourceTarget);
	if (!IsValid(Target) || ProduceIn.IsEmpty())
	{
		UE_LOGFMT(LogKBFLCDOOverwrite, Error,
				  "KBFLCDORecipeMover: Invalid Target or no ProduceIn classes set! Aborting! {0} | Asset: {AssetPath}",
				  GetName(), AssetPath);
		return;
	}

	UKBFLAssetDataSubsystem* AssetSubsystem = mSubsystem->GetGameInstance()->GetSubsystem<UKBFLAssetDataSubsystem>();

	for (TSubclassOf<UFGRecipe> Recipe : AssetSubsystem->GetAllRecipes())
	{
		TArray<TSubclassOf<UObject>> ProducedInRecipe = UFGRecipe::GetProducedIn(Recipe);
		if (ProducedInRecipe.Contains(Target))
		{
			UFGRecipe* RecipeCDO = mSubsystem->GetAndStoreDefaultObject_Native<UFGRecipe>(Recipe);

			if (!Requirements_IsMet(Target))
			{
				continue;
			}

			Requirements_NotifyOnModify(RecipeCDO);

			RecipeCDO->mProducedIn.Remove(mSourceTarget);
			if (bReplace)
			{
				RecipeCDO->mProducedIn.Empty();
			}

			for (TSubclassOf<UObject> In : ProduceIn)
			{
				TSoftClassPtr<UObject> TargetSoft{In};
				RecipeCDO->mProducedIn.Add(TargetSoft);
			}

			UE_LOGFMT(LogKBFLCDOOverwrite, Warning,
					  "KBFLCDORecipeMover: Moved Recipe {0} from {1} to {2} | Asset: {AssetPath}", *Recipe->GetName(),
					  *mSourceTarget.GetAssetName(),
					  *FString::JoinBy(ProduceIn, TEXT(", "), [](TSubclassOf<UObject> Cls)
									   { return Cls ? Cls->GetName() : FString("Invalid"); }),
					  AssetPath);

			Requirements_NotifyOnModified(RecipeCDO);
		}
	}
}

bool UKBFLCDORecipeMover::ShouldCallForInstance(UClass* NewClass)
{
	if (!IsValid(NewClass) || !NewClass->IsChildOf(UFGRecipe::StaticClass()))
	{
		return false;
	}

	TSubclassOf<UObject> Target = LoadSoftClass(mSourceTarget);
	if (!IsValid(Target) || mProducedIn.IsEmpty())
	{
		return false;
	}

	TSubclassOf<UFGRecipe> Recipe = NewClass;
	if (!UFGRecipe::GetProducedIn(Recipe).Contains(Target))
	{
		return false;
	}

	for (const TSoftClassPtr<UFGRecipe>& Ignore : mRecipesToIgnore)
	{
		if (Ignore.Get() == NewClass)
		{
			return false;
		}
	}

	// Matches the bulk path, which guards on the source target's requirements.
	return Requirements_IsMet(Target);
}

void UKBFLCDORecipeMover::ApplyToInstance(UObject* Instance)
{
	UFGRecipe* RecipeCDO = Cast<UFGRecipe>(Instance);
	if (!IsValid(RecipeCDO))
	{
		return;
	}

	TArray<TSubclassOf<UObject>> ProduceIn = LoadSoftClassesArray(mProducedIn);

	RecipeCDO->mProducedIn.Remove(mSourceTarget);
	if (bReplace)
	{
		RecipeCDO->mProducedIn.Empty();
	}

	for (TSubclassOf<UObject> In : ProduceIn)
	{
		TSoftClassPtr<UObject> TargetSoft{In};
		RecipeCDO->mProducedIn.Add(TargetSoft);
	}
}
