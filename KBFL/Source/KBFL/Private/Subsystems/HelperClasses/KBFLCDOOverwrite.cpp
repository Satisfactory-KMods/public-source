// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#include "Subsystems/HelperClasses/KBFLCDOOverwrite.h"
#include "Logging/StructuredLog.h"
#include "Misc/DataValidation.h"
#include "Subsystems/KBFLAssetDataSubsystem.h"
#include "Subsystems/KBFLContentCDOHelperSubsystem.h"

bool FKBFLCDOOverwriteProperty::ShouldSkipByDefault() const
{
	if (mPropertyType == EKBFLPropertyType::Component)
	{
		return true;
	}

	if (mPropertyType == EKBFLPropertyType::Object)
	{

		if (const UClass* ObjectClass = mObjectPropertyClass.Get())
		{

			static const TArray<FName> AssetBaseClassNames = {
				TEXT("Texture"),
				TEXT("StaticMesh"),
				TEXT("SkeletalMesh"),
				TEXT("Material"),
				TEXT("MaterialInterface"),
				TEXT("SoundBase"),
				TEXT("ParticleSystem"),
				TEXT("NiagaraSystem"),
				TEXT("AnimationAsset"),
				TEXT("DataAsset"),
				TEXT("CurveBase"),
				TEXT("DataTable"),
				TEXT("Blueprint"),
				TEXT("PhysicsAsset"),
				TEXT("PhysicalMaterial"),
			};

			for (const UClass* CurrentClass = ObjectClass; CurrentClass != nullptr;
				 CurrentClass = CurrentClass->GetSuperClass())
			{
				if (AssetBaseClassNames.Contains(CurrentClass->GetFName()))
				{
					return false;
				}
			}
		}

		return true;
	}

	return false;
}
UKBFLCDOOverwrite::UKBFLCDOOverwrite() {}

void UKBFLCDOOverwrite::PostLoad()
{
	Super::PostLoad();

	if (mTargetClass && !mNativeClass)
	{
		mNativeClass = GetSuperClass();
	}

	if (mTargetClass)
	{
		if (!mPropertyContainer)
		{

			UE_LOG(LogKBFLCDOOverwrite, Warning, TEXT("PostLoad: Property container is null for '%s', recreating..."),
				   *GetPathName());
			RefreshPropertyContainer();
		}
		else if (!mPropertyContainer->IsA(mTargetClass))
		{

			UE_LOG(
				LogKBFLCDOOverwrite, Warning,
				TEXT("PostLoad: Property container class mismatch for '%s'. Expected '%s' but got '%s'. Recreating..."),
				*GetPathName(), *mTargetClass->GetName(), *mPropertyContainer->GetClass()->GetName());

			mPropertyContainer->MarkAsGarbage();
			mPropertyContainer = nullptr;
			RefreshPropertyContainer();
		}
	}
}

#if WITH_EDITOR
void UKBFLCDOOverwrite::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	FName ChangedPropertyName = PropertyChangedEvent.GetPropertyName();

	if (ChangedPropertyName == GET_MEMBER_NAME_CHECKED(UKBFLCDOOverwrite, mTargetClass))
	{

		mModifiedProperties.Empty();
		mManuelPropertiesOverwrite.Empty();
		RefreshPropertyContainer();
	}
	else if (ChangedPropertyName == GET_MEMBER_NAME_CHECKED(UKBFLCDOOverwrite, mManuelPropertiesOverwrite))
	{

		ValidateManualProperties();
	}
}

void UKBFLCDOOverwrite::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	if (mPropertyContainer && IsValid(mTargetClass))
	{
		FEditPropertyChain::TDoubleLinkedListNode* PropertyNode = PropertyChangedEvent.PropertyChain.GetHead();
		if (PropertyNode)
		{
			FProperty* HeadProperty = PropertyNode->GetValue();
			if (HeadProperty &&
				HeadProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UKBFLCDOOverwrite, mPropertyContainer))
			{

				RebuildModifiedProperties();
			}
		}
	}
}

