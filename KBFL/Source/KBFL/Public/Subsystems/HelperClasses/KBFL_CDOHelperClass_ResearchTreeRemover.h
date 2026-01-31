#pragma once

#include "CoreMinimal.h"
#include "FGSchematic.h"
#include "KBFL_CDOHelperClass_RemoverBase.h"
#include "KBFL_CDOHelperClass_ResearchTreeRemover.generated.h"


/**
 *@deprecated: use the new UKBFLCDOOverwrite system instead
 */
UCLASS()
class KBFL_API UKBFL_CDOHelperClass_ResearchTreeRemover : public UKBFL_CDOHelperClass_RemoverBase
{
	GENERATED_BODY()

public:
	virtual void DoCDO() override;
	virtual TArray<UClass*> GetClasses() override;
	TArray<TSubclassOf<UFGRecipe>> GetExcludeClassesRecipes();
	TArray<TSubclassOf<UFGSchematic>> GetExcludeClassesSchematics();

	/** must be set for CDO */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "CDO Helper")
	TArray<TSoftClassPtr<UFGResearchTree>> mTrees;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "CDO Helper")
	TArray<TSoftClassPtr<UFGSchematic>> mExcludeSchematics;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "CDO Helper")
	TArray<TSoftClassPtr<UFGRecipe>> mExcludeRecipes;
};
