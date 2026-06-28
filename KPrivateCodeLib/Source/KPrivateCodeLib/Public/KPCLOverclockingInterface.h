// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "BFL/KBFL_Math.h"
#include "Buildables/FGBuildableFactory.h"

#include "KPCLOverclockingInterface.generated.h"

UINTERFACE()
class UKPCLOverclockingInterface : public UInterface
{
	GENERATED_BODY()
};

USTRUCT(BlueprintType)
struct FKPCLOverclockingProductionInfo
{
	GENERATED_BODY()

	FKPCLOverclockingProductionInfo() : mAmount(0), mDefaultProductionTime(0) {}

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UFGItemDescriptor> mItemClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 mAmount;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float mDefaultProductionTime;
};

USTRUCT(BlueprintType)
struct FKPCLOverclockingProductionResults
{
	GENERATED_BODY()

	FKPCLOverclockingProductionResults() : mAmount(0), mProductionTime(0) {}

	FKPCLOverclockingProductionResults(FItemAmount InAmount, float InProductionTime)
	{
		mItemClass = InAmount.ItemClass;
		mAmount = InAmount.Amount;
		mProductionTime = InProductionTime;
		bIsSolid = UFGItemDescriptor::GetForm(InAmount.ItemClass) == EResourceForm::RF_SOLID;
	}

	FKPCLOverclockingProductionResults(TSubclassOf<UFGItemDescriptor> InItemClass, int32 InAmount,
									   float InProductionTime)
	{
		mItemClass = InItemClass;
		mAmount = InAmount;
		mProductionTime = InProductionTime;
		bIsSolid = UFGItemDescriptor::GetForm(InItemClass) == EResourceForm::RF_SOLID;
	}

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UFGItemDescriptor> mItemClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 mAmount;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float mProductionTime;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bIsSolid = false;
};

class KPRIVATECODELIB_API IKPCLOverclockingInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "KMods|Overclocking")
	bool Overclocking_ShouldUse();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "KMods|Overclocking")
	bool Overclocking_UseInventory(int32& UnlockedSlots);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "KMods|Overclocking")
	bool Overclocking_IsConsumer();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "KMods|Overclocking")
	UFGInventoryComponent* Overclocking_GetInventory();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "KMods|Overclocking")
	AFGBuildableFactory* Overclocking_GetFactory();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "KMods|Overclocking")
	void Overclocking_GetInfo(FKPCLOverclockingProductionInfo& OutProductionInfo);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "KMods|Overclocking")
	void Overclocking_GetCostSlots(TArray<FItemAmount>& OutSlots);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "KMods|Overclocking")
	void Overclocking_GetProductionResults(TArray<FKPCLOverclockingProductionResults>& OutIngredients,
										   TArray<FKPCLOverclockingProductionResults>& OutProducts);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "KMods|Overclocking")
	void UI_ApplyRelevantItems(TArray<TSubclassOf<UFGItemDescriptor>>& OutSlots);
};
