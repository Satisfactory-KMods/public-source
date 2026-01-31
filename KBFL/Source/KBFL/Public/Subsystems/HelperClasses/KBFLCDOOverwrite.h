//

#pragma once

#include "CoreMinimal.h"
#include "KBFLCDOCallRequirement.h"
#include "KBFLCDOOverwriteBase.h"
#include "Runtime/CoreUObject/Public/UObject/Object.h"

#include "KBFLCDOOverwrite.generated.h"

UENUM(BlueprintType)
enum class EKBFLBehaivor : uint8
{
	/**
	 * Fully replace existing collection
	 */
	Replace,
	/**
	 * Add entries to existing collection (for sets and arrays)
	 */
	Merge,
	/**
	 * Merge but only add unique entries (for sets and arrays)
	 */
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
	Component, // Skip by default
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

	/** Name of the property to override */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CDO")
	FName mPropertyName;

	/** Skip applying this property override */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CDO")
	bool bSkipThisField = false;

	/** Automatically detected property type (set by validation) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CDO")
	EKBFLPropertyType mPropertyType = EKBFLPropertyType::Unknown;

	/** For Object properties: the class of the object (used to determine if it should be skipped) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CDO")
	TSoftClassPtr<UObject> mObjectPropertyClass;

	// ===== Array/Set/Map Behavior =====
	/** Behavior for array/set/map properties (Replace is default) */
	// clang-format off
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CDO|Collection", meta = (EditCondition = "mPropertyType == EKBFLPropertyType::Array || mPropertyType == EKBFLPropertyType::Set || mPropertyType == EKBFLPropertyType::Map", EditConditionHides))
	// clang-format on
	EKBFLBehaivor mCollectionBehavior = EKBFLBehaivor::Replace;

	// ===== Numeric Behavior =====
	/** Behavior for numeric properties (int, float, double) */
	// clang-format off
  	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CDO|Numeric", meta = (EditCondition = "mPropertyType == EKBFLPropertyType::Int32 || mPropertyType == EKBFLPropertyType::Int64 || mPropertyType == EKBFLPropertyType::UInt32 || mPropertyType == EKBFLPropertyType::UInt64 || mPropertyType == EKBFLPropertyType::Float || mPropertyType == EKBFLPropertyType::Double", EditConditionHides))
	// clang-format on
	EKBFLNumericBehavior mNumericBehavior = EKBFLNumericBehavior::Replace;

	/** Minimum value for Clamp behavior */
	// clang-format off
  	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CDO|Numeric", meta = (EditCondition = "mNumericBehavior == EKBFLNumericBehavior::Clamp && (mPropertyType == EKBFLPropertyType::Int32 || mPropertyType == EKBFLPropertyType::Int64 || mPropertyType == EKBFLPropertyType::UInt32 || mPropertyType == EKBFLPropertyType::UInt64 || mPropertyType == EKBFLPropertyType::Float || mPropertyType == EKBFLPropertyType::Double)", EditConditionHides))
	// clang-format on
	double mMinValue = 0.0;

	/** Maximum value for Clamp behavior */
	// clang-format off
  	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CDO|Numeric", meta = (EditCondition = "mNumericBehavior == EKBFLNumericBehavior::Clamp && (mPropertyType == EKBFLPropertyType::Int32 || mPropertyType == EKBFLPropertyType::Int64 || mPropertyType == EKBFLPropertyType::UInt32 || mPropertyType == EKBFLPropertyType::UInt64 || mPropertyType == EKBFLPropertyType::Float || mPropertyType == EKBFLPropertyType::Double)", EditConditionHides))
	// clang-format on
	double mMaxValue = 100.0;

	// ===== Bool Behavior =====
	/** Behavior for bool properties */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CDO|Bool",
			  meta = (EditCondition = "mPropertyType == EKBFLPropertyType::Bool", EditConditionHides))
	EKBFLBoolBehavior mBoolBehavior = EKBFLBoolBehavior::Replace;

	// ===== String/Text/Name Behavior =====
	/** Behavior for string/text/name properties */
	// clang-format off
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CDO|String", meta = (EditCondition = "mPropertyType == EKBFLPropertyType::String || mPropertyType == EKBFLPropertyType::Name || mPropertyType == EKBFLPropertyType::Text", EditConditionHides))
	// clang-format on
	EKBFLStringBehavior mStringBehavior = EKBFLStringBehavior::Replace;

	/** Separator for Append/Prepend string operations */
	// clang-format off
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CDO|String", meta = (EditCondition = "(mStringBehavior == EKBFLStringBehavior::Append || mStringBehavior == EKBFLStringBehavior::Prepend) && (mPropertyType == EKBFLPropertyType::String || mPropertyType == EKBFLPropertyType::Name || mPropertyType == EKBFLPropertyType::Text)", EditConditionHides))
	// clang-format on
	FString mStringSeparator = TEXT("");

	// Operators for TSet support - compare by mPropertyName only
	bool operator==(const FKBFLCDOOverwriteProperty& Other) const { return mPropertyName == Other.mPropertyName; }

	bool operator==(const FName& OtherName) const { return mPropertyName == OtherName; }

	friend uint32 GetTypeHash(const FKBFLCDOOverwriteProperty& Prop) { return GetTypeHash(Prop.mPropertyName); }

	/** Check if this property type should be skipped by default (Components,
	 * Objects, etc.) 
	 * Asset types like UTexture, UStaticMesh, UMaterial, etc. are NOT skipped. */
	bool ShouldSkipByDefault() const;
};