EDataValidationResult UKBFLCDOOverwrite::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	if (!IsValid(mTargetClass))
	{
		Context.AddError(FText::FromString(TEXT("Target Class must be set.")));
		Result = EDataValidationResult::Invalid;
	}

	TSubclassOf<UObject> LoadedNativeClass = mNativeClass;
	for (const TSubclassOf<UObject>& Class : LoadSoftClassesArray(mAlsoApplyOn))
	{
		if (Class && LoadedNativeClass && !Class->IsChildOf(LoadedNativeClass))
		{
			Context.AddError(FText::Format(NSLOCTEXT("KBFLCDOOverwrite", "InvalidSubclass",
													 "'{0}' is not a subclass of '{1}' and will be ignored."),
										   FText::FromString(Class->GetName()),
										   FText::FromString(LoadedNativeClass->GetName())));
			Result = EDataValidationResult::Invalid;
		}
	}

	if (mTargetClass)
	{
		for (const FKBFLCDOOverwriteProperty& ManualProp : mManuelPropertiesOverwrite)
		{

			if (ManualProp.mPropertyName.IsNone())
			{
				Context.AddError(FText::FromString(TEXT("Manual property override has empty property name.")));
				Result = EDataValidationResult::Invalid;
				continue;
			}

			FProperty* Property = FindFProperty<FProperty>(mTargetClass, ManualProp.mPropertyName);
			if (!Property)
			{
				Context.AddError(FText::Format(
					NSLOCTEXT("KBFLCDOOverwrite", "PropertyNotFound", "Property '{0}' does not exist on class '{1}'."),
					FText::FromName(ManualProp.mPropertyName), FText::FromString(mTargetClass->GetName())));
				Result = EDataValidationResult::Invalid;
				continue;
			}

			EKBFLPropertyType ActualType = GetPropertyType(Property);

			if (ManualProp.mPropertyType != EKBFLPropertyType::Unknown && ManualProp.mPropertyType != ActualType)
			{
				Context.AddError(FText::Format(NSLOCTEXT("KBFLCDOOverwrite", "PropertyTypeMismatch",
														 "Property '{0}' type mismatch: Expected {1}, but actual "
														 "type is {2}."),
											   FText::FromName(ManualProp.mPropertyName),
											   FText::AsNumber(static_cast<int32>(ManualProp.mPropertyType)),
											   FText::AsNumber(static_cast<int32>(ActualType))));
				Result = EDataValidationResult::Invalid;
			}

			bool bIsNumeric = (ActualType == EKBFLPropertyType::Int32 || ActualType == EKBFLPropertyType::Int64 ||
							   ActualType == EKBFLPropertyType::UInt32 || ActualType == EKBFLPropertyType::UInt64 ||
							   ActualType == EKBFLPropertyType::Float || ActualType == EKBFLPropertyType::Double);

			if (ManualProp.mNumericBehavior != EKBFLNumericBehavior::Replace && !bIsNumeric)
			{
				Context.AddError(FText::Format(NSLOCTEXT("KBFLCDOOverwrite", "NumericBehaviorOnNonNumeric",
														 "Property '{0}' has numeric behavior set but is not a "
														 "numeric type."),
											   FText::FromName(ManualProp.mPropertyName)));
				Result = EDataValidationResult::Invalid;
			}

			if (ManualProp.mNumericBehavior == EKBFLNumericBehavior::Clamp)
			{
				if (ManualProp.mMinValue >= ManualProp.mMaxValue)
				{
					Context.AddError(FText::Format(NSLOCTEXT("KBFLCDOOverwrite", "InvalidClampRange",
															 "Property '{0}': Min value ({1}) must be "
															 "less than Max value ({2})."),
												   FText::FromName(ManualProp.mPropertyName),
												   FText::AsNumber(ManualProp.mMinValue),
												   FText::AsNumber(ManualProp.mMaxValue)));
					Result = EDataValidationResult::Invalid;
				}
			}
		}
	}

	return Result;
}

void UKBFLCDOOverwrite::ValidateManualProperties()
{
	if (!IsValid(mTargetClass))
	{
		return;
	}

	if (!mTargetClass)
	{
		return;
	}

	for (FKBFLCDOOverwriteProperty& ManualProp : mManuelPropertiesOverwrite)
	{
		if (ManualProp.mPropertyName.IsNone())
		{
			continue;
		}

		FProperty* Property = FindFProperty<FProperty>(mTargetClass, ManualProp.mPropertyName);
		if (Property)
		{
			ManualProp.mPropertyType = GetPropertyType(Property);

			if (ManualProp.mPropertyType == EKBFLPropertyType::Object)
			{
				if (const FObjectPropertyBase* ObjectProp = CastField<FObjectPropertyBase>(Property))
				{
					ManualProp.mObjectPropertyClass = ObjectProp->PropertyClass.Get();
				}
			}

			if (ManualProp.ShouldSkipByDefault() && !ManualProp.bSkipThisField)
			{
				ManualProp.bSkipThisField = true;
			}
		}
	}
}

