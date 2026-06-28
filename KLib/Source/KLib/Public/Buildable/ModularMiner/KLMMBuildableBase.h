// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "Buildable/KPCLExtractorBase.h"
#include "Buildable/Modular/KPCLModularExtractorBase.h"
#include "Buildables/FGBuildableResourceExtractor.h"
#include "Resources/FGResourceNode.h"
#include "Structures/KPCLFunctionalStructure.h"

#include "KLMMBuildableBase.generated.h"

UCLASS(Abstract)
class KLIB_API AKLMMBuildableBase : public AKPCLModularExtractorBase
{
	GENERATED_BODY()

public:
	AKLMMBuildableBase();

	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	//~ End AActor Interface

	//~ Begin IKPCLModularBuildingInterface
	virtual void OnModulesUpdated_Implementation() override;
	//~ End IKPCLModularBuildingInterface

	//~ Begin AFGBuildableFactory Interface
	virtual bool CanProduce_Implementation() const override;
	virtual void Factory_ProductionCycleCompleted(float overProductionRate) override;
	virtual void Factory_TickAuthOnly(float dt) override;
	virtual void Factory_TickProducing(float dt) override;
	virtual float GetProductionCycleTime() const override { return mMinerTask.GetProductionTime(); }
	virtual void GetChildDismantleActors_Implementation(TArray<AActor*>& out_ChildDismantleActors) const override;
	virtual void GetConditionalReplicatedProps(TArray<FFGCondReplicatedProperty>& outProps) const override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool Factory_HasPower() const override;
	//~ End AFGBuildableFactory Interface

	//~ Begin AKPCLExtractorBase Interface
	virtual void EndProductionTime() override;
	virtual void HandlePower(float dt) override;
	virtual void HandlePowerInit() override;
	virtual float GetMaxPowerConsume() const override;
	virtual float GetPowerConsume() const override;
	virtual float GetProductionTime() const override;
	virtual float GetPendingProductionTime() const override;
	virtual void SetPendingPotential(float newPendingPotential) override;
	virtual void FlushFluids() override;
	//~ End AKPCLExtractorBase Interface

	virtual void SetupInventory();
	virtual float GetNodeProductionTime() const;
	virtual float GetProductionCycleBuff() const { return 1.0f; }
	virtual bool CheckProduction() const { return false; }
	virtual void HandleState();
	virtual void Server_Flush();
	virtual void OnResourceToProduceChanged();
	virtual void SetResourceToProduce();
	virtual void ApplyPotentialToModule();

	void SetUIProductionBuff(float NewBuff);
	void SetResourceToProduceInternal(TSubclassOf<UFGItemDescriptor> NewResource);

	/** Centralised dirty-marking for the replicated production handles. Always mutate the handle and
	 *  then call the matching Commit* so MarkPropertyDirty happens in exactly one place per property. */
	void CommitMinerTask();
	void CommitModuleProductionTask();
	void CommitFluidConsumeTask();
	void CommitUIProductionBuff();

	UFUNCTION(BlueprintNativeEvent, Category = "KMods|Production")
	void OnModuleProductionFinial();

	virtual void OnModuleProductionFinial_Implementation();

	UFUNCTION(BlueprintNativeEvent, Category = "KMods|Production")
	void OnFluidProductionFinial();

	virtual void OnFluidProductionFinial_Implementation();

	UFUNCTION(BlueprintNativeEvent, Category = "KMods|Production")
	void OnMainProductionFinial();

	virtual void OnMainProductionFinial_Implementation();

	UFUNCTION(BlueprintPure, Category = "KMods|Production")
	float GetProductionsPerMin() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Production")
	virtual float GetModuleProductionCycleTime() const { return mMinerTask.mProductionTime * 3; }

