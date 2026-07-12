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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Property")
	FName mPropertyName;

	/** Automatically detected property type (set by validation) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Property")
	EKBFLPropertyType mPropertyType = EKBFLPropertyType::Unknown;

	/** For Object properties: the class of the object (used to determine if it should be skipped) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Property")
	TSoftClassPtr<UObject> mObjectPropertyClass;

	/** Skip applying this property override */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Property")
	bool bSkipThisField = false;

	// ===== Collection Behavior (Array/Set/Map) =====
	/** Behavior for array/set/map properties (Replace is default) */
	// clang-format off
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior|Collection", meta = (EditCondition = "mPropertyType == EKBFLPropertyType::Array || mPropertyType == EKBFLPropertyType::Set || mPropertyType == EKBFLPropertyType::Map", EditConditionHides))
	// clang-format on
	EKBFLBehaivor mCollectionBehavior = EKBFLBehaivor::Replace;

	// ===== Numeric Behavior =====
	/** Behavior for numeric properties (int, float, double) */
	// clang-format off
  	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior|Numeric", meta = (EditCondition = "mPropertyType == EKBFLPropertyType::Int32 || mPropertyType == EKBFLPropertyType::Int64 || mPropertyType == EKBFLPropertyType::UInt32 || mPropertyType == EKBFLPropertyType::UInt64 || mPropertyType == EKBFLPropertyType::Float || mPropertyType == EKBFLPropertyType::Double", EditConditionHides))
	// clang-format on
	EKBFLNumericBehavior mNumericBehavior = EKBFLNumericBehavior::Replace;

	/** Minimum value for Clamp behavior */
	// clang-format off
  	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior|Numeric", meta = (EditCondition = "mNumericBehavior == EKBFLNumericBehavior::Clamp && (mPropertyType == EKBFLPropertyType::Int32 || mPropertyType == EKBFLPropertyType::Int64 || mPropertyType == EKBFLPropertyType::UInt32 || mPropertyType == EKBFLPropertyType::UInt64 || mPropertyType == EKBFLPropertyType::Float || mPropertyType == EKBFLPropertyType::Double)", EditConditionHides))
	// clang-format on
	double mMinValue = 0.0;

	/** Maximum value for Clamp behavior */
	// clang-format off
  	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior|Numeric", meta = (EditCondition = "mNumericBehavior == EKBFLNumericBehavior::Clamp && (mPropertyType == EKBFLPropertyType::Int32 || mPropertyType == EKBFLPropertyType::Int64 || mPropertyType == EKBFLPropertyType::UInt32 || mPropertyType == EKBFLPropertyType::UInt64 || mPropertyType == EKBFLPropertyType::Float || mPropertyType == EKBFLPropertyType::Double)", EditConditionHides))
	// clang-format on
	double mMaxValue = 100.0;

	// ===== Bool Behavior =====
	/** Behavior for bool properties */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior|Bool",
			  meta = (EditCondition = "mPropertyType == EKBFLPropertyType::Bool", EditConditionHides))
	EKBFLBoolBehavior mBoolBehavior = EKBFLBoolBehavior::Replace;

	// ===== String/Text/Name Behavior =====
	/** Behavior for string/text/name properties */
	// clang-format off
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior|String", meta = (EditCondition = "mPropertyType == EKBFLPropertyType::String || mPropertyType == EKBFLPropertyType::Name || mPropertyType == EKBFLPropertyType::Text", EditConditionHides))
	// clang-format on
	EKBFLStringBehavior mStringBehavior = EKBFLStringBehavior::Replace;

	/** Separator for Append/Prepend string operations */
	// clang-format off
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior|String", meta = (EditCondition = "(mStringBehavior == EKBFLStringBehavior::Append || mStringBehavior == EKBFLStringBehavior::Prepend) && (mPropertyType == EKBFLPropertyType::String || mPropertyType == EKBFLPropertyType::Name || mPropertyType == EKBFLPropertyType::Text)", EditConditionHides))
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
	/** Default constructor. */
	UKBFLCDOOverwrite();

	// UObject interface
	/** Ensures mNativeClass is resolved and recreates the property container if it is missing or class-mismatched. */
	virtual void PostLoad() override;
	// End UObject interface

#if WITH_EDITOR
	/** Editor: clears overrides and refreshes the container when the target class changes; revalidates manual props. */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	/** Editor: rebuilds mModifiedProperties when a value inside mPropertyContainer is edited. */
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
	/** Editor: validates the target class, mAlsoApplyOn subclassing, and each manual property override. */
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;

	/** Validate and auto-detect property types for manual property overrides */
	void ValidateManualProperties();
