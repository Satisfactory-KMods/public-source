// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "Buildables/FGBuildableWaterPump.h"

#include "KLMMBuildableBase.h"

#include "KLMMBuildableMiner.generated.h"

UCLASS()
class KLIB_API AKLMMBuildableMiner : public AKLMMBuildableBase
{
	GENERATED_BODY()

public:
	AKLMMBuildableMiner();

	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	//~ End AActor Interface

	//~ Begin AFGBuildableFactory Interface
	virtual void Factory_ProductionCycleCompleted(float overProductionRate) override;
	virtual void Factory_TickAuthOnly(float dt) override;
	virtual void Factory_TickProducing(float dt) override;
	virtual void CollectAndPushPipes(float dt, bool IsPush) override;
	virtual void GetConditionalReplicatedProps(TArray<FFGCondReplicatedProperty>& outProps) const override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnBuildEffectFinished() override;
	//~ End AFGBuildableFactory Interface

	//~ Begin AKPCLProducerBase Interface
	virtual void Overclocking_GetCostSlots_Implementation(TArray<FItemAmount>& OutSlots) override;
	virtual void Overclocking_GetInfo_Implementation(FKPCLOverclockingProductionInfo& OutProductionInfo) override;
	virtual void
	Overclocking_GetProductionResults_Implementation(TArray<FKPCLOverclockingProductionResults>& OutIngredients,
													 TArray<FKPCLOverclockingProductionResults>& OutProducts) override;
	virtual bool Overclocking_UseInventory_Implementation(int32& UnlockedSlots) override;
	virtual void SetPendingPotential(float newPendingPotential) override;
	virtual void UI_ApplyRelevantItems_Implementation(TArray<TSubclassOf<UFGItemDescriptor>>& OutSlots) override;
	//~ End AKPCLProducerBase Interface

	//~ Begin AKLMMBuildableBase Interface
	virtual bool CheckProduction() const override;
	virtual float GetCurrentMaxPotential() const override;
	virtual float GetNodeProductionTime() const override;
	virtual float GetProductionCycleBuff() const override;
	virtual void HandlePower(float dt) override;
	virtual void HandleState() override;
	virtual bool IsModuleAllowed(TSubclassOf<AKLMMBuildableModule> Module) const override;
	virtual void OnFluidProductionFinial_Implementation() override;
	virtual void OnMainProductionFinial_Implementation() override;
	virtual void OnModuleProductionFinial_Implementation() override;
	virtual void SetupInventory() override;
	virtual TSubclassOf<UFGItemDescriptor> GetResourceToProduce() const override;
	//~ End AKLMMBuildableBase Interface

	bool CheckStorage() const;

	int32 GetSizeFromOutputSlots() const;
	int32 GetClampedPossibleProduction() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Modular Extractor")
	bool HasAllNeededModules() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Modular Extractor")
	FKAPIMinerInfoFluids GetCurrentFluidInfo(TSubclassOf<UFGItemDescriptor>& FluidDesc) const;

	UFUNCTION(BlueprintPure, Category = "KMods|Modular Extractor")
	TSubclassOf<UFGItemDescriptor> GetCurrentFluid() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Modular Extractor")
	TArray<TSubclassOf<UFGItemDescriptor>> GetAllowedFluidDesc() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Modular Extractor")
	float GetTierMulti(int Tier) const;

	UFUNCTION(BlueprintPure, Category = "KMods|Modular Extractor")
	TSubclassOf<UFGItemDescriptor> GetTrashDescriptor() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Modular Extractor")
	UKAPIModularMinerDescription* GetMinerInfo(bool& Valid) const;

	UFUNCTION(BlueprintPure, Category = "KMods|Modular Extractor")
	bool IsValidResource() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Modular Extractor")
	int GetProductionCrusherAmount() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Modular Extractor")
	int GetProductionAmount() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Modular Extractor")
	int GetCrusherProductionAmount() const;

	UKAPIModularMinerDescription* GetMinerInfoInternal() const;

	void SetExtractionInfo(UKAPIModularMinerDescription* NewInfo);
	void SetCachedFluid(TSubclassOf<UFGItemDescriptor> NewFluid);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|UI")
	TMap<TSubclassOf<AKLMMBuildableModule>, TSubclassOf<UFGBuildDescriptor>> mOverclockingBuildingMapping;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|Attachment")
	TSubclassOf<UKAPIModularAttachmentDescriptor> mFluidAttachmentClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|Attachmentr")
	TSubclassOf<UKAPIModularAttachmentDescriptor> mPowerShardAttachmentClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|Attachment")
	TSubclassOf<UKAPIModularAttachmentDescriptor> mDrillAttachmentClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|Attachment")
	TSubclassOf<UKAPIModularAttachmentDescriptor> mWasteProducerAttachmentClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|Modular Extractor")
	float mModuleMultiplier = 3.f;

	UPROPERTY(meta = (FGReplicated), SaveGame, BlueprintReadOnly, Category = "KMods|Modular Extractor")
	TObjectPtr<UKAPIModularMinerDescription> mExtractionInfo;

	UPROPERTY(meta = (FGReplicated), SaveGame)
	TSubclassOf<UFGItemDescriptor> mCachedFluid;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KMods|Modular Extractor")
	TObjectPtr<class UFGPipeConnectionFactory> mPipeInput;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KMods|Modular Extractor")
	TObjectPtr<class UFGFactoryConnectionComponent> mBeltOutput_01;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KMods|Modular Extractor")
	TObjectPtr<class UFGFactoryConnectionComponent> mBeltOutput_02;
};
