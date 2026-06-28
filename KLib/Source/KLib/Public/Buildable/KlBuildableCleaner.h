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
	TObjectPtr<UKAPICleanerItemDescription> mCurrentCleanerRecipe;
};

UCLASS()
class KLIB_API AKLBuildableCleaner : public AKPCLProducerBase
{
	GENERATED_BODY()

public:
	AKLBuildableCleaner();

	//~ Begin AFGBuildableFactory Interface
	virtual bool CanProduce_Implementation() const override;
	virtual bool CanUseFactoryClipboard_Implementation() override;
	virtual bool FilterInputInventory(TSubclassOf<UObject> object, int32 idx) const override;
	virtual void CollectBelts() override;
	virtual void CollectAndPushPipes(float dt, bool IsPush) override;
	virtual void Factory_TickAuthOnly(float dt) override;
	virtual void GetConditionalReplicatedProps(TArray<FFGCondReplicatedProperty>& outProps) const override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void onProducingFinal_Implementation() override;
	virtual UFGFactoryClipboardSettings* CopySettings_Implementation() override;
	virtual UFGInventoryComponent* GetInventory() const override;
	virtual UFGInventoryComponent* GetOutputInventory() const override;
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
	virtual bool Overclocking_IsConsumer_Implementation() override;
	virtual void SetPendingPotential(float newPendingPotential) override;
	virtual void UI_ApplyRelevantItems_Implementation(TArray<TSubclassOf<UFGItemDescriptor>>& OutSlots) override;
	//~ End AKPCLProducerBase Interface

	UFUNCTION(BlueprintCallable, Category = "KMods|Filtrator")
	TArray<UKAPICleanerItemDescription*> GetAllCleanerRecipes() const;

	UFUNCTION(BlueprintCallable, Category = "KMods|Filtrator")
	bool CanSetCleanerRecipe(UKAPICleanerItemDescription* NewCleanerInfo) const;

	UFUNCTION(BlueprintCallable, Category = "KMods|Filtrator")
	UKAPICleanerItemDescription* GetCurrentCleanerRecipe() const;

	UFUNCTION(BlueprintCallable, Category = "KMods|Filtrator")
	void SetCleanerRecipe(UKAPICleanerItemDescription* NewCleanerInfo);

	UFUNCTION(BlueprintPure, Category = "KMods|Cleaner")
	bool CheckConsumeAmount() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Cleaner")
	bool CheckOutputAndWasteInventory() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Cleaner")
	FFullProductionHandle GetCleanerItemHandler() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Cleaner")
	TArray<FFullProductionHandle> GetSlotHandler() const;

	void ApplyRecipeSettings();
	void ConfigureRecipeSlots();
	void ByPassProduceHandle(float dt);
	void CheckFluid(float dt);
	int32 GetFullestStackIndex() const;
	bool ValidateRecipeSettings();
	virtual void Server_DoFlush() override;

	/** Centralised dirty-marking for the replicated production handles. */
	void CommitProductionHandlers();

	int32 CLEANER_ITEM_INDEX = 1;
	int32 FLUID_INPUT_INDEX = 0;
	int32 FLUID_OUTPUT_INDEX = 2;
	int32 MAX_BYPASS_SLOTS = 10;

	UPROPERTY(EditDefaultsOnly, SaveGame, Category = "KMods|Inventory")
	TObjectPtr<UFGInventoryComponent> mInputInventory;

	UPROPERTY(EditDefaultsOnly, SaveGame, Category = "KMods|Inventory")
	TObjectPtr<UFGInventoryComponent> mOutputInventory;

	UPROPERTY(EditDefaultsOnly, Category = "KMods|Filtrator")
	bool bShouldAutoSetRecipeIfNotSet = true;

	UPROPERTY(EditDefaultsOnly, Category = "KMods|Filtrator")
	TObjectPtr<UKAPICleanerItemDescription> mDefaultCleanerRecipe;

	UPROPERTY(SaveGame, meta = (FGReplicated))
	FFullProductionHandle mCleanerItemHandler;

	UPROPERTY(SaveGame, meta = (FGReplicated))
	TArray<FFullProductionHandle> mSlotProductionHandler;

	UPROPERTY(SaveGame, meta = (FGReplicated))
	TObjectPtr<UKAPICleanerItemDescription> mCurrentCleanerRecipe;

	UPROPERTY(SaveGame)
	FSmartTimer mSearchForRecipeTimer = FSmartTimer(1.0f, true);

	UPROPERTY(Transient)
	TObjectPtr<AKLUnlockSubsystem> mUnlockSubsystem;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KMods|Cleaner")
	TObjectPtr<UFGFactoryConnectionComponent> mBeltInput;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KMods|Cleaner")
	TObjectPtr<UFGFactoryConnectionComponent> mBeltOutput;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KMods|Cleaner")
	TObjectPtr<UFGPipeConnectionFactory> mPipeInput;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KMods|Cleaner")
	TObjectPtr<UFGPipeConnectionFactory> mPipeOutput;

	UPROPERTY()
	TArray<TSubclassOf<UFGItemDescriptor>> mInputItems;
};
