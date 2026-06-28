//

#include "Subsystems/HelperClasses/KBFLPrimaryDataAssetOverwrite.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/AssetManager.h"
#include "Logging/StructuredLog.h"
#include "Misc/DataValidation.h"
#include "Subsystems/KBFLAssetDataSubsystem.h"
#include "Subsystems/KBFLContentCDOHelperSubsystem.h"

UKBFLPrimaryDataAssetOverwrite::UKBFLPrimaryDataAssetOverwrite() {}

void UKBFLPrimaryDataAssetOverwrite::PostLoad()
{
	Super::PostLoad();

	// Validate property container and recreate only if necessary
	if (mTargetDataAssetClass != nullptr || IsValid(mTargetDataAssetInstance))
	{
		if (!mPropertyContainer)
		{
			// Container is missing, recreate it
			UE_LOG(LogKBFLCDOOverwrite, Warning, TEXT("PostLoad: Property container is null for '%s', recreating..."),
				   *GetPathName());
			RefreshPropertyContainer();
		}
		else
		{
			// Validate that container class matches target
			bool bNeedsRecreate = false;

			if (IsValid(mTargetDataAssetInstance))
			{
				if (mTargetDataAssetInstance)
				{
					if (!mPropertyContainer->IsA(mTargetDataAssetInstance->GetClass()))
					{
						UE_LOG(
							LogKBFLCDOOverwrite, Warning,
							TEXT("PostLoad: Property container class mismatch for instance target '%s'. Recreating..."),
							*GetPathName());
						bNeedsRecreate = true;
					}
				}
			}
			else if (mTargetDataAssetClass != nullptr)
			{
				TSubclassOf<UDataAsset> LoadedClass = mTargetDataAssetClass;

				// For abstract target classes the container is created from mAbstractContainerClass,
				// so check against that instead.
				TSubclassOf<UDataAsset> ContainerCheckClass =
					(LoadedClass && LoadedClass->HasAnyClassFlags(CLASS_Abstract) && mAbstractContainerClass)
					? mAbstractContainerClass
					: LoadedClass;

				if (ContainerCheckClass && !mPropertyContainer->IsA(ContainerCheckClass))
				{
					UE_LOG(LogKBFLCDOOverwrite, Warning,
						   TEXT("PostLoad: Property container class mismatch for class target '%s'. Recreating..."),
						   *GetPathName());
					bNeedsRecreate = true;
				}
			}

			if (bNeedsRecreate)
			{
				mPropertyContainer->MarkAsGarbage();
				mPropertyContainer = nullptr;
				RefreshPropertyContainer();
			}
		}
	}
}

#if WITH_EDITOR
void UKBFLPrimaryDataAssetOverwrite::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	FName ChangedPropertyName = PropertyChangedEvent.GetPropertyName();

	if (ChangedPropertyName == GET_MEMBER_NAME_CHECKED(UKBFLPrimaryDataAssetOverwrite, mTargetDataAssetClass))
	{
		// Clear modified properties when target class changes
		mModifiedProperties.Empty();
		mManuelPropertiesOverwrite.Empty();
		mAbstractContainerClass = nullptr; // reset container class when target changes
		RefreshPropertyContainer();
	}
	else if (ChangedPropertyName == GET_MEMBER_NAME_CHECKED(UKBFLPrimaryDataAssetOverwrite, mAbstractContainerClass))
	{
		// Recreate container when the concrete container class for an abstract target is changed
		mModifiedProperties.Empty();
		RefreshPropertyContainer();
	}
	else if (ChangedPropertyName == GET_MEMBER_NAME_CHECKED(UKBFLPrimaryDataAssetOverwrite, mTargetDataAssetInstance))
	{
		// When instance is set, use its class as the property container class
		if (mTargetDataAssetInstance)
		{
			mTargetDataAssetClass = mTargetDataAssetInstance->GetClass();
			mModifiedProperties.Empty();
			mManuelPropertiesOverwrite.Empty();
			RefreshPropertyContainer();
		}
	}
	else if (ChangedPropertyName == GET_MEMBER_NAME_CHECKED(UKBFLPrimaryDataAssetOverwrite, mManuelPropertiesOverwrite))
	{
		// Validate and auto-detect property types when manual properties are edited
		ValidateManualProperties();
	}
}

void UKBFLPrimaryDataAssetOverwrite::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	// Check if the change was made to a property inside mPropertyContainer
	if (mPropertyContainer && mTargetDataAssetClass != nullptr)
	{
		FEditPropertyChain::TDoubleLinkedListNode* PropertyNode = PropertyChangedEvent.PropertyChain.GetHead();
		if (PropertyNode)
		{
			FProperty* HeadProperty = PropertyNode->GetValue();
			if (HeadProperty &&
				HeadProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UKBFLPrimaryDataAssetOverwrite, mPropertyContainer))
			{
				// Rebuild mModifiedProperties by comparing all properties with default values
				RebuildModifiedProperties();
			}
		}
	}
}