void UKBFLCDOOverwrite::RebuildModifiedProperties()
{

	if (!mPropertyContainer || !mTargetClass)
	{
		mModifiedProperties.Empty();
		return;
	}

	UObject* CDO = mTargetClass->GetDefaultObject();
	if (!CDO)
	{
		mModifiedProperties.Empty();
		return;
	}

	TSet<FName> CurrentlyModifiedNames;

	for (TFieldIterator<FProperty> PropIt(mTargetClass, EFieldIteratorFlags::IncludeSuper); PropIt; ++PropIt)
	{
		FProperty* Property = *PropIt;

		if (!Property->HasAnyPropertyFlags(CPF_Edit))
		{
			continue;
		}

		void* ContainerValuePtr = Property->ContainerPtrToValuePtr<void>(mPropertyContainer);
		void* CDOValuePtr = Property->ContainerPtrToValuePtr<void>(CDO);

		if (!Property->Identical(ContainerValuePtr, CDOValuePtr))
		{
			CurrentlyModifiedNames.Add(Property->GetFName());

			FKBFLCDOOverwriteProperty* Existing =
				mModifiedProperties.Find(FKBFLCDOOverwriteProperty(Property->GetFName()));
			if (!Existing)
			{

				EKBFLPropertyType PropType = GetPropertyType(Property);
				FKBFLCDOOverwriteProperty NewProp(Property->GetFName(), PropType);

				if (PropType == EKBFLPropertyType::Object)
				{
					if (const FObjectPropertyBase* ObjectProp = CastField<FObjectPropertyBase>(Property))
					{
						NewProp.mObjectPropertyClass = ObjectProp->PropertyClass.Get();
					}
				}

				if (NewProp.ShouldSkipByDefault())
				{
					NewProp.bSkipThisField = true;
				}

				mModifiedProperties.Add(NewProp);
			}
		}
	}

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

EKBFLPropertyType UKBFLCDOOverwrite::GetPropertyType(const FProperty* Property)
{
	if (!Property)
	{
		return EKBFLPropertyType::Unknown;
	}

	if (Property->IsA<FArrayProperty>())
	{
		return EKBFLPropertyType::Array;
	}
	if (Property->IsA<FSetProperty>())
	{
		return EKBFLPropertyType::Set;
	}
	if (Property->IsA<FMapProperty>())
	{
		return EKBFLPropertyType::Map;
	}
	if (Property->IsA<FIntProperty>())
	{
		return EKBFLPropertyType::Int32;
	}
	if (Property->IsA<FInt64Property>())
	{
		return EKBFLPropertyType::Int64;
	}
	if (Property->IsA<FUInt32Property>())
	{
		return EKBFLPropertyType::UInt32;
	}
	if (Property->IsA<FUInt64Property>())
	{
		return EKBFLPropertyType::UInt64;
	}
	if (Property->IsA<FFloatProperty>())
	{
		return EKBFLPropertyType::Float;
	}
	if (Property->IsA<FDoubleProperty>())
	{
		return EKBFLPropertyType::Double;
	}
	if (Property->IsA<FBoolProperty>())
	{
		return EKBFLPropertyType::Bool;
	}
	if (Property->IsA<FStrProperty>())
	{
		return EKBFLPropertyType::String;
	}
	if (Property->IsA<FNameProperty>())
	{
		return EKBFLPropertyType::Name;
	}
	if (Property->IsA<FTextProperty>())
	{
		return EKBFLPropertyType::Text;
	}

	if (const FObjectProperty* ObjProp = CastField<FObjectProperty>(Property))
	{
		if (ObjProp->PropertyClass && ObjProp->PropertyClass->IsChildOf(UActorComponent::StaticClass()))
		{
			return EKBFLPropertyType::Component;
		}
	}
	if (Property->IsA<FClassProperty>() || Property->IsA<FSoftClassProperty>())
	{
		return EKBFLPropertyType::Class;
	}
	if (Property->IsA<FObjectProperty>() || Property->IsA<FSoftObjectProperty>())
	{
		return EKBFLPropertyType::Object;
	}
	if (Property->IsA<FStructProperty>())
	{
		return EKBFLPropertyType::Struct;
	}
	if (Property->IsA<FEnumProperty>() || Property->IsA<FByteProperty>())
	{
		return EKBFLPropertyType::Enum;
	}

	return EKBFLPropertyType::Other;
}

UClass* UKBFLCDOOverwrite::GetSuperClass() const
{

	UClass* SuperNativeClass = mTargetClass ? mTargetClass.Get() : nullptr;
	while (SuperNativeClass && SuperNativeClass->HasAnyClassFlags(CLASS_CompiledFromBlueprint))
	{
		SuperNativeClass = SuperNativeClass->GetSuperClass();
	}
	return SuperNativeClass;
}

void UKBFLCDOOverwrite::RefreshPropertyContainer()
{

	if (mPropertyContainer)
	{
		mPropertyContainer->MarkAsGarbage();
		mPropertyContainer = nullptr;
	}
	mModifiedProperties.Empty();
	mNativeClass = GetSuperClass();

	if (!mTargetClass)
	{
		return;
	}

	UObject* TargetCDO = mTargetClass->GetDefaultObject();
	if (!TargetCDO)
	{
		UE_LOG(LogKBFLCDOOverwrite, Error, TEXT("RefreshPropertyContainer: Could not get CDO for class '%s'"),
			   *mTargetClass->GetName());
		return;
	}

	FObjectDuplicationParameters DupParams(TargetCDO, this);
	DupParams.DestName = MakeUniqueObjectName(this, mTargetClass, FName(TEXT("PropertyContainer")));
	DupParams.FlagMask = RF_AllFlags & ~(RF_ClassDefaultObject | RF_ArchetypeObject | RF_DefaultSubObject);
	DupParams.ApplyFlags = RF_NoFlags;

	DupParams.InternalFlagMask = EInternalObjectFlags_AllFlags;

	mPropertyContainer = StaticDuplicateObjectEx(DupParams);

	if (!mPropertyContainer)
	{
		UE_LOG(LogKBFLCDOOverwrite, Error, TEXT("RefreshPropertyContainer: Failed to duplicate CDO for class '%s'"),
			   *mTargetClass->GetName());
		return;
	}

	UE_LOG(LogKBFLCDOOverwrite, Log,
		   TEXT("RefreshPropertyContainer: Successfully created property "
				"container of class '%s' for target '%s'"),
		   *mPropertyContainer->GetClass()->GetName(), *mTargetClass->GetName());
}

void UKBFLCDOOverwrite::ValidateAndDetectPropertyTypes()
{
#if WITH_EDITOR
	ValidateManualProperties();
#endif
}

void UKBFLCDOOverwrite::ApplyCollectionProperty(FProperty* Property, void* ContainerValuePtr, void* DestValuePtr,
												EKBFLBehaivor Behavior, const FString& AssetPath,
												UObject* TargetInstance)
{
	FString NewValue;
	Property->ExportTextItem_Direct(NewValue, ContainerValuePtr, nullptr, nullptr, PPF_None);

	if (Behavior == EKBFLBehaivor::Replace)
	{
		Property->CopyCompleteValue(DestValuePtr, ContainerValuePtr);
		UE_LOGFMT(LogKBFLCDOOverwrite, Warning,
				  "ApplyToInstance: Replacing collection property '{PropertyName}' "
				  "on '{TargetName}' | Asset: {AssetPath}",
				  Property->GetName(), TargetInstance->GetName(), AssetPath);
		return;
	}

	if (FArrayProperty* ArrayProp = CastField<FArrayProperty>(Property))
	{
		FScriptArrayHelper SourceArray(ArrayProp, ContainerValuePtr);
		FScriptArrayHelper DestArray(ArrayProp, DestValuePtr);

		for (int32 i = 0; i < SourceArray.Num(); ++i)
		{
			bool bShouldAdd = true;

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
				  "ApplyToInstance: Merging array property '{PropertyName}' on "
				  "'{TargetName}' | Asset: {AssetPath} | Mode: {Mode}",
				  Property->GetName(), TargetInstance->GetName(), AssetPath,
				  Behavior == EKBFLBehaivor::MergeUnique ? TEXT("MergeUnique") : TEXT("Merge"));
	}

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

		UE_LOGFMT(LogKBFLCDOOverwrite, Warning,
				  "ApplyToInstance: Merging set property '{PropertyName}' on "
				  "'{TargetName}' | Asset: {AssetPath}",
				  Property->GetName(), TargetInstance->GetName(), AssetPath);
	}

	else if (FMapProperty* MapProp = CastField<FMapProperty>(Property))
	{
		FScriptMapHelper SourceMap(MapProp, ContainerValuePtr);
		FScriptMapHelper DestMap(MapProp, DestValuePtr);

		for (int32 i = 0; i < SourceMap.Num(); ++i)
		{
			if (SourceMap.IsValidIndex(i))
			{
				DestMap.AddPair(SourceMap.GetKeyPtr(i), SourceMap.GetValuePtr(i));
			}
		}

		UE_LOGFMT(LogKBFLCDOOverwrite, Warning,
				  "ApplyToInstance: Merging map property '{PropertyName}' on "
				  "'{TargetName}' | Asset: {AssetPath}",
				  Property->GetName(), TargetInstance->GetName(), AssetPath);
	}
}

