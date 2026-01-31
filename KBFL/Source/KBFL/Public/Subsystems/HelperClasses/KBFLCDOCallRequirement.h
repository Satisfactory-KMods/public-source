//

#pragma once

#include "CoreMinimal.h"
#include "KBFLCDOCallRequirement.generated.h"

/**
 * This class can be used to create requirements that must be met for a CDO Helper to be called
 * This is mainly used for KBFLCDOOverwrite classes to determine if they should be applied or not
 * Since DataAssets doesn't support blueprint events, we need to use this helper class
 * You can also use this to trigger events when a CDO overwrite is applied
 */
UCLASS(BlueprintType, Blueprintable, HideCategories = (Object))
class KBFL_API UKBFLCDOCallRequirement : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent)
	void OnInit();

	UFUNCTION(BlueprintNativeEvent)
	bool IsRequirementMet(class UKBFLContentCDOHelperSubsystem* Subsystem, class UKBFLCDOOverwriteBase* From,
						  UObject* Target);

	UFUNCTION(BlueprintNativeEvent)
	void OnModify(class UKBFLContentCDOHelperSubsystem* Subsystem, class UKBFLCDOOverwriteBase* From, UObject* Target);

	UFUNCTION(BlueprintNativeEvent)
	void OnModified(class UKBFLContentCDOHelperSubsystem* Subsystem, class UKBFLCDOOverwriteBase* From,
					UObject* Target);

	UFUNCTION(BlueprintNativeEvent)
	void OnFinishedAll(class UKBFLContentCDOHelperSubsystem* Subsystem, class UKBFLCDOOverwriteBase* From);

	UPROPERTY(Transient)
	class UKBFLContentCDOHelperSubsystem* mSubsystem;

	virtual class UWorld* GetWorld() const override;
};