EDataValidationResult UKBFLPrimaryDataAssetOverwrite::IsDataValid(FDataValidationContext& Context) const
{
	// Determine if the target is an abstract class early so we can adjust validation.
	const bool bTargetIsAbstract = mTargetDataAssetClass && mTargetDataAssetClass->HasAnyClassFlags(CLASS_Abstract) &&
		!IsValid(mTargetDataAssetInstance);

	// When the target is abstract and mAbstractContainerClass is set (concrete subclass),
	// the mPropertyContainer will be of that concrete type — Super validation is safe.
	// When abstract but NO container class is set yet, mPropertyContainer is null and Super
	// would fail on the missing Instanced sub-object, so skip Super in that case only.
	const bool bSkipSuper = bTargetIsAbstract && !mAbstractContainerClass;

	EDataValidationResult Result = bSkipSuper ? EDataValidationResult::Valid : Super::IsDataValid(Context);

	if (!mTargetDataAssetClass)
	{
		Context.AddError(FText::FromString(TEXT("Target DataAsset Class must be set.")));
		Result = EDataValidationResult::Invalid;
	}

	if (TSubclassOf<UDataAsset> LoadedTargetClass = mTargetDataAssetClass)
	{
		if (bTargetIsAbstract)
		{
			// Must provide a concrete container class when target is abstract
			if (!mAbstractContainerClass)
			{
				Context.AddError(FText::Format(NSLOCTEXT("KBFLPrimaryDataAssetOverwrite", "AbstractNoContainer",
														 "Target DataAsset Class '{0}' is abstract. "
														 "Please set 'Abstract Container Class' to a concrete subclass "
														 "so the property container can be created."),
											   FText::FromString(LoadedTargetClass->GetName())));
				Result = EDataValidationResult::Invalid;
			}
			else if (!mAbstractContainerClass->IsChildOf(LoadedTargetClass))
			{
				Context.AddError(
					FText::Format(NSLOCTEXT("KBFLPrimaryDataAssetOverwrite", "AbstractContainerWrongBase",
											"Abstract Container Class '{0}' is not a subclass of target class '{1}'."),
								  FText::FromString(mAbstractContainerClass->GetName()),
								  FText::FromString(LoadedTargetClass->GetName())));
				Result = EDataValidationResult::Invalid;
			}
		}

		for (const FKBFLCDOOverwriteProperty& ManualProp : mManuelPropertiesOverwrite)
		{
			FProperty* Property = FindFProperty<FProperty>(LoadedTargetClass, ManualProp.mPropertyName);
			if (!Property)
			{
				Context.AddError(FText::Format(
					NSLOCTEXT("KBFLPrimaryDataAssetOverwrite", "InvalidManualProperty",
							  "Manual property '{0}' does not exist on target DataAsset class '{1}'."),
					FText::FromName(ManualProp.mPropertyName), FText::FromString(LoadedTargetClass->GetName())));
				Result = EDataValidationResult::Invalid;
			}
		}
	}

	return Result;
}

void UKBFLPrimaryDataAssetOverwrite::ValidateManualProperties()
{
	if (!mTargetDataAssetClass)
	{
		return;
	}

	TSubclassOf<UDataAsset> LoadedTargetClass = mTargetDataAssetClass;
	if (!LoadedTargetClass)
	{
		return;
	}

	// For each manual property, try to auto-detect its type if not already set
	for (FKBFLCDOOverwriteProperty& ManualProp : mManuelPropertiesOverwrite)
	{
		FProperty* Property = FindFProperty<FProperty>(LoadedTargetClass, ManualProp.mPropertyName);

		if (Property)
		{
			// Auto-detect property type using the static method from UKBFLCDOOverwrite
			ManualProp.mPropertyType = UKBFLCDOOverwrite::GetPropertyType(Property);

			// Auto-skip Component and Object types by default
			if (ManualProp.ShouldSkipByDefault() && !ManualProp.bSkipThisField)
			{
				ManualProp.bSkipThisField = true;
			}
		}
	}
}

void UKBFLPrimaryDataAssetOverwrite::RebuildModifiedProperties()
{
	if (!mPropertyContainer)
	{
		mModifiedProperties.Empty();
		return;
	}

	// Priority: Compare with instance if set, otherwise with CDO
	UObject* ComparisonObject = nullptr;

	if (mTargetDataAssetInstance)
	{
		// Compare with the specific instance
		ComparisonObject = mTargetDataAssetInstance;
	}
	else if (TSubclassOf<UDataAsset> LoadedClass = mTargetDataAssetClass)
	{
		// Compare with the CDO
		ComparisonObject = LoadedClass->GetDefaultObject();
	}

	if (!ComparisonObject)
	{
		mModifiedProperties.Empty();
		return;
	}

	// Keep track of which properties we find that differ from comparison object
	TSet<FName> CurrentlyModifiedNames;

	// Compare all editable properties between mPropertyContainer and comparison object
	for (TFieldIterator<FProperty> PropIt(ComparisonObject->GetClass(), EFieldIteratorFlags::IncludeSuper); PropIt;
		 ++PropIt)
	{
		FProperty* Property = *PropIt;

		// Skip properties that aren't editable
		if (!Property->HasAnyPropertyFlags(CPF_Edit))
		{
			continue;
		}

		void* ContainerValuePtr = Property->ContainerPtrToValuePtr<void>(mPropertyContainer);
		void* ComparisonValuePtr = Property->ContainerPtrToValuePtr<void>(ComparisonObject);

		// If property differs from comparison object, add it to modified set
		if (!Property->Identical(ContainerValuePtr, ComparisonValuePtr))
		{
			CurrentlyModifiedNames.Add(Property->GetFName());

			// Check if this property already exists in our set
			FKBFLCDOOverwriteProperty* Existing =
				mModifiedProperties.Find(FKBFLCDOOverwriteProperty(Property->GetFName()));
			if (!Existing)
			{
				// Add new entry with detected property type
				EKBFLPropertyType PropType = UKBFLCDOOverwrite::GetPropertyType(Property);
				FKBFLCDOOverwriteProperty NewProp(Property->GetFName(), PropType);

				// Auto-skip Component and Object types
				if (NewProp.ShouldSkipByDefault())
				{
					NewProp.bSkipThisField = true;
				}

				mModifiedProperties.Add(NewProp);
			}
		}
	}

	// Remove entries that are no longer modified
	TArray<FKBFLCDOOverwriteProperty> ToRemove;
	for (const FKBFLCDOOverwriteProperty& Prop : mModifiedProperties)
	{
		if (!CurrentlyModifiedNames.Contains(Prop.mPropertyName))
		{
			ToRemove.Add(Prop);
		}
	}
	for (const FKBFLCDOOverwriteProperty& Prop : ToRemove)
	{
		mModifiedProperties.Remove(Prop);
	}
}
#endif