void UKBFLCDOOverwrite::ApplyNumericProperty(FProperty* Property, void* ContainerValuePtr, void* DestValuePtr,
											 EKBFLNumericBehavior Behavior, double MinValue, double MaxValue,
											 const FString& AssetPath, UObject* TargetInstance)
{
	auto ApplyOperation = [Behavior, MinValue, MaxValue](auto CurrentValue,
														 auto ModifierValue) -> decltype(CurrentValue)
	{
		switch (Behavior)
		{
		case EKBFLNumericBehavior::Add:
			return CurrentValue + ModifierValue;
		case EKBFLNumericBehavior::Subtract:
			return CurrentValue - ModifierValue;
		case EKBFLNumericBehavior::Multiply:
			return CurrentValue * ModifierValue;
		case EKBFLNumericBehavior::Divide:
			return ModifierValue != 0 ? CurrentValue / ModifierValue : CurrentValue;
		case EKBFLNumericBehavior::Min:
			return FMath::Min(CurrentValue, ModifierValue);
		case EKBFLNumericBehavior::Max:
			return FMath::Max(CurrentValue, ModifierValue);
		case EKBFLNumericBehavior::Clamp:
			return FMath::Clamp(CurrentValue, static_cast<decltype(CurrentValue)>(MinValue),
								static_cast<decltype(CurrentValue)>(MaxValue));
		default:
			return ModifierValue;
		}
	};

	if (FIntProperty* IntProp = CastField<FIntProperty>(Property))
	{
		int32 CurrentValue = IntProp->GetPropertyValue(DestValuePtr);
		int32 ModifierValue = IntProp->GetPropertyValue(ContainerValuePtr);
		int32 ResultValue = ApplyOperation(CurrentValue, ModifierValue);
		IntProp->SetPropertyValue(DestValuePtr, ResultValue);

		UE_LOGFMT(LogKBFLCDOOverwrite, Warning,
				  "ApplyToInstance: Numeric operation on '{PropertyName}' on "
				  "'{TargetName}' | Asset: {AssetPath} | {OldValue} -> {NewValue}",
				  Property->GetName(), TargetInstance->GetName(), AssetPath, CurrentValue, ResultValue);
	}
	else if (FInt64Property* Int64Prop = CastField<FInt64Property>(Property))
	{
		int64 CurrentValue = Int64Prop->GetPropertyValue(DestValuePtr);
		int64 ModifierValue = Int64Prop->GetPropertyValue(ContainerValuePtr);
		int64 ResultValue = ApplyOperation(CurrentValue, ModifierValue);
		Int64Prop->SetPropertyValue(DestValuePtr, ResultValue);

		UE_LOGFMT(LogKBFLCDOOverwrite, Warning,
				  "ApplyToInstance: Numeric operation on '{PropertyName}' on "
				  "'{TargetName}' | Asset: {AssetPath} | {OldValue} -> {NewValue}",
				  Property->GetName(), TargetInstance->GetName(), AssetPath, CurrentValue, ResultValue);
	}
	else if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Property))
	{
		float CurrentValue = FloatProp->GetPropertyValue(DestValuePtr);
		float ModifierValue = FloatProp->GetPropertyValue(ContainerValuePtr);
		float ResultValue = ApplyOperation(CurrentValue, ModifierValue);
		FloatProp->SetPropertyValue(DestValuePtr, ResultValue);

		UE_LOGFMT(LogKBFLCDOOverwrite, Warning,
				  "ApplyToInstance: Numeric operation on '{PropertyName}' on "
				  "'{TargetName}' | Asset: {AssetPath} | {OldValue} -> {NewValue}",
				  Property->GetName(), TargetInstance->GetName(), AssetPath, CurrentValue, ResultValue);
	}
	else if (FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(Property))
	{
		double CurrentValue = DoubleProp->GetPropertyValue(DestValuePtr);
		double ModifierValue = DoubleProp->GetPropertyValue(ContainerValuePtr);
		double ResultValue = ApplyOperation(CurrentValue, ModifierValue);
		DoubleProp->SetPropertyValue(DestValuePtr, ResultValue);

		UE_LOGFMT(LogKBFLCDOOverwrite, Warning,
				  "ApplyToInstance: Numeric operation on '{PropertyName}' on "
				  "'{TargetName}' | Asset: {AssetPath} | {OldValue} -> {NewValue}",
				  Property->GetName(), TargetInstance->GetName(), AssetPath, CurrentValue, ResultValue);
	}
	else if (FUInt32Property* UInt32Prop = CastField<FUInt32Property>(Property))
	{
		uint32 CurrentValue = UInt32Prop->GetPropertyValue(DestValuePtr);
		uint32 ModifierValue = UInt32Prop->GetPropertyValue(ContainerValuePtr);
		uint32 ResultValue = ApplyOperation(CurrentValue, ModifierValue);
		UInt32Prop->SetPropertyValue(DestValuePtr, ResultValue);

		UE_LOGFMT(LogKBFLCDOOverwrite, Warning,
				  "ApplyToInstance: Numeric operation on '{PropertyName}' on "
				  "'{TargetName}' | Asset: {AssetPath} | {OldValue} -> {NewValue}",
				  Property->GetName(), TargetInstance->GetName(), AssetPath, CurrentValue, ResultValue);
	}
	else if (FUInt64Property* UInt64Prop = CastField<FUInt64Property>(Property))
	{
		uint64 CurrentValue = UInt64Prop->GetPropertyValue(DestValuePtr);
		uint64 ModifierValue = UInt64Prop->GetPropertyValue(ContainerValuePtr);
		uint64 ResultValue = ApplyOperation(CurrentValue, ModifierValue);
		UInt64Prop->SetPropertyValue(DestValuePtr, ResultValue);

		UE_LOGFMT(LogKBFLCDOOverwrite, Warning,
				  "ApplyToInstance: Numeric operation on '{PropertyName}' on "
				  "'{TargetName}' | Asset: {AssetPath} | {OldValue} -> {NewValue}",
				  Property->GetName(), TargetInstance->GetName(), AssetPath, CurrentValue, ResultValue);
	}
}

