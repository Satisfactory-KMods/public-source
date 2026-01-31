#pragma once
#include "Subsystems/HelperClasses/KBFL_CDOHelperClass_RemoverBase.h"

#include "FGCustomizationRecipe.h"
#include "Reflection/ReflectionHelper.h"
#include "Registry/ModContentRegistry.h"
#include "Resources/FGBuildDescriptor.h"
#include "Subsystems/KBFLContentCDOHelperSubsystem.h"
#include "Unlocks/FGUnlockRecipe.h"


DECLARE_LOG_CATEGORY_EXTERN(HelperClassRemoverBaseLog, Log, All)

DEFINE_LOG_CATEGORY(HelperClassRemoverBaseLog)

bool UKBFL_CDOHelperClass_RemoverBase::RemoveSchematic(
	TSubclassOf<UFGSchematic> Subclass, AFGRecipeManager* RecipeSubsystem, AFGSchematicManager* SchematicSubsystem,
	TArray<TSubclassOf<UFGSchematic>> Exclude, TArray<FString> ExcludeStrings,
	TArray<TSubclassOf<UFGRecipe>> ExcludeRecipes, TArray<FString> ExcludeRecipesStrings,
	TArray<UClass*>& RemovedClasses)
{
	if (Subclass && RecipeSubsystem && SchematicSubsystem && !Exclude.Contains(Subclass))
	{
		if (CheckClassString(Subclass, ExcludeStrings))
		{
			TArray<UFGUnlock*> Unlocks = UFGSchematic::GetUnlocks(Subclass);
			for (auto Unlock : Unlocks)
			{
				if (Unlock)
				{
					if (UFGUnlockRecipe* RecipeUnlock = Cast<UFGUnlockRecipe>(Unlock))
					{
						for (auto Recipe : RecipeUnlock->GetRecipesToUnlock())
						{
							if (Recipe)
							{
								UE_LOG(HelperClassRemoverBaseLog, Log,
									   TEXT("RemoveSchematic > Found Recipe try to remove %s"), *Recipe->GetName());
								RemoveRecipe(Recipe, RecipeSubsystem, ExcludeRecipes, ExcludeRecipesStrings,
											 RemovedClasses);
							}
						}
					}
				}
			}
			if (SchematicSubsystem->mActiveSchematic == Subclass)
			{
				UE_LOG(HelperClassRemoverBaseLog, Log, TEXT("RemoveSchematic > mActiveSchematic Remove %s"),
					   *Subclass->GetName());
				SchematicSubsystem->mActiveSchematic = nullptr;
				SchematicSubsystem->mOnActiveSchematicChanged.Broadcast(nullptr);
			}

			if (SchematicSubsystem->mAllSchematics.Contains(Subclass))
			{
				SchematicSubsystem->mAllSchematics.Remove(Subclass);
				UE_LOG(HelperClassRemoverBaseLog, Log, TEXT("RemoveSchematic > Remove mAllSchematics %s"),
					   *Subclass->GetName());
			}

			if (SchematicSubsystem->mPurchasedSchematics.Contains(Subclass))
			{
				SchematicSubsystem->mPurchasedSchematics.Remove(Subclass);
				UE_LOG(HelperClassRemoverBaseLog, Log, TEXT("RemoveSchematic > Remove mPurchasedSchematics %s"),
					   *Subclass->GetName());
			}

			// Subclass.GetDefaultObject()->mType = ESchematicType::EST_Custom;

			RemovedClasses.AddUnique(Subclass);
			return true;
		}
	}
	return false;
}

// from Content reg (SML)
void UKBFL_CDOHelperClass_RemoverBase::ExtractSchematicsFromResearchTree(
	TSubclassOf<UFGResearchTree> ResearchTree, TArray<TSubclassOf<UFGSchematic>>& OutSchematics)
{
	// Lazily initialize research tree node reflection properties for faster access
	UClass* ResearchTreeNodeClass = LoadClass<UFGResearchTreeNode>(
		nullptr, TEXT("/Game/FactoryGame/Schematics/Research/BPD_ResearchTreeNode.BPD_ResearchTreeNode_C"));
	check(ResearchTreeNodeClass);

	FStructProperty* NodeDataStructProperty =
		FReflectionHelper::FindPropertyChecked<FStructProperty>(ResearchTreeNodeClass, TEXT("mNodeDataStruct"));
	check(NodeDataStructProperty);

	FClassProperty* SchematicStructProperty = FReflectionHelper::FindPropertyByShortNameChecked<FClassProperty>(
		NodeDataStructProperty->Struct, TEXT("Schematic"));
	check(SchematicStructProperty);

	UFGResearchTree* DefaultResearchTree = ResearchTree.GetDefaultObject();
	if (DefaultResearchTree)
	{
		const TArray<UFGResearchTreeNode*> Nodes = DefaultResearchTree->mNodes;
		for (UFGResearchTreeNode* Node : Nodes)
		{
			if (!Node->IsA(ResearchTreeNodeClass))
			{
				continue;
			}
			const void* NodeDataStructPtr = NodeDataStructProperty->ContainerPtrToValuePtr<void>(Node);
			UClass* SchematicClass =
				Cast<UClass>(SchematicStructProperty->GetPropertyValue_InContainer(NodeDataStructPtr));
			if (SchematicClass == nullptr)
			{
				continue;
			}
			OutSchematics.Add(SchematicClass);
		}
	}
}

bool UKBFL_CDOHelperClass_RemoverBase::CheckClassString(UClass* Class, TArray<FString> Strings)
{
	if (!Class)
	{
		return false;
	}

	if (Strings.Num() == 0)
	{
		return true;
	}

	FString ClassName = Class->GetName();
	for (FString String : Strings)
	{
		if (ClassName.Contains(String))
		{
			return false;
		}
	}

	return true;
}