#endif

	/** Get the property type from an FProperty */
	static EKBFLPropertyType GetPropertyType(const FProperty* Property);

	/** Walks up from mTargetClass to the first non-Blueprint (native) super class. */
	UClass* GetSuperClass() const;

	// ===== Target Settings =====
	/** The target class whose properties we want to override */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowAbstract = "true"), Category = "Target Settings")
	TSubclassOf<UObject> mTargetClass;

	/** If true, mTargetClass is only used as a container for properties, not for
	 * subclass handling. Use mRealTargetClass for actual class targeting. This is
	 * a workaround for abstract classes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Target Settings")
	bool bTargetOnlyAsContainer = false;

	/** The real target class to apply overrides to when bTargetOnlyAsContainer is
	 * true. This allows you to use an abstract class as property container while
	 * targeting concrete classes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Target Settings",
			  meta = (AllowAbstract = "true", EditCondition = "bTargetOnlyAsContainer", EditConditionHides))
	TSubclassOf<UObject> mRealTargetClass;

	/** Additional target classes to apply overrides to */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Target Settings")
	TArray<TSoftClassPtr<UObject>> mOtherTargetClasses;

	/** Paths to search for assets (e.g., "/Game/MyMod/") */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Target Settings")
	TArray<FString> mFindAssetsInPaths;

	// ===== Subclass Handling =====
	/** If true, apply overrides to all subclasses of the target class */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Subclass Handling")
	bool bApplyOnSubclasses = false;

	/** If true, the overrides will ONLY be applied on subclasses, not on the
	 * target class itself */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Subclass Handling",
			  meta = (EditCondition = "bApplyOnSubclasses", EditConditionHides))
	bool bOnlyApplyOnSubclasses = false;
	/** True when subclass application is on and the target class itself should be skipped. */
	bool OnlyApplyOnSubclasses() const { return bApplyOnSubclasses && bOnlyApplyOnSubclasses; }

	/** If true, use native class for subclass checks instead of generated class */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Subclass Handling",
			  meta = (EditCondition = "bApplyOnSubclasses", EditConditionHides))
	bool bUseNativeForSubclasses = false;
	/** True when subclass application is on and the native (non-Blueprint) class should be used for subclass checks. */
	bool UseNativeForSubclasses() const { return bApplyOnSubclasses && bUseNativeForSubclasses; }

	/** If true, use mTargetClass as subclass filter instead of all subclasses */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Subclass Handling",
			  meta = (EditCondition = "bApplyOnSubclasses", EditConditionHides))
	bool bUseTargetAsSubclassFilter = false;
	/** True when subclass application is on and mTargetClass should be used as the subclass filter. */
	bool UseTargetAsSubclassFilter() const { return bApplyOnSubclasses && bUseTargetAsSubclassFilter; }

	/** Additional subclasses to include in the override process */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Subclass Handling",
			  meta = (AllowAbstract = "true", EditCondition = "bApplyOnSubclasses"))
	TArray<TSoftClassPtr<UObject>> mAddionalSubClasses;


	/** If true, the overrides will ONLY be applied on Blueprint subclasses (not
	 * native C++ classes) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Subclass Handling",
			  meta = (EditCondition = "bApplyOnSubclasses", EditConditionHides))
	bool bOnlyApplyOnBlueprints = true;

	// ===== Include/Exclude =====
	/** Also apply these overrides to these specific classes (native classes only) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Include/Exclude",
			  meta = (AllowAbstract = "false", BlueprintBaseOnly = "false"))
	TArray<TSoftClassPtr<UObject>> mAlsoApplyOn;

	/** Classes to ignore when applying overrides (excludes specific subclasses) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Include/Exclude",
			  meta = (AllowAbstract = "false", BlueprintBaseOnly = "false"))
	TArray<TSoftClassPtr<UObject>> mIgnoreClasses;

	// ===== Property Container =====
	/** Instanced object storing actual overridden property values */
	UPROPERTY(EditAnywhere, Instanced, Category = "Property Container")
	TObjectPtr<UObject> mPropertyContainer = nullptr;

	/** Native class cached from mTargetClass */
	UPROPERTY(VisibleAnywhere, Category = "Property Container")
	TObjectPtr<UClass> mNativeClass = nullptr;

	// ===== Property Overrides =====
	/** Properties that have been explicitly modified by the user */
	UPROPERTY(EditAnywhere, Category = "Property Overrides", meta = (TitleProperty = "mPropertyName"))
	TSet<FKBFLCDOOverwriteProperty> mModifiedProperties;

	/** Manual property overrides - allows you to manually add/edit properties to
	 * override */
	UPROPERTY(EditAnywhere, Category = "Property Overrides", meta = (TitleProperty = "mPropertyName"))
	TSet<FKBFLCDOOverwriteProperty> mManuelPropertiesOverwrite;

	/** Cheap O(1) check (lazy-load path) for whether a single newly-loaded class matches this overwrite's targeting. */
	virtual bool ShouldCallForInstance(UClass* NewClass) override;

protected:
	/** Apply changed properties (different from original CDO values) to a live
	 * instance */
	virtual void ApplyToInstance(UObject* TargetInstance) override;

private:
	/** Build the full set of classes this overwrite targets (direct + subclasses + paths).
	 * Returns false if the target configuration is invalid. Shared by ApplyToInstances and ShouldCallForInstance. */
	bool CollectClassesToProcess(TSet<TSubclassOf<UObject>>& OutClasses);

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
	/** Collects every targeted class CDO (target + subclasses + paths) and applies the configured overrides to each. */
	virtual void ApplyToInstances() override;

	/** Refresh the property container when TargetClass changes */
	UFUNCTION(CallInEditor, Category = "Actions")
	void RefreshPropertyContainer();

	/** Validate and auto-detect types for manual property overrides */
	UFUNCTION(CallInEditor, Category = "Actions")
	void ValidateAndDetectPropertyTypes();

#if WITH_EDITOR
	/** Rebuild mModifiedProperties by comparing mPropertyContainer with CDO */
	void RebuildModifiedProperties();
#endif
};
