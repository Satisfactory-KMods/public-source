// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "DataAssets/KAPIDataAssetBase.h"
#include "Engine/Texture2D.h"
#include "FGRecipe.h"

#include "KAPIManufacturerModifications.generated.h"

class UFGInventoryComponent;
class AFGBuildableManufacturer;

USTRUCT(Blueprintable, BlueprintType)
struct FKAPIManufacturerModificationData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<int32, TObjectPtr<UTexture2D>> mFluidSlotIcons;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<int32, TObjectPtr<UTexture2D>> mSolidSlotIcons;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float mSolidMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float mFluidMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float mGasMultiplier = 1.f;
};

UCLASS(BlueprintType)
class KAPI_API UKAPIManufacturerModifications : public UKAPIDataAssetBase
{
	GENERATED_BODY()

public:
	bool ApplyInventoryChanges(AFGBuildableManufacturer* Manufacturer, TSubclassOf<class UFGRecipe> recipe) const;

	UFUNCTION(BlueprintCallable, Category = "Mods|FactoryGame|Inventory")
	UTexture2D* GetInputSlotIcon(AFGBuildableManufacturer* Manufacturer, TSubclassOf<class UFGRecipe> recipe,
								 int32 SlotIndex, bool IsSolid = true) const;

	UFUNCTION(BlueprintCallable, Category = "Mods|FactoryGame|Inventory")
	UTexture2D* GetOutputSlotIcon(AFGBuildableManufacturer* Manufacturer, TSubclassOf<class UFGRecipe> recipe,
								  int32 SlotIndex, bool IsSolid = true) const;

	UFUNCTION(BlueprintPure)
	static bool IsSolidItem(TSubclassOf<UFGItemDescriptor> Item);

	UFUNCTION(BlueprintCallable, Category = "Mods|FactoryGame|Inventory")
	bool MatchManufacturer(AFGBuildableManufacturer* Manufacturer) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target")
	TSubclassOf<AFGBuildableManufacturer> mTargetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target")
	bool bApplyOnSubclasses = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifications")
	bool bAllowMoreInputSlots = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifications")
	FKAPIManufacturerModificationData mOutput;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifications")
	FKAPIManufacturerModificationData mInput;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bDebug = false;

private:
	void ApplyInputSlots(AFGBuildableManufacturer* Manufacturer, TSubclassOf<class UFGRecipe> recipe) const;
	void ApplyMoreInputSlots(AFGBuildableManufacturer* Manufacturer, TSubclassOf<class UFGRecipe> recipe) const;
	void ApplyOutputSlots(AFGBuildableManufacturer* Manufacturer, TSubclassOf<class UFGRecipe> recipe) const;

	static TSubclassOf<UFGItemDescriptor> GetAllowedItemForInputSlot(const TSubclassOf<UFGRecipe>& Recipe,
																	 UFGInventoryComponent* InventoryComponent,
																	 int32 SlotIndex, bool Product = false);
	static int32 GetSlotSize(const TSubclassOf<UFGItemDescriptor>& InItem,
							 const FKAPIManufacturerModificationData& Data, int32 DefaultMultiplier);
};