	UFUNCTION(BlueprintPure, Category = "KMods|Modular Extractor")
	float GetPurityMulti() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Modular Extractor")
	TSubclassOf<UFGResourceDescriptor> GetResourceClass() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Modular Extractor")
	EResourceForm GetResourceForm() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Modular Extractor")
	EResourcePurity GetPurity() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Modular Extractor")
	int GetOccupiedPoints(TSubclassOf<UKAPIModularAttachmentDescriptor> AttachmentClass) const;

	UFUNCTION(BlueprintPure, Category = "KMods|Modular Extractor")
	bool HasModule(TSubclassOf<UKAPIModularAttachmentDescriptor> AttachmentClass) const;

	UFUNCTION(BlueprintPure, Category = "KMods|Modular Extractor")
	bool HasEnergyModuleSlot(TSubclassOf<UKAPIModularAttachmentDescriptor> AttachmentClass, int idx) const;

	UFUNCTION(BlueprintPure, Category = "KMods|Modular Extractor")
	TArray<bool> GetBoolArrayOfEnergySlots(TSubclassOf<UKAPIModularAttachmentDescriptor> AttachmentClass) const;

	UFUNCTION(BlueprintPure, Category = "KMods|Modular Extractor")
	TArray<class AKLMMBuildableModule*> GetAllModules() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Modular Extractor")
	TArray<class AKLMMBuildableModule*> GetModule(TSubclassOf<UKAPIModularAttachmentDescriptor> AttachmentClass) const;

	UFUNCTION(BlueprintPure, Category = "KMods|Modular Extractor")
	class AKLMMBuildableModule* GetFirstModule(TSubclassOf<UKAPIModularAttachmentDescriptor> AttachmentClass) const;

	UFUNCTION(BlueprintPure, Category = "KMods|Modular Extractor")
	virtual bool IsModuleAllowed(TSubclassOf<AKLMMBuildableModule> AttachmentClass) const { return false; }

	UFUNCTION(BlueprintPure, Category = "KMods|Modular Extractor")
	virtual TSubclassOf<UFGItemDescriptor> GetResourceToProduce() const { return mResourceToProduce; }

	UFUNCTION(BlueprintPure, Category = "KMods|Production")
	FFullProductionHandle GetModuleProductionHandle() const { return mModuleProductionTask; }

	UFUNCTION(BlueprintPure, Category = "KMods|Production")
	FFullProductionHandle GetFluidProductionHandle() const { return mFluidConsumeTask; }

	UFUNCTION(BlueprintPure, Category = "KMods|Production")
	FFullProductionHandle GetMinerProductionHandle() const { return mMinerTask; }

	UPROPERTY(EditDefaultsOnly, Category = "KMods|Modular Extractor")
	TSubclassOf<UKAPIModularAttachmentDescriptor> mWasteProducerAttachment;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (FGReplicated), Category = "KMods|Modular Extractor")
	FFullProductionHandle mFluidConsumeTask;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (FGReplicated), Category = "KMods|Modular Extractor")
	FFullProductionHandle mMinerTask;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UFGInventoryComponent> mInventory;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UFGInventoryComponent> mBoosterInventory;

	UPROPERTY(BlueprintReadOnly, SaveGame, meta = (FGReplicated), Category = "KMods|Modular Extractor")
	TSubclassOf<UFGItemDescriptor> mResourceToProduce;

	UPROPERTY(BlueprintReadOnly, meta = (FGReplicated), Category = "KMods|Modular Extractor")
	FFullProductionHandle mModuleProductionTask;

	UPROPERTY(meta = (FGReplicated), BlueprintReadOnly)
	float mUIProductionBuff = 0.0f;

protected:
	/**
	 * Rebuilds mCachedModules from the handler. Called by OnModulesUpdated_Implementation on both server
	 * (via handler's OnRep_AttachmentDatas) and client (via replication RepNotify). Override in subclasses
	 * to add derived-class caches.
	 */
	virtual void RecalculateMinerCaches();

	/** Transient cache of attached modules. Rebuilt by RecalculateMinerCaches. Not replicated — derived from
	 *  the replicated handler state on each side independently. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<AKLMMBuildableModule>> mCachedModules;
};
