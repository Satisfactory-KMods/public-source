// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "Resources/FGItemDescriptor.h"

#include "KPCLNetworkDrive.generated.h"

UCLASS()
class KPRIVATECODELIB_API UKPCLNetworkDrive : public UFGItemDescriptor
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Network")
	static int32 GetMultiplier(TSubclassOf<UKPCLNetworkDrive> InClass);

	UFUNCTION(BlueprintPure, Category = "Network")
	static float GetPowerConsume(TSubclassOf<UKPCLNetworkDrive> InClass);

	UFUNCTION(BlueprintPure, Category = "Network")
	static int32 GetBuildingLimit(TSubclassOf<UKPCLNetworkDrive> InClass);

	UFUNCTION(BlueprintPure, Category = "Network")
	static int32 GetUniqueItemLimit(TSubclassOf<UKPCLNetworkDrive> InClass);

	virtual FText GetItemDescriptionInternal() const override;
	virtual FText GetItemNameInternal() const override;

	UFUNCTION(BlueprintNativeEvent)
	FText GetItemDescriptionInternal_BP() const;

	UFUNCTION(BlueprintNativeEvent)
	FText GetItemNameInternal_BP() const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Network")
	int32 mFaxitStorageMultiplier = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Network")
	int32 mPowerConsume = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Network")
	int32 mBuildingLimit = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Network")
	int32 mUniqueItemLimit = 0.f;
};
