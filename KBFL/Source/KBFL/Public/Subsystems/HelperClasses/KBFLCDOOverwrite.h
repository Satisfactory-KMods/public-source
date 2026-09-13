// Copyright Kyri123 / KMods 2026. All Rights Reserved.



#pragma once

#include "CoreMinimal.h"
#include "KBFLCDOCallRequirement.h"
#include "KBFLCDOOverwriteBase.h"
#include "Runtime/CoreUObject/Public/UObject/Object.h"

#include "KBFLCDOOverwrite.generated.h"

UENUM(BlueprintType)
enum class EKBFLBehaivor : uint8
{

	Replace,

	Merge,

	MergeUnique
};

UENUM(BlueprintType)
enum class EKBFLPropertyType : uint8
{
	Unknown,
	Array,
	Set,
	Map,
	Int32,
	Int64,
	UInt32,
	UInt64,
	Float,
	Double,
	Bool,
	String,
	Name,
	Text,
	Object,
	Class,
	Struct,
	Enum,
	Component,
	Other
};

UENUM(BlueprintType)
enum class EKBFLNumericBehavior : uint8
{
	Replace,
	Add,
	Subtract,
	Multiply,
	Divide,
	Min,
	Max,
	Clamp
};

UENUM(BlueprintType)
enum class EKBFLBoolBehavior : uint8
{
	Replace,
	Invert,
	And,
	Or,
	Xor
};

UENUM(BlueprintType)
enum class EKBFLStringBehavior : uint8
{
	Replace,
	Append,
	Prepend,
	Clear
};

USTRUCT(BlueprintType)
struct FKBFLCDOOverwriteProperty
{
	GENERATED_BODY()

	FKBFLCDOOverwriteProperty() = default;

	explicit FKBFLCDOOverwriteProperty(FName InPropertyName, EKBFLPropertyType InType = EKBFLPropertyType::Unknown) :
		mPropertyName(InPropertyName), mPropertyType(InType)
	{
	}

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Property")
	FName mPropertyName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Property")
	EKBFLPropertyType mPropertyType = EKBFLPropertyType::Unknown;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Property")
	TSoftClassPtr<UObject> mObjectPropertyClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Property")
	bool bSkipThisField = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior|Collection", meta = (EditCondition = "mPropertyType == EKBFLPropertyType::Array || mPropertyType == EKBFLPropertyType::Set || mPropertyType == EKBFLPropertyType::Map", EditConditionHides))

	EKBFLBehaivor mCollectionBehavior = EKBFLBehaivor::Replace;

  	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior|Numeric", meta = (EditCondition = "mPropertyType == EKBFLPropertyType::Int32 || mPropertyType == EKBFLPropertyType::Int64 || mPropertyType == EKBFLPropertyType::UInt32 || mPropertyType == EKBFLPropertyType::UInt64 || mPropertyType == EKBFLPropertyType::Float || mPropertyType == EKBFLPropertyType::Double", EditConditionHides))

	EKBFLNumericBehavior mNumericBehavior = EKBFLNumericBehavior::Replace;

  	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior|Numeric", meta = (EditCondition = "mNumericBehavior == EKBFLNumericBehavior::Clamp && (mPropertyType == EKBFLPropertyType::Int32 || mPropertyType == EKBFLPropertyType::Int64 || mPropertyType == EKBFLPropertyType::UInt32 || mPropertyType == EKBFLPropertyType::UInt64 || mPropertyType == EKBFLPropertyType::Float || mPropertyType == EKBFLPropertyType::Double)", EditConditionHides))

	double mMinValue = 0.0;

  	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior|Numeric", meta = (EditCondition = "mNumericBehavior == EKBFLNumericBehavior::Clamp && (mPropertyType == EKBFLPropertyType::Int32 || mPropertyType == EKBFLPropertyType::Int64 || mPropertyType == EKBFLPropertyType::UInt32 || mPropertyType == EKBFLPropertyType::UInt64 || mPropertyType == EKBFLPropertyType::Float || mPropertyType == EKBFLPropertyType::Double)", EditConditionHides))