void UKBFLCDOOverwrite::ApplyBoolProperty(FProperty* Property, void* ContainerValuePtr, void* DestValuePtr,
										  EKBFLBoolBehavior Behavior, const FString& AssetPath, UObject* TargetInstance)
{
	FBoolProperty* BoolProp = CastField<FBoolProperty>(Property);
	if (!BoolProp)
	{
		return;
	}

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
		ResultValue = CurrentValue != ModifierValue;
		break;
	}

	BoolProp->SetPropertyValue(DestValuePtr, ResultValue);

	UE_LOGFMT(LogKBFLCDOOverwrite, Warning,
			  "ApplyToInstance: Bool operation on '{PropertyName}' on "
			  "'{TargetName}' | Asset: {AssetPath} | {OldValue} -> {NewValue}",
			  Property->GetName(), TargetInstance->GetName(), AssetPath, CurrentValue, ResultValue);
}

void UKBFLCDOOverwrite::ApplyStringProperty(FProperty* Property, void* ContainerValuePtr, void* DestValuePtr,
											EKBFLStringBehavior Behavior, const FString& Separator,
											const FString& AssetPath, UObject* TargetInstance)
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
				  "ApplyToInstance: String operation on '{PropertyName}' on "
				  "'{TargetName}' | Asset: {AssetPath} | {OldValue} -> {NewValue}",
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
			ResultValue = FName(CurrentValue.ToString() + Separator + ModifierValue.ToString());
			break;
		case EKBFLStringBehavior::Prepend:
			ResultValue = FName(ModifierValue.ToString() + Separator + CurrentValue.ToString());
			break;
		case EKBFLStringBehavior::Clear:
			ResultValue = NAME_None;
			break;
		}

		NameProp->SetPropertyValue(DestValuePtr, ResultValue);

		UE_LOGFMT(LogKBFLCDOOverwrite, Warning,
				  "ApplyToInstance: Name operation on '{PropertyName}' on "
				  "'{TargetName}' | Asset: {AssetPath} | {OldValue} -> {NewValue}",
				  Property->GetName(), TargetInstance->GetName(), AssetPath, CurrentValue.ToString(),
				  ResultValue.ToString());
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
				  "ApplyToInstance: Text operation on '{PropertyName}' on "
				  "'{TargetName}' | Asset: {AssetPath} | {OldValue} -> {NewValue}",
				  Property->GetName(), TargetInstance->GetName(), AssetPath, CurrentValue.ToString(),
				  ResultValue.ToString());
	}
}

