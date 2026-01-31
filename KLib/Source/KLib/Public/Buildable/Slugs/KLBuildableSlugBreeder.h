// ILikeBanas

#pragma once

#include "CoreMinimal.h"
#include "KLBuildableSlugBuildingBase.h"
#include "KLBuildableSlugHatchingModule.h"

#include "KLBuildableSlugBreeder.generated.h"

UCLASS()
class KLIB_API UKLSlugBreederClipboardSettings : public UKPCLBaseClipboardSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite)
	float mTemperature;

	UPROPERTY(BlueprintReadWrite)
	float mHumidity;
};

UENUM(BlueprintType)
enum class ESlugSlot : uint8
{
	Slot1 = 0 UMETA(DisplayName = "Slot1"),
	Slot2 = 1 UMETA(DisplayName = "Slot2"),
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBreedingInformationHasChanged);

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class KLIB_API AKLBuildableSlugBreeder : public AKLBuildableSlugBuildingBase
{
	GENERATED_BODY()

public:
	AKLBuildableSlugBreeder();

	virtual void UI_ApplyRelevantItems_Implementation(TArray<TSubclassOf<UFGItemDescriptor>>& OutSlots) override;
	virtual void Overclocking_GetInfo_Implementation(FKPCLOverclockingProductionInfo& OutProductionInfo) override;
	virtual void Overclocking_GetProductionResults_Implementation(
		TArray<FKPCLOverclockingProductionResults>& OutIngredients,
		TArray<FKPCLOverclockingProductionResults>& OutProducts) override;


	// Repl
	virtual void GetConditionalReplicatedProps(TArray<FFGCondReplicatedProperty>& outProps) const override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual bool CanUseFactoryClipboard_Implementation() override;
	virtual UFGFactoryClipboardSettings* CopySettings_Implementation() override;
	virtual bool PasteSettings_Implementation(UFGFactoryClipboardSettings* factoryClipboard,
	                                          class AFGPlayerController* player) override;

	virtual void SetPendingPotential(float newPendingPotential) override;
	virtual void BeginPlay() override;
	virtual void Factory_Tick(float dt) override;
	virtual void CollectBelts() override;

	virtual bool CanProduce_Implementation() const override;

	UFUNCTION(BlueprintNativeEvent, Category="KMods|Breeder")
	void OnSlugInformationHasChanged();

	UFUNCTION(BlueprintPure, BlueprintCallable, Category="KMods|Breeder")
	bool SlugFeeling() const;
	bool CanConsume() const;
	bool CanStorage() const;
	void CheckSlugs();

	UFUNCTION(BlueprintNativeEvent, Category="KMods|Breeder")
	void OnFoodFinial();

	UFUNCTION(BlueprintNativeEvent, Category="KMods|Breeder")
	void OnSlugDied(uint8 slot);

	virtual void onProducingFinal_Implementation() override;
	virtual void EndProductionTime() override;
	void ApplySlugData();

	// Native Helper
	FInventoryStack GetFoodStack() const;

	UFUNCTION(BlueprintPure, BlueprintCallable, Category="KMods|Breeder")
	bool IsFoodCorrect() const;

	UFUNCTION(BlueprintPure, BlueprintCallable, Category="KMods|Breeder")
	int32 GetNumOfFood() const;

	/** Works on Client and Host */
	UFUNCTION(BlueprintCallable, Category="KMods|Breeder")
	void ApplyNewHumidity(float Humidity);

	/** Works on Client and Host */
	UFUNCTION(BlueprintCallable, Category="KMods|Breeder")
	void ApplyNewTemperature(float Temperature);

	UFUNCTION(BlueprintPure, BlueprintCallable, Category="KMods|Breeder")
	int32 GetNumOfFoodConsume() const;

	UFUNCTION(BlueprintPure, BlueprintCallable, Category="KMods|Breeder")
	UKAPISugHatchingData* GetDataForSlot(ESlugSlot Slot) const;

	UFUNCTION(BlueprintPure, BlueprintCallable, Category="KMods|Breeder")
	UKAPISugHatchingData* GetActiveData() const;

	UFUNCTION(BlueprintPure, BlueprintCallable, Category="KMods|Breeder")
	TSubclassOf<UFGItemDescriptor> GetCurrentFood() const;

	UFUNCTION(BlueprintPure, BlueprintCallable, Category="KMods|Breeder")
	TSubclassOf<UFGItemDescriptor> GetStoredFoodClass() const;

	UFUNCTION(BlueprintPure, BlueprintCallable, Category="KMods|Breeder")
	bool AreSlugsComfortable() const;

	UFUNCTION(BlueprintPure, Category="KMods|Breeder")
	FFullProductionHandle GetDeadTimer(ESlugSlot Slot) const;

	// Filter Inventorys to be Slug Classes!
	virtual bool FilterInputInventory(TSubclassOf<UObject> object, int32 idx) const override;

	UPROPERTY(Transient)
	TSet<TSubclassOf<UFGItemDescriptor>> mPossibleSlugs;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="KMods|Breeder")
	UKAPISugHatchingData* mDefaultHatchingData;

	UPROPERTY(SaveGame, meta = ( FGReplicated ))
	UKAPISugHatchingData* mCurrentHatchingData;

	UPROPERTY(BlueprintReadOnly, SaveGame, meta = ( FGReplicated ))
	FFullProductionHandle mFoodProductionHandle;

	UPROPERTY(EditDefaultsOnly, SaveGame, meta = ( FGReplicated ), Category="KMods|Breeder")
	TArray<UKAPISugHatchingData*> mActiveHatchingDatas = {
		nullptr, nullptr
	};

	UPROPERTY(SaveGame, meta = ( FGReplicated ))
	TArray<FFullProductionHandle> mDeadTimer = {FFullProductionHandle(60.f), FFullProductionHandle(60.f)};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="KMods|Breeder")
	int32 mMaxSlugPerSlot = 5;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="KMods|Breeder")
	int32 mMaxFoodStack = 50;

	UPROPERTY(EditDefaultsOnly, SaveGame, BlueprintReadOnly, meta = ( FGReplicated ), Category="KMods|Breeder")
	FKLHatchingModeleStats mTemperature;

	UPROPERTY(EditDefaultsOnly, SaveGame, BlueprintReadOnly, meta = ( FGReplicated ), Category="KMods|Breeder")
	FKLHatchingModeleStats mHumidity;

	UPROPERTY(BlueprintAssignable, Category="KMods|Events")
	FOnBreedingInformationHasChanged OnBreedingInformationHasChanged;


	UPROPERTY(Transient)
	UKAPIDataAssetSubsystem* mDataSubsystem;

	int32 FIRST_SLUG_IDX = 0;
	int32 SECOND_SLUG_IDX = 1;
	int32 FOOD_IDX = 2;
};
