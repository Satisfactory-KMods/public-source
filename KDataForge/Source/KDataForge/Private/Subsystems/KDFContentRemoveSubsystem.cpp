// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#include "Subsystems/KDFContentRemoveSubsystem.h"

#include "Content/KDFRecipeRemoval.h"
#include "Engine/World.h"
#include "KDFLogging.h"
#include "Subsystems/KDFSubsystem.h"

UKDFContentRemoveSubsystem* UKDFContentRemoveSubsystem::Get(const UObject* WorldContext)
{
	if (!IsValid(WorldContext))
	{
		return nullptr;
	}

	const UWorld* World = WorldContext->GetWorld();
	if (World == nullptr)
	{
		return nullptr;
	}
	return World->GetSubsystem<UKDFContentRemoveSubsystem>();
}

bool UKDFContentRemoveSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UKDFContentRemoveSubsystem::ApplyWorldRemovals()
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}
	const UKDFSubsystem* Subsystem = UKDFSubsystem::Get(World);
	if (Subsystem == nullptr || Subsystem->GetContentRemovals().IsEmpty())
	{

		return;
	}

	int32 RecipeRules = 0;
	int32 SchematicRules = 0;
	int32 ResearchTreeRules = 0;
	for (const FKDFContentRemoval& Removal : Subsystem->GetContentRemovals())
	{

		if (!IsValid(Removal.mContentClass.Get()))
		{
			continue;
		}
		switch (Removal.mKind)
		{
		case EKDFContentRegKind::Recipe:
			++RecipeRules;
			break;
		case EKDFContentRegKind::Schematic:
			++SchematicRules;
			break;
		case EKDFContentRegKind::ResearchTree:
			++ResearchTreeRules;
			break;
		default:
			break;
		}
	}

	if (bRemovalsApplied && RecipeRules == mRecipeRulesApplied && SchematicRules == mSchematicRulesApplied &&
		ResearchTreeRules == mResearchTreeRulesApplied)
	{
		UE_LOG(LogKDataForge, Verbose,
			   TEXT("Content removal: rule set unchanged for world '%s' — skipping repeated sweep"), *World->GetName());
		return;
	}

	mRecipeRulesApplied = RecipeRules;
	mSchematicRulesApplied = SchematicRules;
	mResearchTreeRulesApplied = ResearchTreeRules;
	bRemovalsApplied = true;

	FKDFRecipeRemoval::SweepSaveRestoredRecipes(*World);

	UE_LOG(LogKDataForge, Log,
		   TEXT("Content removal applied for world '%s': %d recipe rule(s), %d schematic rule(s), %d research "
				"tree rule(s)"),
		   *World->GetName(), mRecipeRulesApplied, mSchematicRulesApplied, mResearchTreeRulesApplied);
}

bool UKDFContentRemoveSubsystem::IsRecipeRemoved(TSubclassOf<UFGRecipe> Recipe) const
{

	return FKDFRecipeRemoval::IsRecipeRemoved(this, Recipe);
}

bool UKDFContentRemoveSubsystem::IsContentRemoved(const UClass* ContentClass, EKDFContentRegKind Kind) const
{
	if (ContentClass == nullptr || Kind == EKDFContentRegKind::None)
	{
		return false;
	}
	const UKDFSubsystem* Subsystem = UKDFSubsystem::Get(this);
	if (Subsystem == nullptr || Subsystem->GetContentRemovals().IsEmpty())
	{

		return false;
	}
	for (const FKDFContentRemoval& Removal : Subsystem->GetContentRemovals())
	{

		if (Removal.mKind == Kind && Removal.mContentClass.Get() == ContentClass)
		{
			return true;
		}
	}
	return false;
}

int32 UKDFContentRemoveSubsystem::GetAppliedRemovalCount(EKDFContentRegKind Kind) const
{
	switch (Kind)
	{
	case EKDFContentRegKind::Recipe:
		return mRecipeRulesApplied;
	case EKDFContentRegKind::Schematic:
		return mSchematicRulesApplied;
	case EKDFContentRegKind::ResearchTree:
		return mResearchTreeRulesApplied;
	default:
		return 0;
	}
}
