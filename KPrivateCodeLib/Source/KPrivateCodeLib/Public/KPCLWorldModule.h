// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "KBFLWorldModule.h"

#include "KPCLWorldModule.generated.h"

/** This is a full native version for UKBFLWorldModule. */
UCLASS(Blueprintable)
class KPRIVATECODELIB_API UKPCLWorldModule : public UKBFLWorldModule
{
	GENERATED_BODY()

public:
	UKPCLWorldModule();

	//~ Begin UKBFLWorldModule Interface
	virtual void InitPhase_Implementation() override;
	//~ End UKBFLWorldModule Interface

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Options")
	bool IsEasyNodesEnabled();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Options")
	void ApplyEasyNodes(const TArray<TSubclassOf<UFGResearchTree>>& Nodes);

	UPROPERTY(EditDefaultsOnly, Category = "KMods|ADA")
	bool mUseEasyNodes = true;
};
