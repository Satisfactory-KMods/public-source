#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "Subsystems/WorldSubsystem.h"
#include "Templates/SubclassOf.h"

#include "Loader/KDFLoaderTypes.h"

#include "KDFContentRemoveSubsystem.generated.h"

class UFGRecipe;

UCLASS()
class KDATAFORGE_API UKDFContentRemoveSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	static UKDFContentRemoveSubsystem* Get(const UObject* WorldContext);

	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

	void ApplyWorldRemovals();

	bool IsRecipeRemoved(TSubclassOf<UFGRecipe> Recipe) const;

	bool IsContentRemoved(const UClass* ContentClass, EKDFContentRegKind Kind) const;

	int32 GetAppliedRemovalCount(EKDFContentRegKind Kind) const;

	bool WereRemovalsApplied() const { return bRemovalsApplied; }

private:

	bool bRemovalsApplied = false;

	int32 mRecipeRulesApplied = 0;
	int32 mSchematicRulesApplied = 0;
	int32 mResearchTreeRulesApplied = 0;
};