void UKBFLPrimaryDataAssetOverwrite::RefreshPropertyContainer()
{
	// Clear existing container
	if (mPropertyContainer)
	{
		mPropertyContainer->MarkAsGarbage();
		mPropertyContainer = nullptr;
	}
	mModifiedProperties.Empty();

	// Priority: Use mTargetDataAssetInstance if set, otherwise use mTargetDataAssetClass
	UObject* SourceObject = nullptr;
	FString SourceDescription;

	if (mTargetDataAssetInstance)
	{
		// Use the specific instance as source
		SourceObject = mTargetDataAssetInstance;
		SourceDescription = FString::Printf(TEXT("instance '%s'"), *mTargetDataAssetInstance->GetPathName());

		// Also update mTargetDataAssetClass to match the instance's class for validation
		if (!mTargetDataAssetClass)
		{
			mTargetDataAssetClass = mTargetDataAssetInstance->GetClass();
		}
	}
	else if (TSubclassOf<UDataAsset> LoadedClass = mTargetDataAssetClass)
	{
		// If the target class is abstract, we cannot duplicate its CDO directly because UE's asset
		// validation rejects Instanced sub-objects of abstract classes. Use mAbstractContainerClass
		// (a concrete subclass) as the container template instead. mTargetDataAssetClass is still
		// used as the targeting identifier when searching/applying to assets.
		if (LoadedClass->HasAnyClassFlags(CLASS_Abstract))
		{
			TSubclassOf<UDataAsset> ContainerClass = mAbstractContainerClass;
			if (!ContainerClass)
			{
				UE_LOG(LogKBFLCDOOverwrite, Warning,
					   TEXT("RefreshPropertyContainer: mTargetDataAssetClass '%s' is abstract but "
							"mAbstractContainerClass is not set. Please set a concrete subclass as container. "
							"| Asset: '%s'"),
					   *LoadedClass->GetName(), *GetPathName());
				return;
			}

			SourceObject = ContainerClass->GetDefaultObject();
			SourceDescription = FString::Printf(TEXT("abstract target '%s' via concrete container class '%s' CDO"),
												*LoadedClass->GetName(), *ContainerClass->GetName());
		}
		else
		{
			// Normal case: use the CDO of the (concrete) target class
			SourceObject = LoadedClass->GetDefaultObject();
			SourceDescription = FString::Printf(TEXT("class '%s' CDO"), *LoadedClass->GetName());
		}
	}

	if (!SourceObject)
	{
		UE_LOG(LogKBFLCDOOverwrite, Error,
			   TEXT("RefreshPropertyContainer: No valid source object - both mTargetDataAssetInstance and "
					"mTargetDataAssetClass are invalid | Asset: '%s'"),
			   *GetPathName());
		return;
	}

	// Use FObjectDuplicationParameters for more control
	FObjectDuplicationParameters DupParams(SourceObject, this);
	DupParams.DestName = MakeUniqueObjectName(this, SourceObject->GetClass(), FName(TEXT("PropertyContainer")));
	DupParams.FlagMask = RF_AllFlags & ~(RF_ClassDefaultObject | RF_ArchetypeObject | RF_DefaultSubObject);
	DupParams.ApplyFlags = RF_NoFlags;
	DupParams.InternalFlagMask = EInternalObjectFlags_AllFlags;

	mPropertyContainer = StaticDuplicateObjectEx(DupParams);

	if (!mPropertyContainer)
	{
		UE_LOG(LogKBFLCDOOverwrite, Error,
			   TEXT("RefreshPropertyContainer: Failed to duplicate source object from %s | Asset: '%s'"),
			   *SourceDescription, *GetPathName());
		return;
	}

	UE_LOG(
		LogKBFLCDOOverwrite, Log,
		TEXT("RefreshPropertyContainer: Successfully created property container of class '%s' from %s | Asset: '%s'"),
		*mPropertyContainer->GetClass()->GetName(), *SourceDescription, *GetPathName());
}

void UKBFLPrimaryDataAssetOverwrite::ValidateAndDetectPropertyTypes()
{
#if WITH_EDITOR
	ValidateManualProperties();
#endif
}

void UKBFLPrimaryDataAssetOverwrite::ApplyCollectionProperty(FProperty* Property, void* ContainerValuePtr,
															 void* DestValuePtr, EKBFLBehaivor Behavior,
															 const FString& AssetPath, UObject* TargetInstance)
{
	// Use the same implementation as UKBFLCDOOverwrite
	// This code is duplicated to maintain independence from UKBFLCDOOverwrite private methods

	if (Behavior == EKBFLBehaivor::Replace)
	{
		Property->CopyCompleteValue(DestValuePtr, ContainerValuePtr);
		UE_LOGFMT(LogKBFLCDOOverwrite, Warning,
				  "ApplyToDataAssetInstance: Replacing collection property '{PropertyName}' on '{TargetName}' | Asset: "
				  "{AssetPath}",
				  Property->GetName(), TargetInstance->GetName(), AssetPath);
		return;
	}

	// Handle Array
	if (FArrayProperty* ArrayProp = CastField<FArrayProperty>(Property))
	{
		FScriptArrayHelper SourceArray(ArrayProp, ContainerValuePtr);
		FScriptArrayHelper DestArray(ArrayProp, DestValuePtr);

		for (int32 i = 0; i < SourceArray.Num(); ++i)
		{
			bool bShouldAdd = true;

			// For MergeUnique, check if element already exists
			if (Behavior == EKBFLBehaivor::MergeUnique)
			{
				for (int32 j = 0; j < DestArray.Num(); ++j)
				{
					if (ArrayProp->Inner->Identical(SourceArray.GetRawPtr(i), DestArray.GetRawPtr(j)))
					{
						bShouldAdd = false;
						break;
					}
				}
			}

			if (bShouldAdd)
			{
				int32 NewIndex = DestArray.AddValue();
				ArrayProp->Inner->CopySingleValue(DestArray.GetRawPtr(NewIndex), SourceArray.GetRawPtr(i));
			}
		}

		UE_LOGFMT(LogKBFLCDOOverwrite, Warning,
				  "ApplyToDataAssetInstance: Merging array property '{PropertyName}' on '{TargetName}' | Asset: "
				  "{AssetPath} | Mode: {Mode}",
				  Property->GetName(), TargetInstance->GetName(), AssetPath,
				  Behavior == EKBFLBehaivor::MergeUnique ? TEXT("MergeUnique") : TEXT("Merge"));
	}
	// Handle Set
	else if (FSetProperty* SetProp = CastField<FSetProperty>(Property))
	{
		FScriptSetHelper SourceSet(SetProp, ContainerValuePtr);
		FScriptSetHelper DestSet(SetProp, DestValuePtr);

		for (int32 i = 0; i < SourceSet.Num(); ++i)
		{
			if (SourceSet.IsValidIndex(i))
			{
				DestSet.AddElement(SourceSet.GetElementPtr(i));
			}
		}

		UE_LOGFMT(
			LogKBFLCDOOverwrite, Warning,
			"ApplyToDataAssetInstance: Merging set property '{PropertyName}' on '{TargetName}' | Asset: {AssetPath}",
			Property->GetName(), TargetInstance->GetName(), AssetPath);
	}
	// Handle Map
	else if (FMapProperty* MapProp = CastField<FMapProperty>(Property))
	{
		FScriptMapHelper SourceMap(MapProp, ContainerValuePtr);
		FScriptMapHelper DestMap(MapProp, DestValuePtr);

		for (int32 i = 0; i < SourceMap.Num(); ++i)
		{
			if (SourceMap.IsValidIndex(i))
			{
				uint8* SourcePairPtr = SourceMap.GetPairPtr(i);
				uint8* SourceKeyPtr = SourcePairPtr;
				uint8* SourceValuePtr = SourcePairPtr + MapProp->MapLayout.ValueOffset;

				if (Behavior == EKBFLBehaivor::MergeUnique)
				{
					// Find if key already exists
					uint8* ExistingValue = DestMap.FindValueFromHash(SourceKeyPtr);
					if (ExistingValue)
					{
						// Skip - key already exists
						continue;
					}
				}

				// Add or replace key-value pair
				DestMap.AddPair(SourceKeyPtr, SourceValuePtr);
			}
		}

		UE_LOGFMT(
			LogKBFLCDOOverwrite, Warning,
			"ApplyToDataAssetInstance: Merging map property '{PropertyName}' on '{TargetName}' | Asset: {AssetPath}",
			Property->GetName(), TargetInstance->GetName(), AssetPath);
	}
}

