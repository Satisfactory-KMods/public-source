#include "Content/KDFRecipeRemoval.h"

#include "Engine/World.h"
#include "FGCustomizationRecipe.h"
#include "FGRecipeManager.h"
#include "KDFLogging.h"
#include "Subsystems/KDFSubsystem.h"

bool FKDFRecipeRemoval::HasAnyRemovalRules(const UObject* WorldContext)
{
	const UKDFSubsystem* Subsystem = UKDFSubsystem::Get(WorldContext);
	return Subsystem != nullptr && !Subsystem->GetContentRemovals().IsEmpty();
}

bool FKDFRecipeRemoval::IsRecipeRemoved(const UObject* WorldContext, TSubclassOf<UFGRecipe> Recipe)
{
	if (Recipe == nullptr)
	{
		return false;
	}
	const UKDFSubsystem* Subsystem = UKDFSubsystem::Get(WorldContext);
	if (Subsystem == nullptr || Subsystem->GetContentRemovals().IsEmpty())
	{
		return false;
	}
	for (const FKDFContentRemoval& Removal : Subsystem->GetContentRemovals())
	{
		if (Removal.mKind == EKDFContentRegKind::Recipe && Removal.mContentClass.Get() == Recipe.Get())
		{
			return true;
		}
	}
	return false;
}

void FKDFRecipeRemoval::SweepSaveRestoredRecipes(UWorld& World)
{

	if (World.GetNetMode() == NM_Client)
	{
		return;
	}
	const UKDFSubsystem* Subsystem = UKDFSubsystem::Get(&World);
	if (Subsystem == nullptr || Subsystem->GetContentRemovals().IsEmpty())
	{
		return;
	}
	AFGRecipeManager* Manager = AFGRecipeManager::Get(&World);
	if (Manager == nullptr)
	{

		UE_LOG(LogKDataForge, Warning,
			   TEXT("Content removal: %d removal(s) configured but AFGRecipeManager does not exist yet in world "
					"'%s' — save-restored recipes were NOT swept"),
			   Subsystem->GetContentRemovals().Num(), *World.GetName());
		return;
	}

	TArray<TSubclassOf<UFGRecipe>> Matched;
	for (const FKDFContentRemoval& Removal : Subsystem->GetContentRemovals())
	{
		UClass* RecipeClass = Removal.mKind == EKDFContentRegKind::Recipe ? Removal.mContentClass.Get() : nullptr;
		if (IsValid(RecipeClass) && Manager->GetAllAvailableRecipes().Contains(RecipeClass))
		{
			Matched.Add(RecipeClass);
		}
	}
	if (Matched.IsEmpty())
	{
		return;
	}

	Manager->RemoveAvailableRecipes(Matched);
	for (const TSubclassOf<UFGRecipe>& Recipe : Matched)
	{

		if (const TSubclassOf<UFGCustomizationRecipe> Customization = Recipe.Get(); Customization != nullptr)
		{

			Manager->mAvailableCustomizationRecipes.Remove(Customization);
			Manager->mAvailableCustomizationRecipesLookup.Remove(Customization);
		}
	}

	Manager->RebuildDerivedAvailableRecipesData();
	Manager->RebuildAvailableItemDescriptorLookup();

	UE_LOG(LogKDataForge, Log, TEXT("Content removal: cleared %d recipe(s) restored from the save"), Matched.Num());
}
