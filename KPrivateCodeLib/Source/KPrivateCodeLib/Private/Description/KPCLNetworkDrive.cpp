// Copyright Coffee Stain Studios. All Rights Reserved.

#include "Description/KPCLNetworkDrive.h"

int32 UKPCLNetworkDrive::GetMultiplier(TSubclassOf<UKPCLNetworkDrive> InClass)
{
	if (IsValid(InClass))
	{
		return InClass.GetDefaultObject()->mFaxitStorageMultiplier;
	}
	return 0;
}

float UKPCLNetworkDrive::GetPowerConsume(TSubclassOf<UKPCLNetworkDrive> InClass)
{
	if (IsValid(InClass))
	{
		return InClass.GetDefaultObject()->mPowerConsume;
	}
	return 0.f;
}

int32 UKPCLNetworkDrive::GetBuildingLimit(TSubclassOf<UKPCLNetworkDrive> InClass)
{
	if (IsValid(InClass))
	{
		return InClass.GetDefaultObject()->mBuildingLimit;
	}
	return 0;
}

int32 UKPCLNetworkDrive::GetUniqueItemLimit(TSubclassOf<UKPCLNetworkDrive> InClass)
{
	if (IsValid(InClass))
	{
		return InClass.GetDefaultObject()->mUniqueItemLimit;
	}
	return 0;
}

FText UKPCLNetworkDrive::GetItemDescriptionInternal() const
{
	FText MainTxt = GetItemDescriptionInternal_BP();

	FFormatNamedArguments FormatPatternArgs;
	FormatPatternArgs.Empty();
	FormatPatternArgs.Add(TEXT("Multiplier"), FText::FromString(FString::FromInt(mFaxitStorageMultiplier)));
	FormatPatternArgs.Add(TEXT("StackMultiplier"), FText::FromString(FString::FromInt(mFaxitStorageMultiplier)));
	FormatPatternArgs.Add(TEXT("Building"), FText::FromString(FString::FromInt(mBuildingLimit)));
	FormatPatternArgs.Add(TEXT("UniqueItems"), FText::FromString(FString::FromInt(mUniqueItemLimit)));
	FormatPatternArgs.Add(TEXT("PowerConsume"), FText::FromString(FString::FromInt(mPowerConsume)));
	return FText::Format(MainTxt, FormatPatternArgs);
}

FText UKPCLNetworkDrive::GetItemNameInternal() const
{
	FText MainTxt = GetItemNameInternal_BP();

	FFormatNamedArguments FormatPatternArgs;
	FormatPatternArgs.Empty();
	FormatPatternArgs.Add(TEXT("Multiplier"), FText::FromString(FString::FromInt(mFaxitStorageMultiplier)));
	FormatPatternArgs.Add(TEXT("StackMultiplier"), FText::FromString(FString::FromInt(mFaxitStorageMultiplier)));
	FormatPatternArgs.Add(TEXT("Building"), FText::FromString(FString::FromInt(mBuildingLimit)));
	FormatPatternArgs.Add(TEXT("UniqueItems"), FText::FromString(FString::FromInt(mUniqueItemLimit)));
	FormatPatternArgs.Add(TEXT("PowerConsume"), FText::FromString(FString::FromInt(mPowerConsume)));
	return FText::Format(MainTxt, FormatPatternArgs);
}

FText UKPCLNetworkDrive::GetItemNameInternal_BP_Implementation() const { return mDisplayName; }

FText UKPCLNetworkDrive::GetItemDescriptionInternal_BP_Implementation() const { return mDescription; }