bool UKBFL_CDOHelperClass_RemoverBase::RemoveRecipe(TSubclassOf<UFGRecipe> Subclass, AFGRecipeManager* RecipeSubsystem,
													TArray<TSubclassOf<UFGRecipe>> Exclude,
													TArray<FString> ExcludeStrings, TArray<UClass*>& RemovedClasses)
{
	if (Subclass && RecipeSubsystem && !Exclude.Contains(Subclass))
	{
		if (CheckClassString(Subclass, ExcludeStrings))
		{
			if (ContainBuildGun(Subclass))
			{
				TArray<FItemAmount> Products = UFGRecipe::GetProducts(Subclass);
				if (Products.IsValidIndex(0))
				{
					FItemAmount Item = Products[0];
					if (Item.ItemClass)
					{
						if (Item.ItemClass->IsChildOf(UFGBuildDescriptor::StaticClass()))
						{
							TSubclassOf<AActor> BuildingClass =
								UFGBuildDescriptor::GetBuildClass(TSubclassOf<UFGBuildDescriptor>(Subclass));
							if (BuildingClass && RecipeSubsystem->mAvailableBuildings.Contains(BuildingClass) &&
								BuildingClass->IsChildOf(AFGBuildable::StaticClass()))
							{
								UE_LOG(HelperClassRemoverBaseLog, Log, TEXT("RemoveRecipe > remove buildingclass %s"),
									   *BuildingClass->GetName());
								RecipeSubsystem->mAvailableBuildings.Remove(TSubclassOf<AFGBuildable>(BuildingClass));
							}
						}
					}
				}
			}

			if (Subclass->IsChildOf(UFGCustomizationRecipe::StaticClass()))
			{
				UE_LOG(HelperClassRemoverBaseLog, Log,
					   TEXT("Try RemoveRecipe > IsChildOf(UFGCustomizationRecipe::StaticClass())"));
				TSubclassOf<UFGCustomizationRecipe> CustomizationSubClass =
					TSubclassOf<UFGCustomizationRecipe>(Subclass);
				if (RecipeSubsystem->mAvailableCustomizationRecipes.Contains(CustomizationSubClass))
				{
					UE_LOG(HelperClassRemoverBaseLog, Log, TEXT("RemoveRecipe > remove Customization %s"),
						   *CustomizationSubClass->GetName());
					RecipeSubsystem->mAvailableCustomizationRecipes.Remove(CustomizationSubClass);
				}
			}

			if (RecipeSubsystem->mAvailableRecipes.Contains(Subclass))
			{
				UE_LOG(HelperClassRemoverBaseLog, Log, TEXT("RemoveRecipe > mAvailableRecipes remove Recipe %s"),
					   *Subclass->GetName());
				RecipeSubsystem->mAvailableRecipes.Remove(Subclass);
			}

			RemovedClasses.AddUnique(Subclass);
			return true;
		}
	}
	return false;
}

bool UKBFL_CDOHelperClass_RemoverBase::RemoveResearchTree(
	TSubclassOf<UFGResearchTree> Subclass, AFGResearchManager* ResearchManager, AFGRecipeManager* RecipeSubsystem,
	AFGSchematicManager* SchematicSubsystem, TArray<TSubclassOf<UFGResearchTree>> Exclude,
	TArray<FString> ExcludeStrings, TArray<TSubclassOf<UFGRecipe>> ExcludeRecipes,
	TArray<FString> ExcludeRecipesStrings, TArray<TSubclassOf<UFGSchematic>> ExcludeSchematics,
	TArray<FString> ExcludeSchematicsStrings, TArray<UClass*>& RemovedClasses)
{
	if (Subclass && ResearchManager && RecipeSubsystem && SchematicSubsystem && !Exclude.Contains(Subclass))
	{
		if (CheckClassString(Subclass, ExcludeStrings))
		{
			TArray<TSubclassOf<UFGSchematic>> Schematics;
			ExtractSchematicsFromResearchTree(Subclass, Schematics);

			for (auto Schematic : Schematics)
			{
				if (Schematic)
				{
					UE_LOG(HelperClassRemoverBaseLog, Log, TEXT("RemoveResearchTree RemoveSchematic"));
					RemoveSchematic(Schematic, RecipeSubsystem, SchematicSubsystem, ExcludeSchematics,
									ExcludeSchematicsStrings, ExcludeRecipes, ExcludeRecipesStrings, RemovedClasses);
				}
			}

			if (ResearchManager->mAvailableResearchTrees.Contains(Subclass))
			{
				ResearchManager->mAvailableResearchTrees.Remove(Subclass);
				UE_LOG(HelperClassRemoverBaseLog, Log, TEXT("RemoveResearchTree mAvailableResearchTrees > %s"),
					   *Subclass->GetName());
			}

			if (ResearchManager->mUnlockedResearchTrees.Contains(Subclass))
			{
				ResearchManager->mUnlockedResearchTrees.Remove(Subclass);
				UE_LOG(HelperClassRemoverBaseLog, Log, TEXT("RemoveResearchTree mUnlockedResearchTrees > %s"),
					   *Subclass->GetName());
			}

			// Subclass.GetDefaultObject()->mPreUnlockDisplayName = FText::GetEmpty();
			// Subclass.GetDefaultObject()->mDisplayName = FText::GetEmpty();
			// Subclass.GetDefaultObject()->mPreUnlockDescription = FText::GetEmpty();
			// Subclass.GetDefaultObject()->mPostUnlockDescription = FText::GetEmpty();

			ResearchManager->UpdateUnlockedResearchTrees();

			RemovedClasses.AddUnique(Subclass);
			return true;
		}
	}
	return false;
}
