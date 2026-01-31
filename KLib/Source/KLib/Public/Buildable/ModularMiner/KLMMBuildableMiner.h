// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "KLMMBuildableBase.h"
#include "Buildables/FGBuildableWaterPump.h"

#include "KLMMBuildableMiner.generated.h"

/**
 * 
 */
UCLASS()
class KLIB_API AKLMMBuildableMiner : public AKLMMBuildableBase
{
	GENERATED_BODY()

public:
	AKLMMBuildableMiner();

	virtual void UI_ApplyRelevantItems_Implementation(TArray<TSubclassOf<UFGItemDescriptor>>& OutSlots) override;
	virtual bool Overclocking_UseInventory_Implementation(int32& UnlockedSlots) override;
	virtual void Overclocking_GetCostSlots_Implementation(TArray<FItemAmount>& OutSlots) override;
	virtual void Overclocking_GetInfo_Implementation(FKPCLOverclockingProductionInfo& OutProductionInfo) override;
	virtual void SetPendingPotential(float newPendingPotential) override;
	virtual void Overclocking_GetProductionResults_Implementation(
		TArray<FKPCLOverclockingProductionResults>& OutIngredients,
		TArray<FKPCLOverclockingProductionResults>& OutProducts) override;
	virtual void OnBuildEffectFinished() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="KMods|UI")
	TMap<TSubclassOf<AKLMMBuildableModule>, TSubclassOf<UFGBuildDescriptor>> mOverclockingBuildingMapping;

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void GetConditionalReplicatedProps(TArray<FFGCondReplicatedProperty>& outProps) const override;

	virtual void Factory_Tick(float dt) override;
	virtual void CollectAndPushPipes(float dt, bool IsPush) override;
	virtual void BeginPlay() override;

	// Disable normal Production Tick
	virtual void Factory_ProductionCycleCompleted(float overProductionRate) override;
	virtual void Factory_TickProducing(float dt) override;

	virtual void SetupInventory() override;
	virtual void HandleState() override;
	virtual void HandlePower(float dt) override;

	int32 GetSizeFromOutputSlots() const;
	int32 GetClampedPossibleProduction() const;

	// Produced Item
	virtual TSubclassOf<UFGItemDescriptor> GetResourceToProduce() const override;


	// Start Production
	virtual bool CheckProduction() const override;
	bool CheckStorage() const;

	virtual float GetProductionCycleBuff() const override;
	virtual float GetCurrentMaxPotential() const override;
	virtual float GetNodeProductionTime() const override;

	virtual void OnMainProductionFinial_Implementation() override;
	virtual void OnModuleProductionFinial_Implementation() override;
	virtual void OnFluidProductionFinial_Implementation() override;
	// End Production

	/** ---------------------------------- END ------------------------- */

	UFUNCTION(BlueprintPure, Category="KMods|Modular Extractor")
	bool HasAllNeededModules() const;

	UFUNCTION(BlueprintPure, Category="KMods|Modular Extractor")
	FKAPIMinerInfoFluids GetCurrentFluidInfo(TSubclassOf<UFGItemDescriptor>& FluidDesc) const;

	UFUNCTION(BlueprintPure, Category="KMods|Modular Extractor")
	TSubclassOf<UFGItemDescriptor> GetCurrentFluid() const;

	UFUNCTION(BlueprintPure, Category="KMods|Modular Extractor")
	TArray<TSubclassOf<UFGItemDescriptor>> GetAllowedFluidDesc() const;

	UFUNCTION(BlueprintPure, Category="KMods|Modular Extractor")
	float GetTierMulti(int Tier) const;

	UFUNCTION(BlueprintPure, Category="KMods|Modular Extractor")
	TSubclassOf<UFGItemDescriptor> GetTrashDescriptor() const;

	UFUNCTION(BlueprintPure, Category="KMods|Modular Extractor")
	UKAPIModularMinerDescription* GetMinerInfo(bool& Valid) const;
	UKAPIModularMinerDescription* GetMinerInfoInternal() const;

	UFUNCTION(BlueprintPure, Category="KMods|Modular Extractor")
	bool IsValidResource() const;

	UFUNCTION(BlueprintPure, Category="KMods|Modular Extractor")
	int GetProductionCrusherAmount() const;

	UFUNCTION(BlueprintPure, Category="KMods|Modular Extractor")
	int GetProductionAmount() const;

	UFUNCTION(BlueprintPure, Category="KMods|Modular Extractor")
	int GetCrusherProductionAmount() const;

	virtual bool IsModuleAllowed(TSubclassOf<AKLMMBuildableModule> Module) const override;

	/** UPROPERTY */
	// This should never be null
	UPROPERTY(meta = ( FGReplicated ), SaveGame, BlueprintReadOnly, Category="KMods|Modular Extractor")
	UKAPIModularMinerDescription* mExtractionInfo;

	UPROPERTY(meta = ( FGReplicated ), SaveGame)
	TSubclassOf<UFGItemDescriptor> mCachedFluid;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="KMods|Attachment")
	TSubclassOf<UKAPIModularAttachmentDescriptor> mFluidAttachmentClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="KMods|Attachmentr")
	TSubclassOf<UKAPIModularAttachmentDescriptor> mPowerShardAttachmentClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="KMods|Attachment")
	TSubclassOf<UKAPIModularAttachmentDescriptor> mDrillAttachmentClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="KMods|Attachment")
	TSubclassOf<UKAPIModularAttachmentDescriptor> mWasteProducerAttachmentClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="KMods|Modular Extractor")
	float mModuleMultiplier = 3.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="KMods|Modular Extractor")
	class UFGPipeConnectionFactory* mPipeInput;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="KMods|Modular Extractor")
	class UFGFactoryConnectionComponent* mBeltOutput_01;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="KMods|Modular Extractor")
	class UFGFactoryConnectionComponent* mBeltOutput_02;
};
