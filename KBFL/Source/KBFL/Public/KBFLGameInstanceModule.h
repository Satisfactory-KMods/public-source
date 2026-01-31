// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Interfaces/KBFLContentCDOHelperInterface.h"
#include "Module/GameInstanceModule.h"
#include "UObject/Object.h"

#include "KBFLGameInstanceModule.generated.h"

/**
 *
 */
UCLASS()
class KBFL_API UKBFLGameInstanceModule : public UGameInstanceModule, public IKBFLContentCDOHelperInterface
{
	GENERATED_BODY()

public:
	UKBFLGameInstanceModule();

	virtual void FindAllCDOs();

	// BEGIN IKBFLContentCDOHelperInterface
	virtual FKBFLCDOInformation GetCDOInformationFromPhase_Implementation(ELifecyclePhase Phase,
																		  bool& HasPhase) override;

	// END IKBFLContentCDOHelperInterface

	// BEGIN UGameInstanceModule
	virtual void DispatchLifecycleEvent(ELifecyclePhase Phase) override;
	// END UGameInstanceModule

	bool IsOwnerModObject(UObject* Object) const;

	UFUNCTION(BlueprintNativeEvent, Category = "LifecyclePhase")
	void ConstructionPhase();

	UFUNCTION(BlueprintNativeEvent, Category = "LifecyclePhase")
	void InitPhase();

	UFUNCTION(BlueprintNativeEvent, Category = "LifecyclePhase")
	void PostInitPhase();

	// END UGameInstanceModule

	UFUNCTION(BlueprintCallable, Category = "LifecyclePhase")
	virtual void ConstructionPhase_Delayed();

	UFUNCTION(BlueprintCallable, Category = "LifecyclePhase")
	virtual void InitPhase_Delayed();

	UFUNCTION(BlueprintCallable, Category = "LifecyclePhase")
	virtual void PostInitPhase_Delayed();

	/** Information for CDO's */
	// Our CDO map for all Phases
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|CDOHelper")
	TMap<ELifecyclePhase, FKBFLCDOInformation> mCDOInformationMap;

	// Cached all Called Phases (Delayed) to prevent multi call
	TArray<ELifecyclePhase> Called;
	/** Information for CDO's */


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|AssetRegistry")
	bool mUseAssetRegistry = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|AssetRegistry",
			  meta = (EditCondition = mUseAssetRegistry))
	bool mRegisterCDOs = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|AssetRegistry",
			  meta = (EditCondition = mUseAssetRegistry))
	bool mRegisterAGS = true;

	bool bScanForCDOsDone = false;

	/**
	 * Path for automatic find classes to register
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|AssetRegistry",
			  meta = (EditCondition = mRegisterCDOs))
	TArray<TSubclassOf<UObject>> mBlacklistedCDOClasses;
};