/**
 *
 */
UCLASS()
class KBFL_API UKBFLCDOOverwrite : public UKBFLCDOOverwriteBase
{
	GENERATED_BODY()

public:
	UKBFLCDOOverwrite();

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

	/** Get the property type from an FProperty */
	static EKBFLPropertyType GetPropertyType(const FProperty* Property);

	UClass* GetSuperClass() const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CDO")
	bool bApplyOnSubclasses = false;

	/** If true, the overrides will ONLY be applied on subclasses, not on the
	 * target class itself */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CDO",
			  meta = (EditCondition = "bApplyOnSubclasses", EditConditionHides))
	bool bOnlyApplyOnSubclasses = false;

	/** If true, the overrides will ONLY be applied on subclasses, not on the
	 * target class itself */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CDO",
			  meta = (EditCondition = "bApplyOnSubclasses", EditConditionHides))
	bool bUseNativeForSubclasses = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CDO",
			  meta = (EditCondition = "bApplyOnSubclasses", EditConditionHides))
	bool bUseTargetAsSubclassFilter = false;

	/** If true, the overrides will ONLY be applied on Blueprint subclasses (not
	 * native C++ classes) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CDO")
	bool bOnlyApplyOnBlueprints = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CDO",
			  meta = (AllowAbstract = "true", EditCondition = "bApplyOnSubclasses"))
	TArray<TSoftClassPtr<UObject>> mAddionalSubClasses;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowAbstract = "true"), Category = "CDO")
	TSubclassOf<UObject> mTargetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CDO")
	TArray<TSoftClassPtr<UObject>> mOtherTargetClasses;

	/** If true, mTargetClass is only used as a container for properties, not for
	 * subclass handling. Use mRealTargetClass for actual class targeting. This is
	 * a workaround for abstract classes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CDO")
	bool bTargetOnlyAsContainer = false;

	/** The real target class to apply overrides to when bTargetOnlyAsContainer is
	 * true. This allows you to use an abstract class as property container while
	 * targeting concrete classes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CDO",
			  meta = (AllowAbstract = "true", EditCondition = "bTargetOnlyAsContainer", EditConditionHides))
	TSubclassOf<UObject> mRealTargetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CDO")
	TArray<FString> mFindAssetsInPaths;

	/** Also apply these overrides to these specific subclasses of TargetClass
	 * (native classes only) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CDO",
			  meta = (AllowAbstract = "false", BlueprintBaseOnly = "false"))
	TArray<TSoftClassPtr<UObject>> mAlsoApplyOn;

	/** Classes to ignore when applying overrides (excludes specific subclasses)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CDO",
			  meta = (AllowAbstract = "false", BlueprintBaseOnly = "false"))
	TArray<TSoftClassPtr<UObject>> mIgnoreClasses;

	UPROPERTY(VisibleAnywhere, Category = "CDO")
	UClass* mNativeClass = nullptr;

	/** Instanced object storing actual overridden property values */
	UPROPERTY(EditAnywhere, Instanced, Category = "CDO")
	UObject* mPropertyContainer = nullptr;

	/** Properties that have been explicitly modified by the user */
	UPROPERTY(EditAnywhere, Category = "CDO", meta = (TitleProperty = "mPropertyName"))
	TSet<FKBFLCDOOverwriteProperty> mModifiedProperties;

	/** Manual property overrides - allows you to manually add/edit properties to
	 * override */
	UPROPERTY(EditAnywhere, Category = "CDO", meta = (TitleProperty = "mPropertyName"))
	TSet<FKBFLCDOOverwriteProperty> mManuelPropertiesOverwrite;

	/** Apply changed properties (different from original CDO values) to a live
	 * instance */
private:
	void ApplyToInstance(UObject* TargetInstance);

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
	UFUNCTION(CallInEditor, Category = "CDO")
	void RefreshPropertyContainer();

	/** Validate and auto-detect types for manual property overrides */
	UFUNCTION(CallInEditor, Category = "CDO")
	void ValidateAndDetectPropertyTypes();

#if WITH_EDITOR
	/** Rebuild mModifiedProperties by comparing mPropertyContainer with CDO */
	void RebuildModifiedProperties();
#endif
};