void UKBFLCDOOverwrite::ApplyPropertyWithBehavior(FProperty* Property, void* ContainerValuePtr, void* DestValuePtr,
												  const FKBFLCDOOverwriteProperty& PropSettings,
												  const FString& AssetPath, UObject* TargetInstance)
{
	if (PropSettings.bSkipThisField)
	{
		return;
	}
	EKBFLPropertyType PropertyType = PropSettings.mPropertyType;

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
					  "ApplyToInstance: Replacing numeric property '{PropertyName}' "
					  "on '{TargetName}' | Asset: {AssetPath} | {OldValue} -> {NewValue}",
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
					  "ApplyToInstance: Replacing string property '{PropertyName}' "
					  "on '{TargetName}' | Asset: {AssetPath} | {OldValue} -> {NewValue}",
					  Property->GetName(), TargetInstance->GetName(), AssetPath, OldValueStr, NewValueStr);
		}
		break;

	case EKBFLPropertyType::Component:

		if (FObjectProperty* ObjectProp = CastField<FObjectProperty>(Property))
		{
			UObject* SourceComponent = ObjectProp->GetObjectPropertyValue(ContainerValuePtr);
			UObject* DestComponent = ObjectProp->GetObjectPropertyValue(DestValuePtr);

			if (SourceComponent && DestComponent && SourceComponent->GetClass() == DestComponent->GetClass())
			{

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
						  "ApplyToInstance: Applied changes to component property "
						  "'{PropertyName}' on '{TargetName}' | Asset: {AssetPath}",
						  Property->GetName(), TargetInstance->GetName(), AssetPath);
			}
			else
			{
				UE_LOGFMT(LogKBFLCDOOverwrite, Warning,
						  "ApplyToInstance: Cannot apply component property "
						  "'{PropertyName}' on '{TargetName}' - incompatible or null "
						  "components | Asset: {AssetPath}",
						  Property->GetName(), TargetInstance->GetName(), AssetPath);
			}
		}
		break;

	case EKBFLPropertyType::Object:
	case EKBFLPropertyType::Class:

		Property->CopyCompleteValue(DestValuePtr, ContainerValuePtr);
		UE_LOGFMT(LogKBFLCDOOverwrite, Warning,
				  "ApplyToInstance: Replacing object or class property "
				  "'{PropertyName}' on '{TargetName}' | Asset: {AssetPath}",
				  Property->GetName(), TargetInstance->GetName(), AssetPath);
		break;

	default:

		Property->CopyCompleteValue(DestValuePtr, ContainerValuePtr);
		FString OldValue, NewValue;
		Property->ExportTextItem_Direct(OldValue, DestValuePtr, nullptr, nullptr, PPF_None);
		Property->ExportTextItem_Direct(NewValue, ContainerValuePtr, nullptr, nullptr, PPF_None);
		UE_LOGFMT(LogKBFLCDOOverwrite, Warning,
				  "ApplyToInstance: Applying property '{PropertyName}' on "
				  "'{TargetName}' | Asset: {AssetPath} | {OldValue} -> {NewValue}",
				  Property->GetName(), TargetInstance->GetName(), AssetPath, OldValue, NewValue);
		break;
	}
}

void UKBFLCDOOverwrite::ApplyToInstance(UObject* TargetInstance)
{
	const FString AssetPath = GetPathName();
	const FString TargetName = TargetInstance ? TargetInstance->GetName() : TEXT("Invalid");

	if (!Requirements_IsMet(TargetInstance))
	{
		return;
	}

	if (!TargetInstance || !mPropertyContainer || !IsValid(mTargetClass))
	{
		UE_LOGFMT(LogKBFLCDOOverwrite, Warning,
				  "ApplyToInstance: Invalid target instance or property container. "
				  "mPropertyContainer: {mPropertyContainer} mTargetClass: {mTargetClass} "
				  "TargetInstance: {TargetInstance} | Asset: {AssetPath}",
				  IsValid(mPropertyContainer), IsValid(mTargetClass), IsValid(TargetInstance), AssetPath);
		return;
	}

	TSubclassOf<UObject> LoadedNativeClass = mNativeClass;
	if (!LoadedNativeClass || !TargetInstance->IsA(LoadedNativeClass))
	{
		UE_LOGFMT(LogKBFLCDOOverwrite, Warning,
				  "ApplyToInstance: TargetInstance '{TargetName}' is not of the "
				  "expected type. | Asset: {AssetPath}",
				  TargetName, AssetPath);
		return;
	}

	TSet<FKBFLCDOOverwriteProperty> MergedProperties = mModifiedProperties;

	for (FKBFLCDOOverwriteProperty ManualProp : mManuelPropertiesOverwrite)
	{

		FProperty* Property = FindFProperty<FProperty>(mTargetClass, ManualProp.mPropertyName);
		if (!Property)
		{
			UE_LOGFMT(LogKBFLCDOOverwrite, Warning,
					  "ApplyToInstance: Manual property '{PropertyName}' not found "
					  "on target class '{ClassName}' | Asset: {AssetPath}",
					  ManualProp.mPropertyName.ToString(), mTargetClass ? mTargetClass->GetName() : TEXT("None"),
					  AssetPath);
			continue;
		}

		MergedProperties.Remove(ManualProp);
		MergedProperties.Add(ManualProp);
	}

	for (FKBFLCDOOverwriteProperty ManualProp : mManuelPropertiesOverwrite)
	{
		if (ManualProp.bSkipThisField)
		{
			MergedProperties.Remove(ManualProp);
			UE_LOGFMT(LogKBFLCDOOverwrite, Log,
					  "ApplyToInstance: Skipped '{PropertyName}' (Type: {Type}) | "
					  "Asset: {AssetPath}",
					  ManualProp.mPropertyName.ToString(),
					  ManualProp.mPropertyType == EKBFLPropertyType::Component ? TEXT("Component") : TEXT("Object"),
					  AssetPath);
		}
	}

	for (TFieldIterator<FProperty> PropIt(mTargetClass, EFieldIteratorFlags::IncludeSuper); PropIt; ++PropIt)
	{
		FProperty* Property = *PropIt;

		const FKBFLCDOOverwriteProperty* MergedProp =
			MergedProperties.Find(FKBFLCDOOverwriteProperty(Property->GetFName()));
		if (!MergedProp)
		{
			continue;
		}

		if (MergedProp->bSkipThisField)
		{
			continue;
		}

		void* ContainerValuePtr = Property->ContainerPtrToValuePtr<void>(mPropertyContainer);
		void* DestValuePtr = Property->ContainerPtrToValuePtr<void>(TargetInstance);

		ApplyPropertyWithBehavior(Property, ContainerValuePtr, DestValuePtr, *MergedProp, AssetPath, TargetInstance);
	}

	mAppliedInstances.Add(TargetInstance);

	if (mManuelPropertiesOverwrite.Contains(FKBFLCDOOverwriteProperty(TEXT("mDisplayName"))) ||
		mModifiedProperties.Contains(FKBFLCDOOverwriteProperty(TEXT("mDisplayName"))))
	{
		FKBFLCDOOverwriteProperty* DisplayNameProp =
			MergedProperties.Find(FKBFLCDOOverwriteProperty(TEXT("mDisplayName")));
		if (DisplayNameProp && DisplayNameProp->bSkipThisField)
		{
			return;
		}
		if (!DisplayNameProp)
		{
			DisplayNameProp = mModifiedProperties.Find(FKBFLCDOOverwriteProperty(TEXT("mDisplayName")));
			if (DisplayNameProp && DisplayNameProp->bSkipThisField)
			{
				return;
			}
		}

		if (UFGRecipe* AsRecipe = Cast<UFGRecipe>(TargetInstance))
		{
			FText DisplayName = AsRecipe->mDisplayName;
			if (DisplayName.IsEmptyOrWhitespace())
			{
				AsRecipe->mDisplayNameOverride = false;
				UE_LOGFMT(LogKBFLCDOOverwrite, Log,
						  "ApplyToInstance: Recipe '{TargetName}' has empty display name, "
						  "resetting override | Asset: {AssetPath}",
						  TargetName, AssetPath);
			}
		}
	}
}

