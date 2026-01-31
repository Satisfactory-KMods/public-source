#pragma once

#include "CoreMinimal.h"
#include "FGSchematic.h"
#include "KBFL_CDOHelperClass_RemoverBase.h"
#include "KBFL_CDOHelperClass_SchematicRemover.generated.h"


/**
 *@deprecated: use the new UKBFLCDOOverwrite system instead
 */
UCLASS()
class KBFL_API UKBFL_CDOHelperClass_SchematicRemover : public UKBFL_CDOHelperClass_RemoverBase
{
	GENERATED_BODY()

public:
	virtual void DoCDO() override;
	virtual TArray<UClass*> GetClasses() override;
	TArray<TSubclassOf<UFGRecipe>> GetExcludeClasses();

	/** must be set for CDO */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "CDO Helper")
	TArray<TSoftClassPtr<UFGSchematic>> mSchematics;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "CDO Helper")
	TArray<TSoftClassPtr<UFGRecipe>> mExcludeRecipes;
};
