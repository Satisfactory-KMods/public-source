//

#pragma once

#include "CoreMinimal.h"
#include "FGRecipe.h"
#include "KBFLCDOOverwriteBase.h"
#include "KBFLCDORecipeMover.generated.h"

/**
 *
 */
UCLASS()
class KBFL_API UKBFLCDORecipeMover : public UKBFLCDOOverwriteBase
{
	GENERATED_BODY()

public:
	virtual void ApplyToInstances() override;

	UPROPERTY(EditAnywhere, Category = "CDO")
	bool bReplace = true;

	UPROPERTY(EditAnywhere, Meta = (MustImplement = "FGRecipeProducerInterface"), Category = "CDO")
	TSoftClassPtr<UObject> mSourceTarget;

	UPROPERTY(EditAnywhere, Category = "CDO")
	TArray<TSoftClassPtr<UFGRecipe>> mRecipesToIgnore;

	UPROPERTY(EditAnywhere, Meta = (MustImplement = "FGRecipeProducerInterface"), Category = "CDO")
	TArray<TSoftClassPtr<UObject>> mProducedIn;
};