void UKBFLCDOOverwrite::ApplyToInstances()
{
	if (UFGRecipe* AsRecipe = Cast<UFGRecipe>(mPropertyContainer))
	{
		if (!AsRecipe->mDisplayNameOverride)
		{
			AsRecipe->mDisplayName = FText::GetEmpty();
			UE_LOGFMT(LogKBFLCDOOverwrite, Log,
					  "ApplyToInstances: Property container recipe has no display "
					  "name override, clearing display name | Asset: {AssetPath}",
					  GetPathName());

			FKBFLCDOOverwriteProperty* DisplayNameProp =
				mManuelPropertiesOverwrite.Find(FKBFLCDOOverwriteProperty(TEXT("mDisplayName")));
			if (!DisplayNameProp)
			{
				mManuelPropertiesOverwrite.Add(FKBFLCDOOverwriteProperty(TEXT("mDisplayName")));
			}
		}
	}

	const FString AssetPath = GetPathName();

	TSet<TSubclassOf<UObject>> ClassesToProcess;
	if (!CollectClassesToProcess(ClassesToProcess))
	{
		return;
	}

	int32 TotalProcessedClasses = 0;

	const TArray<TSubclassOf<UObject>> ClassesToIgnore = LoadSoftClassesArray(mIgnoreClasses);

	for (const TSubclassOf<UObject>& SubClass : ClassesToProcess)
	{

		if (OnlyApplyOnSubclasses() && SubClass == mTargetClass)
		{
			UE_LOGFMT(LogKBFLCDOOverwrite, Warning,
					  "ApplyToInstances: Skipping target class '{0}' "
					  "(OnlyApplyOnSubclasses()=true) | Asset: {AssetPath}",
					  *SubClass->GetName(), AssetPath);
			continue;
		}

		if (bOnlyApplyOnBlueprints && SubClass && !SubClass->HasAnyClassFlags(CLASS_CompiledFromBlueprint))
		{
			UE_LOGFMT(LogKBFLCDOOverwrite, Warning,
					  "ApplyToInstances: Skipping native class '{0}' "
					  "(bOnlyApplyOnBlueprints=true) | Asset: {AssetPath}",
					  *SubClass->GetName(), AssetPath);
			continue;
		}

		if (ClassesToIgnore.Contains(SubClass))
		{
			UE_LOGFMT(LogKBFLCDOOverwrite, Warning,
					  "ApplyToInstances: Skipping ignored class '{0}' | Asset: {AssetPath}",
					  SubClass ? *SubClass->GetName() : TEXT("Invalid"), AssetPath);
			continue;
		}

		TSubclassOf<UObject> LoadedNativeClass = mNativeClass;
		if (SubClass && LoadedNativeClass && SubClass->IsChildOf(LoadedNativeClass))
		{
			if (UObject* SubClassCDO = mSubsystem->GetAndStoreDefaultObject_Native<UObject>(SubClass))
			{
				Requirements_NotifyOnModify(SubClassCDO);
				ApplyToInstance(SubClassCDO);
				Requirements_NotifyOnModified(SubClassCDO);
				TotalProcessedClasses++;
			}
		}
	}

	if (TotalProcessedClasses == 0)
	{
		UE_LOGFMT(LogKBFLCDOOverwrite, Warning,
				  "ApplyToInstances: No classes were processed. Please check your "
				  "filters and target class. | Asset: {AssetPath}",
				  AssetPath);
	}
	else
	{
		UE_LOGFMT(LogKBFLCDOOverwrite, Log,
				  "ApplyToInstances: Successfully applied to {0} class CDOs | "
				  "Asset: {AssetPath}",
				  TotalProcessedClasses, AssetPath);
	}
}