void UKBFLPrimaryDataAssetOverwrite::ApplyNumericProperty(FProperty* Property, void* ContainerValuePtr,
														  void* DestValuePtr, EKBFLNumericBehavior Behavior,
														  double MinValue, double MaxValue, const FString& AssetPath,
														  UObject* TargetInstance)
{
	auto ApplyNumericOp = [&](auto& CurrentValue, auto ModifierValue)
	{
		using NumType = std::decay_t<decltype(CurrentValue)>;
		switch (Behavior)
		{
		case EKBFLNumericBehavior::Replace:
			CurrentValue = ModifierValue;
			break;
		case EKBFLNumericBehavior::Add:
			CurrentValue += ModifierValue;
			break;
		case EKBFLNumericBehavior::Subtract:
			CurrentValue -= ModifierValue;
			break;
		case EKBFLNumericBehavior::Multiply:
			CurrentValue *= ModifierValue;
			break;
		case EKBFLNumericBehavior::Divide:
			if (ModifierValue != 0)
			{
				CurrentValue /= ModifierValue;
			}
			break;
		case EKBFLNumericBehavior::Min:
			CurrentValue = FMath::Min(CurrentValue, ModifierValue);
			break;
		case EKBFLNumericBehavior::Max:
			CurrentValue = FMath::Max(CurrentValue, ModifierValue);
			break;
		case EKBFLNumericBehavior::Clamp:
			CurrentValue = FMath::Clamp(CurrentValue, static_cast<NumType>(MinValue), static_cast<NumType>(MaxValue));
			break;
		}
	};

	if (FIntProperty* IntProp = CastField<FIntProperty>(Property))
	{
		int32 CurrentValue = IntProp->GetPropertyValue(DestValuePtr);
		int32 ModifierValue = IntProp->GetPropertyValue(ContainerValuePtr);
		int32 OldValue = CurrentValue;
		ApplyNumericOp(CurrentValue, ModifierValue);
		IntProp->SetPropertyValue(DestValuePtr, CurrentValue);
		UE_LOGFMT(LogKBFLCDOOverwrite, Warning,
				  "ApplyToDataAssetInstance: Int32 operation on '{PropertyName}' on '{TargetName}' | Asset: "
				  "{AssetPath} | {OldValue} -> {NewValue}",
				  Property->GetName(), TargetInstance->GetName(), AssetPath, OldValue, CurrentValue);
	}
	else if (FInt64Property* Int64Prop = CastField<FInt64Property>(Property))
	{
		int64 CurrentValue = Int64Prop->GetPropertyValue(DestValuePtr);
		int64 ModifierValue = Int64Prop->GetPropertyValue(ContainerValuePtr);
		int64 OldValue = CurrentValue;
		ApplyNumericOp(CurrentValue, ModifierValue);
		Int64Prop->SetPropertyValue(DestValuePtr, CurrentValue);
		UE_LOGFMT(LogKBFLCDOOverwrite, Warning,
				  "ApplyToDataAssetInstance: Int64 operation on '{PropertyName}' on '{TargetName}' | Asset: "
				  "{AssetPath} | {OldValue} -> {NewValue}",
				  Property->GetName(), TargetInstance->GetName(), AssetPath, OldValue, CurrentValue);
	}
	else if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Property))
	{
		float CurrentValue = FloatProp->GetPropertyValue(DestValuePtr);
		float ModifierValue = FloatProp->GetPropertyValue(ContainerValuePtr);
		float OldValue = CurrentValue;
		ApplyNumericOp(CurrentValue, ModifierValue);
		FloatProp->SetPropertyValue(DestValuePtr, CurrentValue);
		UE_LOGFMT(LogKBFLCDOOverwrite, Warning,
				  "ApplyToDataAssetInstance: Float operation on '{PropertyName}' on '{TargetName}' | Asset: "
				  "{AssetPath} | {OldValue} -> {NewValue}",
				  Property->GetName(), TargetInstance->GetName(), AssetPath, OldValue, CurrentValue);
	}
	else if (FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(Property))
	{
		double CurrentValue = DoubleProp->GetPropertyValue(DestValuePtr);
		double ModifierValue = DoubleProp->GetPropertyValue(ContainerValuePtr);
		double OldValue = CurrentValue;
		ApplyNumericOp(CurrentValue, ModifierValue);
		DoubleProp->SetPropertyValue(DestValuePtr, CurrentValue);
		UE_LOGFMT(LogKBFLCDOOverwrite, Warning,
				  "ApplyToDataAssetInstance: Double operation on '{PropertyName}' on '{TargetName}' | Asset: "
				  "{AssetPath} | {OldValue} -> {NewValue}",
				  Property->GetName(), TargetInstance->GetName(), AssetPath, OldValue, CurrentValue);
	}
	else if (FUInt32Property* UInt32Prop = CastField<FUInt32Property>(Property))
	{
		uint32 CurrentValue = UInt32Prop->GetPropertyValue(DestValuePtr);
		uint32 ModifierValue = UInt32Prop->GetPropertyValue(ContainerValuePtr);
		uint32 OldValue = CurrentValue;
		ApplyNumericOp(CurrentValue, ModifierValue);
		UInt32Prop->SetPropertyValue(DestValuePtr, CurrentValue);
		UE_LOGFMT(LogKBFLCDOOverwrite, Warning,
				  "ApplyToDataAssetInstance: UInt32 operation on '{PropertyName}' on '{TargetName}' | Asset: "
				  "{AssetPath} | {OldValue} -> {NewValue}",
				  Property->GetName(), TargetInstance->GetName(), AssetPath, OldValue, CurrentValue);
	}
	else if (FUInt64Property* UInt64Prop = CastField<FUInt64Property>(Property))
	{
		uint64 CurrentValue = UInt64Prop->GetPropertyValue(DestValuePtr);
		uint64 ModifierValue = UInt64Prop->GetPropertyValue(ContainerValuePtr);
		uint64 OldValue = CurrentValue;
		ApplyNumericOp(CurrentValue, ModifierValue);
		UInt64Prop->SetPropertyValue(DestValuePtr, CurrentValue);
		UE_LOGFMT(LogKBFLCDOOverwrite, Warning,
				  "ApplyToDataAssetInstance: UInt64 operation on '{PropertyName}' on '{TargetName}' | Asset: "
				  "{AssetPath} | {OldValue} -> {NewValue}",
				  Property->GetName(), TargetInstance->GetName(), AssetPath, OldValue, CurrentValue);
	}
}

