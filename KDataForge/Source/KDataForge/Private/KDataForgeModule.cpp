// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#include "KDataForgeModule.h"

#include "Content/KDFRecipeRemoval.h"
#include "FGRecipeManager.h"
#include "KDFLogging.h"
#include "Modules/ModuleManager.h"
#include "Patching/NativeHookManager.h"

DEFINE_LOG_CATEGORY(LogKDataForge);

void FKDataForgeModule::StartupModule()
{
#if !WITH_EDITOR

	SUBSCRIBE_METHOD(AFGRecipeManager::AddAvailableRecipe,
					 [](auto& Scope, AFGRecipeManager* Self, TSubclassOf<UFGRecipe> Recipe)
					 {
						 if (FKDFRecipeRemoval::IsRecipeRemoved(Self, Recipe))
						 {
							 Scope.Cancel();
						 }
					 });

	SUBSCRIBE_METHOD(AFGRecipeManager::AddAvailableRecipes,
					 [](auto& Scope, AFGRecipeManager* Self, TArray<TSubclassOf<UFGRecipe>> Recipes)
					 {

						 if (!FKDFRecipeRemoval::HasAnyRemovalRules(Self))
						 {
							 return;
						 }

						 TArray<TSubclassOf<UFGRecipe>> Kept;
						 Kept.Reserve(Recipes.Num());
						 for (const TSubclassOf<UFGRecipe>& Recipe : Recipes)
						 {
							 if (!FKDFRecipeRemoval::IsRecipeRemoved(Self, Recipe))
							 {
								 Kept.Add(Recipe);
							 }
						 }

						 if (Kept.Num() == Recipes.Num())
						 {

							 return;
						 }

						 Scope.Cancel();
						 if (!Kept.IsEmpty())
						 {
							 Self->AddAvailableRecipes(Kept);
						 }
					 });
#endif
}

void FKDataForgeModule::ShutdownModule() {}

IMPLEMENT_MODULE(FKDataForgeModule, KDataForge)
