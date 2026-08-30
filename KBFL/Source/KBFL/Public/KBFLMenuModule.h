

#pragma once

#include "CoreMinimal.h"
#include "Module/MenuWorldModule.h"

#include "KBFLMenuModule.generated.h"

UCLASS(Blueprintable)
class KBFL_API UKBFLMenuModule : public UMenuWorldModule
{
	GENERATED_BODY()

public:
	UKBFLMenuModule();

	virtual void DispatchLifecycleEvent(ELifecyclePhase Phase) override;

	UFUNCTION(BlueprintNativeEvent, Category = "LifecyclePhase")
	void ConstructionPhase();

	UFUNCTION(BlueprintNativeEvent, Category = "LifecyclePhase")
	void InitPhase();

	UFUNCTION(BlueprintNativeEvent, Category = "LifecyclePhase")
	void PostInitPhase();
};