void UKBFLPrimaryDataAssetOverwrite::ApplyBoolProperty(FProperty* Property, void* ContainerValuePtr, void* DestValuePtr,
													   EKBFLBoolBehavior Behavior, const FString& AssetPath,
													   UObject* TargetInstance)
{
	if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
	{
		bool CurrentValue = BoolProp->GetPropertyValue(DestValuePtr);
		bool ModifierValue = BoolProp->GetPropertyValue(ContainerValuePtr);
		bool ResultValue = CurrentValue;

		switch (Behavior)
		{
		case EKBFLBoolBehavior::Replace:
			ResultValue = ModifierValue;
			break;
		case EKBFLBoolBehavior::Invert:
			ResultValue = !CurrentValue;
			break;
		case EKBFLBoolBehavior::And:
			ResultValue = CurrentValue && ModifierValue;
			break;
		case EKBFLBoolBehavior::Or:
			ResultValue = CurrentValue || ModifierValue;
			break;
		case EKBFLBoolBehavior::Xor:
			ResultValue = CurrentValue ^ ModifierValue;
			break;
		}

		BoolProp->SetPropertyValue(DestValuePtr, ResultValue);

		UE_LOGFMT(LogKBFLCDOOverwrite, Warning,
				  "ApplyToDataAssetInstance: Bool operation on '{PropertyName}' on '{TargetName}' | Asset: {AssetPath} "
				  "| {OldValue} -> {NewValue}",
				  Property->GetName(), TargetInstance->GetName(), AssetPath, CurrentValue, ResultValue);
	}
}

void UKBFLPrimaryDataAssetOverwrite::ApplyStringProperty(FProperty* Property, void* ContainerValuePtr,
														 void* DestValuePtr, EKBFLStringBehavior Behavior,
														 const FString& Separator, const FString& AssetPath,
														 UObject* TargetInstance)
{
	if (FStrProperty* StrProp = CastField<FStrProperty>(Property))
	{
		FString CurrentValue = StrProp->GetPropertyValue(DestValuePtr);
		FString ModifierValue = StrProp->GetPropertyValue(ContainerValuePtr);
		FString ResultValue;

		switch (Behavior)
		{
		case EKBFLStringBehavior::Replace:
			ResultValue = ModifierValue;
			break;
		case EKBFLStringBehavior::Append:
			ResultValue = CurrentValue + Separator + ModifierValue;
			break;
		case EKBFLStringBehavior::Prepend:
			ResultValue = ModifierValue + Separator + CurrentValue;
			break;
		case EKBFLStringBehavior::Clear:
			ResultValue = TEXT("");
			break;
		}

		StrProp->SetPropertyValue(DestValuePtr, ResultValue);

		UE_LOGFMT(LogKBFLCDOOverwrite, Warning,
				  "ApplyToDataAssetInstance: String operation on '{PropertyName}' on '{TargetName}' | Asset: "
				  "{AssetPath} | {OldValue} -> {NewValue}",
				  Property->GetName(), TargetInstance->GetName(), AssetPath, CurrentValue, ResultValue);
	}
	else if (FNameProperty* NameProp = CastField<FNameProperty>(Property))
	{
		FName CurrentValue = NameProp->GetPropertyValue(DestValuePtr);
		FName ModifierValue = NameProp->GetPropertyValue(ContainerValuePtr);
		FName ResultValue;

		switch (Behavior)
		{
		case EKBFLStringBehavior::Replace:
			ResultValue = ModifierValue;
			break;
		case EKBFLStringBehavior::Append:
			ResultValue = FName(*(CurrentValue.ToString() + Separator + ModifierValue.ToString()));
			break;
		case EKBFLStringBehavior::Prepend:
			ResultValue = FName(*(ModifierValue.ToString() + Separator + CurrentValue.ToString()));
			break;
		case EKBFLStringBehavior::Clear:
			ResultValue = NAME_None;
			break;
		}

		NameProp->SetPropertyValue(DestValuePtr, ResultValue);

		UE_LOGFMT(LogKBFLCDOOverwrite, Warning,
				  "ApplyToDataAssetInstance: Name operation on '{PropertyName}' on '{TargetName}' | Asset: {AssetPath} "
				  "| {OldValue} -> {NewValue}",
				  Property->GetName(), TargetInstance->GetName(), AssetPath,
				  CurrentValue.ToString(), ResultValue.ToString());
	}
	else if (FTextProperty* TextProp = CastField<FTextProperty>(Property))
	{
		FText CurrentValue = TextProp->GetPropertyValue(DestValuePtr);
		FText ModifierValue = TextProp->GetPropertyValue(ContainerValuePtr);
		FText ResultValue;

		switch (Behavior)
		{
		case EKBFLStringBehavior::Replace:
			ResultValue = ModifierValue;
			break;
		case EKBFLStringBehavior::Append:
			ResultValue = FText::FromString(CurrentValue.ToString() + Separator + ModifierValue.ToString());
			break;
		case EKBFLStringBehavior::Prepend:
			ResultValue = FText::FromString(ModifierValue.ToString() + Separator + CurrentValue.ToString());
			break;
		case EKBFLStringBehavior::Clear:
			ResultValue = FText::GetEmpty();
			break;
		}

		TextProp->SetPropertyValue(DestValuePtr, ResultValue);

		UE_LOGFMT(LogKBFLCDOOverwrite, Warning,
				  "ApplyToDataAssetInstance: Text operation on '{PropertyName}' on '{TargetName}' | Asset: {AssetPath} "
				  "| {OldValue} -> {NewValue}",
				  Property->GetName(), TargetInstance->GetName(), AssetPath,
				  CurrentValue.ToString(), ResultValue.ToString());
	}
}

