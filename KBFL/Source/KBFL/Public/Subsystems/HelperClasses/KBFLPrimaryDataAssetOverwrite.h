//

#pragma once

#include "CoreMinimal.h"
#include "KBFLCDOCallRequirement.h"
#include "KBFLCDOOverwrite.h"
#include "KBFLCDOOverwriteBase.h"
#include "Runtime/CoreUObject/Public/UObject/Object.h"
#include "Runtime/Engine/Classes/Engine/DataAsset.h"

#include "KBFLPrimaryDataAssetOverwrite.generated.h"
/**
 * Specialized CDO Overwrite handler for UPrimaryDataAsset instances.
 * Since UPrimaryDataAsset are treated as object instances rather than CDOs,
 * this class provides special handling for finding and modifying them.
 */
UCLASS()
class KBFL_API UKBFLPrimaryDataAssetOverwrite : public UKBFLCDOOverwriteBase
{
	GENERATED_BODY()

public:
	UKBFLPrimaryDataAssetOverwrite();

	// UObject interface
	virtual void PostLoad() override;
	// End UObject interface

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;

	/** Validate and auto-detect property types for manual property overrides */
	void ValidateManualProperties();
#endif

	/** Target class for DataAsset (UPrimaryDataAsset or derived classes) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowAbstract = "true"), Category = "PrimaryDataAsset")
	TSubclassOf<UPrimaryDataAsset> mTargetDataAssetClass;

	/** Target object instance (alternative to class-based targeting) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PrimaryDataAsset")
	TObjectPtr<UObject> mTargetDataAssetInstance;

	/** Additional target classes to apply overrides to */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowAbstract = "true"), Category = "PrimaryDataAsset")
	TArray<TSoftClassPtr<UPrimaryDataAsset>> mOtherTargetClasses;

	/** If true, mTargetDataAssetClass is only used as a container for properties, not for asset targeting.
	 * Use mRealTargetClass for actual class targeting. This is a workaround for abstract classes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PrimaryDataAsset")
	bool bTargetOnlyAsContainer = false;

	/** The real target class to apply overrides to when bTargetOnlyAsContainer is true.
	 * This allows you to use an abstract class as property container while targeting concrete classes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PrimaryDataAsset",
			  meta = (AllowAbstract = "false", EditCondition = "bTargetOnlyAsContainer", EditConditionHides))
	TSoftClassPtr<UPrimaryDataAsset> mRealTargetClass;

	/** If true, only apply to Blueprint-based PrimaryDataAssets */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PrimaryDataAsset")
	bool bOnlyApplyOnBlueprints = true;

	/** Paths to search for PrimaryDataAsset instances */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PrimaryDataAsset")
	TArray<FString> mFindAssetsInPaths;

	/** Specific DataAsset instances to apply overrides to */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PrimaryDataAsset")
	TArray<TSoftObjectPtr<UPrimaryDataAsset>> mSpecificAssets;

	/** DataAsset instances to ignore/skip */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PrimaryDataAsset")
	TArray<TSoftObjectPtr<UPrimaryDataAsset>> mIgnoreAssets;

	/** Instanced object storing actual overridden property values */
	UPROPERTY(EditAnywhere, Instanced, Category = "PrimaryDataAsset")
	UObject* mPropertyContainer = nullptr;

	/** Properties that have been explicitly modified by the user */
	UPROPERTY(EditAnywhere, Category = "PrimaryDataAsset", meta = (TitleProperty = "mPropertyName"))
	TSet<FKBFLCDOOverwriteProperty> mModifiedProperties;

	/** Manual property overrides - allows you to manually add/edit properties to override */
	UPROPERTY(EditAnywhere, Category = "PrimaryDataAsset", meta = (TitleProperty = "mPropertyName"))
	TSet<FKBFLCDOOverwriteProperty> mManuelPropertiesOverwrite;

private:
	/** Apply changed properties to a DataAsset instance */
	void ApplyToDataAssetInstance(UObject* TargetInstance);

	/** Apply property based on its type and behavior settings */
	void ApplyPropertyWithBehavior(FProperty* Property, void* ContainerValuePtr, void* DestValuePtr,
								   const FKBFLCDOOverwriteProperty& PropSettings, const FString& AssetPath,
								   UObject* TargetInstance);

	/** Apply collection (Array/Set/Map) property */
	void ApplyCollectionProperty(FProperty* Property, void* ContainerValuePtr, void* DestValuePtr,
								 EKBFLBehaivor Behavior, const FString& AssetPath, UObject* TargetInstance);

	/** Apply numeric property with operation */
	void ApplyNumericProperty(FProperty* Property, void* ContainerValuePtr, void* DestValuePtr,
							  EKBFLNumericBehavior Behavior, double MinValue, double MaxValue, const FString& AssetPath,
							  UObject* TargetInstance);

	/** Apply bool property with operation */
	void ApplyBoolProperty(FProperty* Property, void* ContainerValuePtr, void* DestValuePtr, EKBFLBoolBehavior Behavior,
						   const FString& AssetPath, UObject* TargetInstance);

	/** Apply string/text/name property with operation */
	void ApplyStringProperty(FProperty* Property, void* ContainerValuePtr, void* DestValuePtr,
							 EKBFLStringBehavior Behavior, const FString& Separator, const FString& AssetPath,
							 UObject* TargetInstance);

public:
	virtual void ApplyToInstances() override;

	/** Refresh the property container when TargetClass changes */
	UFUNCTION(CallInEditor, Category = "PrimaryDataAsset")
	void RefreshPropertyContainer();

	/** Validate and auto-detect types for manual property overrides */
	UFUNCTION(CallInEditor, Category = "PrimaryDataAsset")
	void ValidateAndDetectPropertyTypes();

#if WITH_EDITOR
	/** Rebuild mModifiedProperties by comparing mPropertyContainer with default instance */
	void RebuildModifiedProperties();
#endif
};
