// ILikeBanas

#pragma once

#include "CoreMinimal.h"
#include "Buildable/KPCLProducerBase.h"

#include "Cpp/KBFLCppInventoryHelper.h"
#include "Subsystem/KLUnlockSubsystem.h"

#include "KlBuildableCleaner.generated.h"


UCLASS()
class KLIB_API UKLCleanerClipboardSettings : public UKPCLBaseClipboardSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite)
	UKAPICleanerItemDescription* mCurrentCleanerRecipe;
};


/**
 * 
 */
UCLASS()
class KLIB_API AKLBuildableCleaner : public AKPCLProducerBase
{
	GENERATED_BODY()

public:
	AKLBuildableCleaner();

	virtual void Overclocking_GetInfo_Implementation(FKPCLOverclockingProductionInfo& OutProductionInfo) override;

	/**
	 * ---------------- Start Recipe System ----------------
	*/
	virtual bool Overclocking_IsConsumer_Implementation() override;
	virtual void UI_ApplyRelevantItems_Implementation(TArray<TSubclassOf<UFGItemDescriptor>>& OutSlots) override;
	virtual bool CanUseFactoryClipboard_Implementation() override;
	virtual UFGFactoryClipboardSettings* CopySettings_Implementation() override;
	virtual bool PasteSettings_Implementation(UFGFactoryClipboardSettings* factoryClipboard,
	                                          class AFGPlayerController* player) override;
	virtual void Overclocking_GetProductionResults_Implementation(
		TArray<FKPCLOverclockingProductionResults>& OutIngredients,
		TArray<FKPCLOverclockingProductionResults>& OutProducts) override;

	UFUNCTION(BlueprintCallable, Category="KMods|Filtrator")
	TArray<UKAPICleanerItemDescription*> GetAllCleanerRecipes() const;

	UFUNCTION(BlueprintCallable, Category="KMods|Filtrator")
	UKAPICleanerItemDescription* GetCurrentCleanerRecipe() const;

	UFUNCTION(BlueprintCallable, Category="KMods|Filtrator")
	void SetCleanerRecipe(UKAPICleanerItemDescription* NewCleanerInfo);

	UFUNCTION(BlueprintCallable, Category="KMods|Filtrator")
	bool CanSetCleanerRecipe(UKAPICleanerItemDescription* NewCleanerInfo) const;

	virtual void SetPendingPotential(float newPendingPotential) override;

	bool ValidateRecipeSettings();
	void ApplyRecipeSettings();

	UPROPERTY(SaveGame, meta = ( FGReplicated ))
	UKAPICleanerItemDescription* mCurrentCleanerRecipe;

	UPROPERTY(SaveGame, meta = ( FGReplicated ))
	TArray<FFullProductionHandle> mSlotProductionHandler;

	UPROPERTY(SaveGame, meta = ( FGReplicated ))
	FFullProductionHandle mCleanerItemHandler;

	UPROPERTY(EditDefaultsOnly, Category="KMods|Filtrator")
	UKAPICleanerItemDescription* mDefaultCleanerRecipe;

	UPROPERTY(EditDefaultsOnly, Category="KMods|Filtrator")
	bool bShouldAutoSetRecipeIfNotSet = true;

	UPROPERTY(SaveGame)
	FSmartTimer mSearchForRecipeTimer = FSmartTimer(1.0f, true);

	int32 FLUID_INPUT_INDEX = 0;
	int32 CLEANER_ITEM_INDEX = 1;
	int32 FLUID_OUTPUT_INDEX = 2;
	int32 MAX_BYPASS_SLOTS = 10;

	/**
	 * ---------------- End Recipe System ----------------
	 */

	virtual void BeginPlay() override;
	virtual void Factory_Tick(float dt) override;

	virtual void CollectBelts() override;
	virtual void CollectAndPushPipes(float dt, bool IsPush) override;

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void GetConditionalReplicatedProps(TArray<FFGCondReplicatedProperty>& outProps) const override;

	virtual bool FilterInputInventory(TSubclassOf<UObject> object, int32 idx) const override;

	virtual UFGInventoryComponent* GetInventory() const override;
	virtual UFGInventoryComponent* GetOutputInventory() const override;

	// Production Stuff
	virtual bool CanProduce_Implementation() const override;
	virtual void onProducingFinal_Implementation() override;
	void CheckFluid(float dt);
	int32 GetFullestStackIndex() const;

	void ByPassProduceHandle(float dt);
	virtual void Server_DoFlush() override;

	UFUNCTION(BlueprintPure, Category="KMods|Cleaner")
	TArray<FFullProductionHandle> GetSlotHandler() const;

	UFUNCTION(BlueprintPure, Category="KMods|Cleaner")
	FFullProductionHandle GetCleanerItemHandler() const;

	UFUNCTION(BlueprintPure, Category="KMods|Cleaner")
	bool CheckOutputAndWasteInventory() const;

	UFUNCTION(BlueprintPure, Category="KMods|Cleaner")
	bool CheckConsumeAmount() const;

	/** Components */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="KMods|Cleaner")
	UFGPipeConnectionFactory* mPipeInput;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="KMods|Cleaner")
	UFGPipeConnectionFactory* mPipeOutput;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="KMods|Cleaner")
	UFGFactoryConnectionComponent* mBeltOutput;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="KMods|Cleaner")
	UFGFactoryConnectionComponent* mBeltInput;

	UPROPERTY(Transient)
	AKLUnlockSubsystem* mUnlockSubsystem;

	UPROPERTY(EditDefaultsOnly, SaveGame, Category = "KMods|Inventory")
	UFGInventoryComponent* mOutputInventory;

	UPROPERTY(EditDefaultsOnly, SaveGame, Category = "KMods|Inventory")
	UFGInventoryComponent* mInputInventory;

	UPROPERTY()
	TArray<TSubclassOf<UFGItemDescriptor>> mInputItems;
};