void UKBFLPrimaryDataAssetOverwrite::ApplyPropertyWithBehavior(FProperty* Property, void* ContainerValuePtr,
															   void* DestValuePtr,
															   const FKBFLCDOOverwriteProperty& PropSettings,
															   const FString& AssetPath, UObject* TargetInstance)
{
	if (PropSettings.bSkipThisField)
	{
		return;
	}

	EKBFLPropertyType PropertyType = PropSettings.mPropertyType;

	// Handle based on property type
	switch (PropertyType)
	{
	case EKBFLPropertyType::Array:
	case EKBFLPropertyType::Set:
	case EKBFLPropertyType::Map:
		ApplyCollectionProperty(Property, ContainerValuePtr, DestValuePtr, PropSettings.mCollectionBehavior, AssetPath,
								TargetInstance);
		break;

	case EKBFLPropertyType::Int32:
	case EKBFLPropertyType::Int64:
	case EKBFLPropertyType::UInt32:
	case EKBFLPropertyType::UInt64:
	case EKBFLPropertyType::Float:
	case EKBFLPropertyType::Double:
		if (PropSettings.mNumericBehavior != EKBFLNumericBehavior::Replace)
		{
			ApplyNumericProperty(Property, ContainerValuePtr, DestValuePtr, PropSettings.mNumericBehavior,
								 PropSettings.mMinValue, PropSettings.mMaxValue, AssetPath, TargetInstance);
		}
		else
		{
			FString OldValueStr;
			Property->ExportTextItem_Direct(OldValueStr, DestValuePtr, nullptr, nullptr, PPF_None);

			Property->CopyCompleteValue(DestValuePtr, ContainerValuePtr);

			FString NewValueStr;
			Property->ExportTextItem_Direct(NewValueStr, DestValuePtr, nullptr, nullptr, PPF_None);

			UE_LOGFMT(LogKBFLCDOOverwrite, Warning,
					  "ApplyToDataAssetInstance: Replacing numeric property '{PropertyName}' on '{TargetName}' | "
					  "Asset: {AssetPath} | {OldValue} -> {NewValue}",
					  Property->GetName(), TargetInstance->GetName(), AssetPath, OldValueStr, NewValueStr);
		}
		break;

	case EKBFLPropertyType::Bool:
		ApplyBoolProperty(Property, ContainerValuePtr, DestValuePtr, PropSettings.mBoolBehavior, AssetPath,
						  TargetInstance);
		break;

	case EKBFLPropertyType::String:
	case EKBFLPropertyType::Name:
	case EKBFLPropertyType::Text:
		if (PropSettings.mStringBehavior != EKBFLStringBehavior::Replace)
		{
			ApplyStringProperty(Property, ContainerValuePtr, DestValuePtr, PropSettings.mStringBehavior,
								PropSettings.mStringSeparator, AssetPath, TargetInstance);
		}
		else
		{
			FString OldValueStr;
			Property->ExportTextItem_Direct(OldValueStr, DestValuePtr, nullptr, nullptr, PPF_None);

			Property->CopyCompleteValue(DestValuePtr, ContainerValuePtr);

			FString NewValueStr;
			Property->ExportTextItem_Direct(NewValueStr, DestValuePtr, nullptr, nullptr, PPF_None);

			UE_LOGFMT(LogKBFLCDOOverwrite, Warning,
					  "ApplyToDataAssetInstance: Replacing string property '{PropertyName}' on '{TargetName}' | Asset: "
					  "{AssetPath} | {OldValue} -> {NewValue}",
					  Property->GetName(), TargetInstance->GetName(), AssetPath, OldValueStr, NewValueStr);
		}
		break;

	case EKBFLPropertyType::Component:
		// For components in DataAssets: apply changes to the same component instance
		if (FObjectProperty* ObjectProp = CastField<FObjectProperty>(Property))
		{
			UObject* SourceComponent = ObjectProp->GetObjectPropertyValue(ContainerValuePtr);
			UObject* DestComponent = ObjectProp->GetObjectPropertyValue(DestValuePtr);

			if (SourceComponent && DestComponent && SourceComponent->GetClass() == DestComponent->GetClass())
			{
				// Copy properties from source component to dest component
				for (TFieldIterator<FProperty> PropIt(SourceComponent->GetClass()); PropIt; ++PropIt)
				{
					FProperty* CompProp = *PropIt;
					if (CompProp->HasAnyPropertyFlags(CPF_Edit))
					{
						void* SourcePropPtr = CompProp->ContainerPtrToValuePtr<void>(SourceComponent);
						void* DestPropPtr = CompProp->ContainerPtrToValuePtr<void>(DestComponent);
						CompProp->CopyCompleteValue(DestPropPtr, SourcePropPtr);
					}
				}
				UE_LOGFMT(LogKBFLCDOOverwrite, Warning,
						  "ApplyToDataAssetInstance: Applied changes to component property '{PropertyName}' on "
						  "'{TargetName}' | Asset: {AssetPath}",
						  Property->GetName(), TargetInstance->GetName(), AssetPath);
			}
		}
		break;

	case EKBFLPropertyType::Object:
	case EKBFLPropertyType::Class:
		// For objects: replace the object reference
		Property->CopyCompleteValue(DestValuePtr, ContainerValuePtr);
		UE_LOGFMT(LogKBFLCDOOverwrite, Warning,
				  "ApplyToDataAssetInstance: Replacing object or class property '{PropertyName}' on '{TargetName}' | "
				  "Asset: {AssetPath}",
				  Property->GetName(), TargetInstance->GetName(), AssetPath);
		break;

	default:
		// Default: simple replace
		Property->CopyCompleteValue(DestValuePtr, ContainerValuePtr);
		FString NewValue;
		Property->ExportTextItem_Direct(NewValue, ContainerValuePtr, nullptr, nullptr, PPF_None);
		UE_LOGFMT(LogKBFLCDOOverwrite, Warning,
				  "ApplyToDataAssetInstance: Applying property '{PropertyName}' on '{TargetName}' | Asset: {AssetPath} "
				  "| Value: {NewValue}",
				  Property->GetName(), TargetInstance->GetName(), AssetPath, NewValue);
		break;
	}
}

