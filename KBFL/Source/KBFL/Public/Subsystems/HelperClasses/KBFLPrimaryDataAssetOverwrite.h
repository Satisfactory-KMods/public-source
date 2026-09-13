// Copyright Kyri123 / KMods 2026. All Rights Reserved.



#pragma once

#include "CoreMinimal.h"
#include "KBFLCDOCallRequirement.h"
#include "KBFLCDOOverwrite.h"
#include "KBFLCDOOverwriteBase.h"
#include "Runtime/CoreUObject/Public/UObject/Object.h"
#include "Runtime/Engine/Classes/Engine/DataAsset.h"

#include "KBFLPrimaryDataAssetOverwrite.generated.h"

UCLASS()
class KBFL_API UKBFLPrimaryDataAssetOverwrite : public UKBFLCDOOverwriteBase
{
	GENERATED_BODY()

public:

	UKBFLPrimaryDataAssetOverwrite();

	virtual void PostLoad() override;

#if WITH_EDITOR

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;

	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;

	void ValidateManualProperties();
#endif

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowAbstract = "true"), Category = "Target Settings")
	TSubclassOf<UDataAsset> mTargetDataAssetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Target Settings",
			  meta = (AllowAbstract = "false", EditCondition = "mTargetDataAssetClass != nullptr", EditConditionHides))
	TSubclassOf<UDataAsset> mAbstractContainerClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Target Settings")
	TObjectPtr<UObject> mTargetDataAssetInstance;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Target Settings")
	bool bTargetOnlyAsContainer = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Target Settings",
			  meta = (AllowAbstract = "false", EditCondition = "bTargetOnlyAsContainer", EditConditionHides))
	TSoftClassPtr<UDataAsset> mRealTargetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowAbstract = "true"), Category = "Target Settings")
	TArray<TSoftClassPtr<UDataAsset>> mOtherTargetClasses;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Target Settings")
	TArray<FString> mFindAssetsInPaths;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Blueprint Handling")
	bool bOnlyApplyOnBlueprints = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Include/Exclude")
	TArray<TSoftObjectPtr<UDataAsset>> mSpecificAssets;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Include/Exclude")
	TArray<TSoftObjectPtr<UDataAsset>> mIgnoreAssets;

	UPROPERTY(EditAnywhere, Instanced, Category = "Property Container")
	TObjectPtr<UObject> mPropertyContainer = nullptr;

	UPROPERTY(EditAnywhere, Category = "Property Overrides", meta = (TitleProperty = "mPropertyName"))
	TSet<FKBFLCDOOverwriteProperty> mModifiedProperties;

	UPROPERTY(EditAnywhere, Category = "Property Overrides", meta = (TitleProperty = "mPropertyName"))
	TSet<FKBFLCDOOverwriteProperty> mManuelPropertiesOverwrite;

private:

	void ApplyToDataAssetInstance(UObject* TargetInstance);

	void ApplyPropertyWithBehavior(FProperty* Property, void* ContainerValuePtr, void* DestValuePtr,
								   const FKBFLCDOOverwriteProperty& PropSettings, const FString& AssetPath,
								   UObject* TargetInstance);

	void ApplyCollectionProperty(FProperty* Property, void* ContainerValuePtr, void* DestValuePtr,
								 EKBFLBehaivor Behavior, const FString& AssetPath, UObject* TargetInstance);

	void ApplyNumericProperty(FProperty* Property, void* ContainerValuePtr, void* DestValuePtr,
							  EKBFLNumericBehavior Behavior, double MinValue, double MaxValue, const FString& AssetPath,
							  UObject* TargetInstance);

	void ApplyBoolProperty(FProperty* Property, void* ContainerValuePtr, void* DestValuePtr, EKBFLBoolBehavior Behavior,
						   const FString& AssetPath, UObject* TargetInstance);

	void ApplyStringProperty(FProperty* Property, void* ContainerValuePtr, void* DestValuePtr,
							 EKBFLStringBehavior Behavior, const FString& Separator, const FString& AssetPath,
							 UObject* TargetInstance);

public:

	virtual void ApplyToInstances() override;

	UFUNCTION(CallInEditor, Category = "Actions")
	void RefreshPropertyContainer();

	UFUNCTION(CallInEditor, Category = "Actions")
	void ValidateAndDetectPropertyTypes();

#if WITH_EDITOR

	void RebuildModifiedProperties();
#endif
};
