

#pragma once

#include "CoreMinimal.h"
#include "KBFLCDOCallRequirement.generated.h"

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

	UFUNCTION(BlueprintNativeEvent)
	void DeferedCall(class UKBFLContentCDOHelperSubsystem* Subsystem, class UKBFLCDOOverwriteBase* From,
					 UObject* Target);

	UFUNCTION(BlueprintNativeEvent)
	bool ShouldCallDefered(class UKBFLContentCDOHelperSubsystem* Subsystem, class UKBFLCDOOverwriteBase* From,
						   UObject* Target);

	void DispatchDeferedCall(class UKBFLContentCDOHelperSubsystem* Subsystem, class UKBFLCDOOverwriteBase* From,
							 UObject* Target);

	UPROPERTY(Transient)
	TObjectPtr<class UKBFLContentCDOHelperSubsystem> mSubsystem;

	virtual class UWorld* GetWorld() const override;
};