void UKBFLPrimaryDataAssetOverwrite::ApplyToDataAssetInstance(UObject* TargetInstance)
{
	const FString AssetPath = GetPathName();
	const FString TargetName = TargetInstance ? TargetInstance->GetName() : TEXT("Invalid");

	// Check requirements
	if (!Requirements_IsMet(TargetInstance))
	{
		return;
	}


	if (!TargetInstance || !mPropertyContainer || !mTargetDataAssetClass)
	{
		UE_LOGFMT(LogKBFLCDOOverwrite, Warning,
				  "ApplyToDataAssetInstance: Invalid target instance or property container | Asset: {AssetPath}",
				  AssetPath);
		return;
	}

	// Determine which class to check against
	TSubclassOf<UDataAsset> ClassToCheck =
		bTargetOnlyAsContainer ? LoadSoftClass(mRealTargetClass) : mTargetDataAssetClass;

	// In container mode, if mRealTargetClass is not set, we still apply to any matching target from mOtherTargetClasses
	if (bTargetOnlyAsContainer && !mRealTargetClass.IsValid())
	{
		// Check if instance matches any of the other target classes
		bool bMatchesOtherTarget = false;
		for (const TSoftClassPtr<UDataAsset>& SoftClass : mOtherTargetClasses)
		{
			if (TSubclassOf<UDataAsset> LoadedClass = SoftClass.LoadSynchronous())
			{
				if (TargetInstance->IsA(LoadedClass))
				{
					bMatchesOtherTarget = true;
					break;
				}
			}
		}

		if (!bMatchesOtherTarget)
		{
			UE_LOGFMT(LogKBFLCDOOverwrite, Warning,
					  "ApplyToDataAssetInstance: TargetInstance '{TargetName}' does not match any target class in "
					  "container mode | Asset: {AssetPath}",
					  TargetName, AssetPath);
			return;
		}
	}
	else if (ClassToCheck && !TargetInstance->IsA(ClassToCheck))
	{
		UE_LOGFMT(LogKBFLCDOOverwrite, Warning,
				  "ApplyToDataAssetInstance: TargetInstance '{TargetName}' is not of the expected type "
				  "'{ExpectedType}' | Asset: {AssetPath}",
				  TargetName, *ClassToCheck->GetName(), AssetPath);
		return;
	}

	// Merge modified properties with manual overrides (manual overrides take priority)
	TSet<FKBFLCDOOverwriteProperty> MergedProperties = mModifiedProperties;

	// Get the class to use for property lookup
	UClass* PropertyClass = mPropertyContainer->GetClass();

	// Add/Override with manual properties
	for (const FKBFLCDOOverwriteProperty& ManualProp : mManuelPropertiesOverwrite)
	{
		// Validate that the property exists on the property container class
		FProperty* Property = FindFProperty<FProperty>(PropertyClass, ManualProp.mPropertyName);
		if (!Property)
		{
			UE_LOGFMT(
				LogKBFLCDOOverwrite, Warning,
				"ApplyToDataAssetInstance: Manual property '{PropertyName}' not found on property container class "
				"'{ClassName}' | Asset: {AssetPath}",
				ManualProp.mPropertyName.ToString(), PropertyClass->GetName(), AssetPath);
			continue;
		}

		// Remove existing entry if present and add manual property (which takes priority)
		MergedProperties.Remove(ManualProp);
		MergedProperties.Add(ManualProp);
	}

	// Remove skipped properties
	for (const FKBFLCDOOverwriteProperty& ManualProp : mManuelPropertiesOverwrite)
	{
		if (ManualProp.bSkipThisField)
		{
			MergedProperties.Remove(ManualProp);
			UE_LOGFMT(LogKBFLCDOOverwrite, Log,
					  "ApplyToDataAssetInstance: Skipped '{PropertyName}' | Asset: {AssetPath}",
					  ManualProp.mPropertyName.ToString(), AssetPath);
		}
	}

	// Iterate all properties and copy those that are in our merged set
	for (TFieldIterator<FProperty> PropIt(PropertyClass, EFieldIteratorFlags::IncludeSuper); PropIt; ++PropIt)
	{
		FProperty* Property = *PropIt;

		// Find the property in our merged set
		const FKBFLCDOOverwriteProperty* MergedProp =
			MergedProperties.Find(FKBFLCDOOverwriteProperty(Property->GetFName()));
		if (!MergedProp)
		{
			continue;
		}

		// Skip if user marked this field to be skipped
		if (MergedProp->bSkipThisField)
		{
			continue;
		}

		void* ContainerValuePtr = Property->ContainerPtrToValuePtr<void>(mPropertyContainer);
		void* DestValuePtr = Property->ContainerPtrToValuePtr<void>(TargetInstance);

		// Use the unified property application function
		ApplyPropertyWithBehavior(Property, ContainerValuePtr, DestValuePtr, *MergedProp, AssetPath, TargetInstance);
	}

	mAppliedInstances.Add(TargetInstance);
}

