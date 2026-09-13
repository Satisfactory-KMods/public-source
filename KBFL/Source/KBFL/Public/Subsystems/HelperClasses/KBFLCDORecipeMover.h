// Copyright Kyri123 / KMods 2026. All Rights Reserved.



#pragma once

#include "CoreMinimal.h"
#include "FGRecipe.h"
#include "KBFLCDOOverwriteBase.h"
#include "KBFLCDORecipeMover.generated.h"

UCLASS()
class KBFL_API UKBFLCDORecipeMover : public UKBFLCDOOverwriteBase
{
	GENERATED_BODY()

public:

	virtual void ApplyToInstances() override;

	virtual bool ShouldCallForInstance(UClass* NewClass) override;

	virtual void ApplyToInstance(UObject* Instance) override;

	UPROPERTY(EditAnywhere, Meta = (MustImplement = "FGRecipeProducerInterface"), Category = "Source Settings")
	TSoftClassPtr<UObject> mSourceTarget;

	UPROPERTY(EditAnywhere, Meta = (MustImplement = "FGRecipeProducerInterface"), Category = "Target Settings")
	TArray<TSoftClassPtr<UObject>> mProducedIn;

	UPROPERTY(EditAnywhere, Category = "Target Settings")
	bool bReplace = true;

	UPROPERTY(EditAnywhere, Category = "Exclusions")
	TArray<TSoftClassPtr<UFGRecipe>> mRecipesToIgnore;
};