bool UKBFLCDOOverwrite::CollectClassesToProcess(TSet<TSubclassOf<UObject>>& ClassesToProcess)
{
	const FString AssetPath = GetPathName();

	TSubclassOf<UObject> EffectiveTargetClass = bTargetOnlyAsContainer ? mRealTargetClass : mTargetClass;

	if (!bTargetOnlyAsContainer && !EffectiveTargetClass)
	{
		UE_LOGFMT(LogKBFLCDOOverwrite, Error,
				  "CollectClassesToProcess: No valid target class (mTargetClass is null) "
				  "| Asset: {AssetPath}",
				  AssetPath);
		return false;
	}

	if (!OnlyApplyOnSubclasses() && EffectiveTargetClass)
	{
		ClassesToProcess.Add(EffectiveTargetClass);
	}

	if (bTargetOnlyAsContainer && mRealTargetClass)
	{
		ClassesToProcess.Add(mRealTargetClass);
	}

	ClassesToProcess.Append(LoadSoftClassesArray(mAlsoApplyOn));

	if (bApplyOnSubclasses && EffectiveTargetClass)
	{
		TArray<UClass*> AddionalSubClasses;
		AddionalSubClasses.Append(LoadSoftClassesArray(mAddionalSubClasses));
		UKBFLAssetDataSubsystem* AssetSubsystem =
			mSubsystem->GetGameInstance()->GetSubsystem<UKBFLAssetDataSubsystem>();

		if (IsValid(EffectiveTargetClass) && UseTargetAsSubclassFilter())
		{
			AddionalSubClasses.Add(EffectiveTargetClass);
		}

		if (UseNativeForSubclasses() && IsValid(mNativeClass))
		{
			AddionalSubClasses.Add(mNativeClass);
		}

		TArray<TSubclassOf<UObject>> Out;
		AssetSubsystem->GetObjectsOfChilds(AddionalSubClasses, Out, true);
		AssetSubsystem->GetObjectsOfChilds(AddionalSubClasses, Out, false);
		ClassesToProcess.Append(Out);
	}

	if (mFindAssetsInPaths.Num() > 0 && IsValid(mNativeClass))
	{
		UKBFLAssetDataSubsystem* AssetSubsystem =
			mSubsystem->GetGameInstance()->GetSubsystem<UKBFLAssetDataSubsystem>();

		if (AssetSubsystem)
		{
			TSubclassOf<UObject> LoadedNativeClass = mNativeClass;
			if (LoadedNativeClass)
			{
				TArray<TSubclassOf<UObject>> FoundClasses;
				TArray<UClass*> NativeClassArray = {LoadedNativeClass};
				AssetSubsystem->GetObjectsOfChilds(NativeClassArray, FoundClasses, true);
				AssetSubsystem->GetObjectsOfChilds(NativeClassArray, FoundClasses, false);

				for (const TSubclassOf<UObject>& FoundClass : FoundClasses)
				{
					if (!FoundClass)
					{
						continue;
					}

					FString ClassPath = FoundClass->GetPathName();
					for (const FString& SearchPath : mFindAssetsInPaths)
					{
						if (ClassPath.StartsWith(SearchPath))
						{
							ClassesToProcess.Add(FoundClass);
							break;
						}
					}
				}
			}
		}
	}

	ClassesToProcess.Append(LoadSoftClassesArray(mOtherTargetClasses));

	return true;
}

bool UKBFLCDOOverwrite::ShouldCallForInstance(UClass* NewClass)
{
	if (!NewClass)
	{
		return false;
	}

	if (bOnlyApplyOnBlueprints && !NewClass->HasAnyClassFlags(CLASS_CompiledFromBlueprint))
	{
		return false;
	}

	TSubclassOf<UObject> LoadedNativeClass = mNativeClass;
	if (!LoadedNativeClass || !NewClass->IsChildOf(LoadedNativeClass))
	{
		return false;
	}

	for (const TSoftClassPtr<UObject>& Ignore : mIgnoreClasses)
	{
		if (Ignore.Get() == NewClass)
		{
			return false;
		}
	}

	const TSubclassOf<UObject> EffectiveTargetClass = bTargetOnlyAsContainer ? mRealTargetClass : mTargetClass;

	if (OnlyApplyOnSubclasses() && NewClass == mTargetClass)
	{
		return false;
	}

	if (!OnlyApplyOnSubclasses() && EffectiveTargetClass && NewClass == EffectiveTargetClass)
	{
		return true;
	}
	if (bTargetOnlyAsContainer && mRealTargetClass && NewClass == mRealTargetClass)
	{
		return true;
	}
	for (const TSoftClassPtr<UObject>& Also : mAlsoApplyOn)
	{
		if (Also.Get() == NewClass)
		{
			return true;
		}
	}
	for (const TSoftClassPtr<UObject>& Other : mOtherTargetClasses)
	{
		if (Other.Get() == NewClass)
		{
			return true;
		}
	}

	if (bApplyOnSubclasses && EffectiveTargetClass)
	{
		if (UseTargetAsSubclassFilter() && NewClass->IsChildOf(EffectiveTargetClass))
		{
			return true;
		}
		if (UseNativeForSubclasses() && NewClass->IsChildOf(LoadedNativeClass))
		{
			return true;
		}
		for (const TSoftClassPtr<UObject>& AddSub : mAddionalSubClasses)
		{
			if (UClass* AddSubClass = AddSub.Get())
			{
				if (NewClass->IsChildOf(AddSubClass))
				{
					return true;
				}
			}
		}
	}

	if (mFindAssetsInPaths.Num() > 0)
	{
		const FString ClassPath = NewClass->GetPathName();
		for (const FString& SearchPath : mFindAssetsInPaths)
		{
			if (ClassPath.StartsWith(SearchPath))
			{
				return true;
			}
		}
	}

	return false;
}
