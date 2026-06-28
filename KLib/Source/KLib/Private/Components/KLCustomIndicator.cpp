#include "Components/KLCustomIndicator.h"

#include "FGBuildableSubsystem.h"
#include "FGColorInterface.h"


UKLCustomIndicator::UKLCustomIndicator() { PrimaryComponentTick.bCanEverTick = false; }

void UKLCustomIndicator::BeginPlay() { Super::BeginPlay(); }

void UKLCustomIndicator::UpdateColors(const TArray<FLinearColor>& ColorArray)
{
	// UE_LOG(LogTemp, Warning, TEXT("UpdateColors"));
	mColorArray = ColorArray;
}

void UKLCustomIndicator::UpdateColorIndex(int NewIndex)
{
	if (NewIndex == mMaterialIndex)
	{
		return;
	}
	mMaterialIndex = NewIndex;

	if (!mColorArray.IsValidIndex(mMaterialIndex))
	{
		return;
	}

	// UE_LOG(LogTemp, Warning, TEXT("UpdateColorIndex, %d"), NewIndex);
	if (mInstanceHandle.IsInstanced())
	{
		FFactoryCustomizationData Data = IFGColorInterface::Execute_GetCustomizationData(GetOwner());
		Data.Data[16] = mMaterialIndex != 0;
		Data.Data[17] = mColorArray[mMaterialIndex].R;
		Data.Data[18] = mColorArray[mMaterialIndex].G;
		Data.Data[19] = mColorArray[mMaterialIndex].B;
		IFGColorInterface::Execute_SetCustomizationData(GetOwner(), Data);
		/*
		mActiveColorIndex = NewIndex;
		mInstanceHandle.SetCustomDataById(16, NewIndex > 0 ? 1.0f : 0.0f);
		mInstanceHandle.SetCustomDataById(17, mColorArray[NewIndex].R);
		mInstanceHandle.SetCustomDataById(18, mColorArray[NewIndex].G);
		mInstanceHandle.SetCustomDataById(19, mColorArray[NewIndex].B);

		AFGBuildableSubsystem* Subsystem = AFGBuildableSubsystem::Get(this);
		if(Subsystem)
			if(Subsystem->GetColoredInstanceManager(this))
				Subsystem->GetColoredInstanceManager(this)->UpdateColorForInstanceFromDataArray(mInstanceHandle);
				*/
	}

	SetCustomPrimitiveDataFloat(16, mMaterialIndex != 0);
	SetCustomPrimitiveDataFloat(17, mColorArray[mMaterialIndex].R);
	SetCustomPrimitiveDataFloat(18, mColorArray[mMaterialIndex].G);
	SetCustomPrimitiveDataFloat(19, mColorArray[mMaterialIndex].B);
}

void UKLCustomIndicator::onUpdatedColorIndex_Implementation(int NewIndex)
{
	if (NewIndex == mMaterialIndex)
	{
		return;
	}
	mMaterialIndex = NewIndex;

	if (!mColorArray.IsValidIndex(mMaterialIndex))
	{
		return;
	}

	// UE_LOG(LogTemp, Warning, TEXT("onUpdatedColorIndex_Implementation, %d"), NewIndex);
	if (mInstanceHandle.IsInstanced())
	{
		FFactoryCustomizationData Data = IFGColorInterface::Execute_GetCustomizationData(GetOwner());
		Data.Data[16] = mMaterialIndex != 0;
		Data.Data[17] = mColorArray[mMaterialIndex].R;
		Data.Data[18] = mColorArray[mMaterialIndex].G;
		Data.Data[19] = mColorArray[mMaterialIndex].B;
		IFGColorInterface::Execute_SetCustomizationData(GetOwner(), Data);
		mActiveColorIndex = NewIndex;
		/*
		mInstanceHandle.SetCustomDataById(16, NewIndex > 0 ? 1.0f : 0.0f);
		mInstanceHandle.SetCustomDataById(17, mColorArray[NewIndex].R);
		mInstanceHandle.SetCustomDataById(18, mColorArray[NewIndex].G);
		mInstanceHandle.SetCustomDataById(19, mColorArray[NewIndex].B);

		AFGBuildableSubsystem* Subsystem = AFGBuildableSubsystem::Get(this);
		if(Subsystem)
			if(Subsystem->GetColoredInstanceManager(this))
				Subsystem->GetColoredInstanceManager(this)->UpdateColorForInstanceFromDataArray(mInstanceHandle);*/
	}

	SetCustomPrimitiveDataFloat(16, mMaterialIndex != 0);
	SetCustomPrimitiveDataFloat(17, mColorArray[mMaterialIndex].R);
	SetCustomPrimitiveDataFloat(18, mColorArray[mMaterialIndex].G);
	SetCustomPrimitiveDataFloat(19, mColorArray[mMaterialIndex].B);
}
