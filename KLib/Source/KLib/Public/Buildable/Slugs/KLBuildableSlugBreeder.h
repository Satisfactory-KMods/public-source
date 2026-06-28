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

UCLASS(Blueprintable, BlueprintType)
class KLIB_API AKLBuildableSlugBreeder : public AKLBuildableSlugBuildingBase
{
	GENERATED_BODY()

public:
	AKLBuildableSlugBreeder();

	//~ Begin AFGBuildableFactory Interface
	virtual bool CanProduce_Implementation() const override;
	virtual bool CanUseFactoryClipboard_Implementation() override;
	virtual void CollectBelts() override;
	virtual void Factory_TickAuthOnly(float dt) override;
	virtual void GetConditionalReplicatedProps(TArray<FFGCondReplicatedProperty>& outProps) const override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void onProducingFinal_Implementation() override;
	virtual UFGFactoryClipboardSettings* CopySettings_Implementation() override;
	virtual bool FilterInputInventory(TSubclassOf<UObject> object, int32 idx) const override;
	virtual bool PasteSettings_Implementation(UFGFactoryClipboardSettings* factoryClipboard,
											  class AFGPlayerController* player) override;
	//~ End AFGBuildableFactory Interface

	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	//~ End AActor Interface

	//~ Begin AKPCLProducerBase Interface
	virtual void Overclocking_GetInfo_Implementation(FKPCLOverclockingProductionInfo& OutProductionInfo) override;
	virtual void
	Overclocking_GetProductionResults_Implementation(TArray<FKPCLOverclockingProductionResults>& OutIngredients,
													 TArray<FKPCLOverclockingProductionResults>& OutProducts) override;
	virtual void SetPendingPotential(float newPendingPotential) override;
	virtual void UI_ApplyRelevantItems_Implementation(TArray<TSubclassOf<UFGItemDescriptor>>& OutSlots) override;
	//~ End AKPCLProducerBase Interface

	//~ Begin AKLBuildableSlugBuildingBase Interface
	virtual void EndProductionTime() override;
	//~ End AKLBuildableSlugBuildingBase Interface

	UFUNCTION(BlueprintNativeEvent, Category = "KMods|Breeder")
	void OnFoodFinial();

	UFUNCTION(BlueprintNativeEvent, Category = "KMods|Breeder")
	void OnSlugDied(uint8 slot);

	UFUNCTION(BlueprintNativeEvent, Category = "KMods|Breeder")
	void OnSlugInformationHasChanged();

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "KMods|Breeder")
	bool AreSlugsComfortable() const;

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "KMods|Breeder")
	int32 GetNumOfFood() const;

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "KMods|Breeder")
	int32 GetNumOfFoodConsume() const;

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "KMods|Breeder")
	bool IsFoodCorrect() const;

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "KMods|Breeder")
	bool SlugFeeling() const;

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "KMods|Breeder")
	FFullProductionHandle GetDeadTimer(ESlugSlot Slot) const;

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "KMods|Breeder")
	TSubclassOf<UFGItemDescriptor> GetCurrentFood() const;

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "KMods|Breeder")
	TSubclassOf<UFGItemDescriptor> GetStoredFoodClass() const;

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "KMods|Breeder")
	UKAPISugHatchingData* GetActiveData() const;

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "KMods|Breeder")
	UKAPISugHatchingData* GetDataForSlot(ESlugSlot Slot) const;

	UFUNCTION(BlueprintCallable, Category = "KMods|Breeder")
	void ApplyNewHumidity(float Humidity);

	UFUNCTION(BlueprintCallable, Category = "KMods|Breeder")
	void ApplyNewTemperature(float Temperature);

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "KMods|Breeder")
	FFullProductionHandle GetFoodProductionHandle() const;

	void ApplySlugData();
	bool CanConsume() const;
	bool CanStorage() const;
	void CheckSlugs();
	FInventoryStack GetFoodStack() const;

	void SetActiveHatchingData(int32 SlotIdx, UKAPISugHatchingData* NewData);
	void SetCurrentHatchingData(UKAPISugHatchingData* NewData);
	void CommitFoodProductionHandle();
	void CommitDeadTimers();
	void CommitTemperature();
	void CommitHumidity();

	static constexpr int32 FIRST_SLUG_IDX = 0;
	static constexpr int32 SECOND_SLUG_IDX = 1;
	static constexpr int32 FOOD_IDX = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|Breeder")
	int32 mMaxFoodStack = 50;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|Breeder")
	int32 mMaxSlugPerSlot = 5;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|Breeder")
	TObjectPtr<UKAPISugHatchingData> mDefaultHatchingData;

	UPROPERTY(EditDefaultsOnly, SaveGame, BlueprintReadOnly, meta = (FGReplicated), Category = "KMods|Breeder")
	FKLHatchingModeleStats mHumidity;

	UPROPERTY(EditDefaultsOnly, SaveGame, BlueprintReadOnly, meta = (FGReplicated), Category = "KMods|Breeder")
	FKLHatchingModeleStats mTemperature;

	UPROPERTY(EditDefaultsOnly, SaveGame, meta = (FGReplicated), Category = "KMods|Breeder")
	TArray<TObjectPtr<UKAPISugHatchingData>> mActiveHatchingDatas;

	UPROPERTY(SaveGame, meta = (FGReplicated))
	FFullProductionHandle mFoodProductionHandle;

	UPROPERTY(SaveGame, meta = (FGReplicated))
	TArray<FFullProductionHandle> mDeadTimer;

	UPROPERTY(SaveGame, meta = (FGReplicated))
	TObjectPtr<UKAPISugHatchingData> mCurrentHatchingData;

	UPROPERTY(Transient)
	TObjectPtr<UKAPIDataAssetSubsystem> mDataSubsystem;

	UPROPERTY(Transient)
	TSet<TSubclassOf<UFGItemDescriptor>> mPossibleSlugs;

	UPROPERTY(BlueprintAssignable, Category = "KMods|Events")
	FOnBreedingInformationHasChanged OnBreedingInformationHasChanged;
};