void UKBFLPrimaryDataAssetOverwrite::ApplyToInstances()
{
	const FString AssetPath = GetPathName();

	// If mTargetDataAssetInstance is set, only apply to that specific instance
	if (mTargetDataAssetInstance)
	{
		UE_LOGFMT(LogKBFLCDOOverwrite, Log,
				  "ApplyToInstances: Applying to specific instance '{0}' | Asset: {AssetPath}",
				  mTargetDataAssetInstance->GetPathName(), AssetPath);

		Requirements_NotifyOnModify(mTargetDataAssetInstance);
		ApplyToDataAssetInstance(mTargetDataAssetInstance);
		Requirements_NotifyOnModified(mTargetDataAssetInstance);
		return;
	}

	// Determine which class to use for targeting
	TSubclassOf<UDataAsset> EffectiveTargetClass =
		bTargetOnlyAsContainer ? LoadSoftClass(mRealTargetClass) : mTargetDataAssetClass;

	// In non-container mode, EffectiveTargetClass must be set
	if (!bTargetOnlyAsContainer && !EffectiveTargetClass)
	{
		UE_LOGFMT(
			LogKBFLCDOOverwrite, Error,
			"ApplyToInstances: No valid target DataAsset class (mTargetDataAssetClass is null) | Asset: {AssetPath}",
			AssetPath);
		return;
	}

	// Log if using container mode
	if (bTargetOnlyAsContainer)
	{
		UE_LOGFMT(LogKBFLCDOOverwrite, Log,
				  "ApplyToInstances: Using TargetOnlyAsContainer mode - PropertyContainer: {0}, RealTarget: {1} | "
				  "Asset: {AssetPath}",
				  mTargetDataAssetClass ? *mTargetDataAssetClass->GetName() : TEXT("None"),
				  mRealTargetClass ? *mRealTargetClass->GetName()
								   : TEXT("None (will use mFindAssetsInPaths or mOtherTargetClasses)"),
				  AssetPath);
	}

	// Collect all target classes
	TSet<TSubclassOf<UDataAsset>> TargetClasses;

	// Add effective target class
	if (EffectiveTargetClass)
	{
		TargetClasses.Add(EffectiveTargetClass);
	}

	// Add other target classes
	for (const TSoftClassPtr<UDataAsset>& SoftClass : mOtherTargetClasses)
	{
		if (TSubclassOf<UDataAsset> LoadedClass = SoftClass.LoadSynchronous())
		{
			TargetClasses.Add(LoadedClass);
		}
	}

	// Collect all PrimaryDataAsset instances to process
	TArray<UDataAsset*> AssetsToProcess;

	// Add specific assets
	for (const TSoftObjectPtr<UDataAsset>& SoftAsset : mSpecificAssets)
	{
		if (UDataAsset* Asset = SoftAsset.LoadSynchronous())
		{
			// Check if asset matches any target class
			bool bMatchesTargetClass = false;
			for (const TSubclassOf<UDataAsset>& TargetClass : TargetClasses)
			{
				if (Asset->IsA(TargetClass))
				{
					bMatchesTargetClass = true;
					break;
				}
			}

			if (bMatchesTargetClass)
			{
				AssetsToProcess.Add(Asset);
			}
		}
	}

	// Find assets in specified paths using Asset Registry
	if (mFindAssetsInPaths.Num() > 0)
	{
		FAssetRegistryModule& AssetRegistryModule =
			FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

		// Safety guard: do not query the Asset Registry while it is still loading assets.
		// Calling GetAssetsByPath during asset discovery can cause an EXCEPTION_ACCESS_VIOLATION.
		if (AssetRegistry.IsLoadingAssets())
		{
			UE_LOGFMT(LogKBFLCDOOverwrite, Warning,
					  "ApplyToInstances: AssetRegistry is still loading assets - skipping path-based search for '{0}'. "
					  "mFindAssetsInPaths will not be processed. | Asset: {AssetPath}",
					  *GetName(), AssetPath);
		}
		else
		{

			// Convert paths to proper format
			TArray<FName> PathNames;
			for (const FString& Path : mFindAssetsInPaths)
			{
				PathNames.Add(FName(*Path));
			}

			// Find all assets in these paths
			TArray<FAssetData> FoundAssets;
			for (const FName& PathName : PathNames)
			{
				TArray<FAssetData> PathAssets;
				AssetRegistry.GetAssetsByPath(PathName, PathAssets, true);
				FoundAssets.Append(PathAssets);
			}

			// Filter by class and load
			for (const FAssetData& AssetData : FoundAssets)
			{
				// Get the native class of the asset (not the Blueprint class)
				UClass* AssetNativeClass = AssetData.GetClass();
				if (!AssetNativeClass)
				{
					continue;
				}

				// Check if this is a PrimaryDataAsset by checking the asset's native class
				// For data assets, we need to load them to check their actual type
				if (!AssetNativeClass->IsChildOf(UDataAsset::StaticClass()))
				{
					// Try to load and check - some assets may be PrimaryDataAssets
					UObject* LoadedObject = AssetData.GetAsset();
					if (!LoadedObject)
					{
						continue;
					}

					if (!LoadedObject->IsA(UDataAsset::StaticClass()))
					{
						continue;
					}
				}

				// Load the asset to get its actual class for proper IsA checking
				UDataAsset* LoadedAsset = Cast<UDataAsset>(AssetData.GetAsset());
				if (!LoadedAsset)
				{
					continue;
				}

				// Check if asset matches any target class using IsA on the loaded asset
				bool bMatchesTargetClass = false;
				for (const TSubclassOf<UDataAsset>& TargetClass : TargetClasses)
				{
					if (TargetClass && LoadedAsset->IsA(TargetClass))
					{
						bMatchesTargetClass = true;
						break;
					}
				}

				if (!bMatchesTargetClass)
				{
					continue;
				}

				// Check if not in ignore list
				bool bShouldIgnore = false;
				for (const TSoftObjectPtr<UDataAsset>& IgnoreAsset : mIgnoreAssets)
				{
					if (IgnoreAsset.Get() == LoadedAsset)
					{
						bShouldIgnore = true;
						break;
					}
				}

				if (bShouldIgnore)
				{
					continue;
				}

				if (!AssetsToProcess.Contains(LoadedAsset))
				{
					AssetsToProcess.Add(LoadedAsset);
				}
			}
		} // end else (AssetRegistry not loading)
	}

	// Sort assets by path hash for consistent ordering
	AssetsToProcess.Sort([](const UDataAsset& A, const UDataAsset& B)
						 { return GetTypeHash(A.GetPathName()) < GetTypeHash(B.GetPathName()); });

	UE_LOGFMT(LogKBFLCDOOverwrite, Log,
			  "ApplyToInstances: Found {0} PrimaryDataAsset instance(s) to process | Asset: {AssetPath}",
			  AssetsToProcess.Num(), AssetPath);

	// Apply overrides to all collected assets
	for (UDataAsset* Asset : AssetsToProcess)
	{
		Requirements_NotifyOnModify(Asset);
		ApplyToDataAssetInstance(Asset);
		Requirements_NotifyOnModified(Asset);
	}
}