	double mMaxValue = 100.0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior|Bool",
			  meta = (EditCondition = "mPropertyType == EKBFLPropertyType::Bool", EditConditionHides))
	EKBFLBoolBehavior mBoolBehavior = EKBFLBoolBehavior::Replace;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior|String", meta = (EditCondition = "mPropertyType == EKBFLPropertyType::String || mPropertyType == EKBFLPropertyType::Name || mPropertyType == EKBFLPropertyType::Text", EditConditionHides))

	EKBFLStringBehavior mStringBehavior = EKBFLStringBehavior::Replace;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior|String", meta = (EditCondition = "(mStringBehavior == EKBFLStringBehavior::Append || mStringBehavior == EKBFLStringBehavior::Prepend) && (mPropertyType == EKBFLPropertyType::String || mPropertyType == EKBFLPropertyType::Name || mPropertyType == EKBFLPropertyType::Text)", EditConditionHides))

	FString mStringSeparator = TEXT("");

	bool operator==(const FKBFLCDOOverwriteProperty& Other) const { return mPropertyName == Other.mPropertyName; }

	bool operator==(const FName& OtherName) const { return mPropertyName == OtherName; }

	friend uint32 GetTypeHash(const FKBFLCDOOverwriteProperty& Prop) { return GetTypeHash(Prop.mPropertyName); }

	bool ShouldSkipByDefault() const;
};

UCLASS()
class KBFL_API UKBFLCDOOverwrite : public UKBFLCDOOverwriteBase
{
	GENERATED_BODY()

public:

	UKBFLCDOOverwrite();

	virtual void PostLoad() override;

#if WITH_EDITOR

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;

	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;

	void ValidateManualProperties();
#endif

	static EKBFLPropertyType GetPropertyType(const FProperty* Property);

	UClass* GetSuperClass() const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowAbstract = "true"), Category = "Target Settings")
	TSubclassOf<UObject> mTargetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Target Settings")
	bool bTargetOnlyAsContainer = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Target Settings",
			  meta = (AllowAbstract = "true", EditCondition = "bTargetOnlyAsContainer", EditConditionHides))
	TSubclassOf<UObject> mRealTargetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Target Settings")
	TArray<TSoftClassPtr<UObject>> mOtherTargetClasses;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Target Settings")
	TArray<FString> mFindAssetsInPaths;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Subclass Handling")
	bool bApplyOnSubclasses = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Subclass Handling",
			  meta = (EditCondition = "bApplyOnSubclasses", EditConditionHides))
	bool bOnlyApplyOnSubclasses = false;

	bool OnlyApplyOnSubclasses() const { return bApplyOnSubclasses && bOnlyApplyOnSubclasses; }

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Subclass Handling",
			  meta = (EditCondition = "bApplyOnSubclasses", EditConditionHides))
	bool bUseNativeForSubclasses = false;

	bool UseNativeForSubclasses() const { return bApplyOnSubclasses && bUseNativeForSubclasses; }

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Subclass Handling",
			  meta = (EditCondition = "bApplyOnSubclasses", EditConditionHides))
	bool bUseTargetAsSubclassFilter = false;

	bool UseTargetAsSubclassFilter() const { return bApplyOnSubclasses && bUseTargetAsSubclassFilter; }

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Subclass Handling",
			  meta = (AllowAbstract = "true", EditCondition = "bApplyOnSubclasses"))
	TArray<TSoftClassPtr<UObject>> mAddionalSubClasses;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Subclass Handling",
			  meta = (EditCondition = "bApplyOnSubclasses", EditConditionHides))
	bool bOnlyApplyOnBlueprints = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Include/Exclude",
			  meta = (AllowAbstract = "false", BlueprintBaseOnly = "false"))
	TArray<TSoftClassPtr<UObject>> mAlsoApplyOn;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Include/Exclude",
			  meta = (AllowAbstract = "false", BlueprintBaseOnly = "false"))
	TArray<TSoftClassPtr<UObject>> mIgnoreClasses;

	UPROPERTY(EditAnywhere, Instanced, Category = "Property Container")
	TObjectPtr<UObject> mPropertyContainer = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Property Container")
	TObjectPtr<UClass> mNativeClass = nullptr;

	UPROPERTY(EditAnywhere, Category = "Property Overrides", meta = (TitleProperty = "mPropertyName"))
	TSet<FKBFLCDOOverwriteProperty> mModifiedProperties;

	UPROPERTY(EditAnywhere, Category = "Property Overrides", meta = (TitleProperty = "mPropertyName"))
	TSet<FKBFLCDOOverwriteProperty> mManuelPropertiesOverwrite;

	virtual bool ShouldCallForInstance(UClass* NewClass) override;

protected:

	virtual void ApplyToInstance(UObject* TargetInstance) override;

private:

	bool CollectClassesToProcess(TSet<TSubclassOf<UObject>>& OutClasses);

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
