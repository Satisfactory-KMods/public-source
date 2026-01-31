// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "Buildable/KPCLExtractorBase.h"
#include "Buildable/Modular/KPCLModularExtractorBase.h"
#include "Buildables/FGBuildableResourceExtractor.h"
#include "Resources/FGResourceNode.h"
#include "Structures/KPCLFunctionalStructure.h"

#include "KLMMBuildableBase.generated.h"

/**
 * 
 */
UCLASS(Abstract)
class KLIB_API AKLMMBuildableBase : public AKPCLModularExtractorBase
{
	GENERATED_BODY()

public:
	AKLMMBuildableBase();

	virtual void BeginPlay() override;
	virtual void Factory_Tick(float dt) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void GetConditionalReplicatedProps(TArray<FFGCondReplicatedProperty>& outProps) const override;
	virtual void GetChildDismantleActors_Implementation(TArray<AActor*>& out_ChildDismantleActors) const override;

	virtual bool Factory_HasPower() const override;

	// NewProduction Code
	virtual void SetupInventory();
	virtual float GetNodeProductionTime() const;

	/** ---------------------------------- START ------------------------- */

	// Disable normal Production Tick
	virtual void Factory_ProductionCycleCompleted(float overProductionRate) override;;
	virtual void EndProductionTime() override;

	virtual void Factory_TickProducing(float dt) override;;
	virtual float GetProductionCycleTime() const override { return mMinerTask.GetProductionTime(); };
	virtual float GetProductionCycleBuff() const { return 1.0f; };
	virtual bool CanProduce_Implementation() const override;
	virtual bool CheckProduction() const { return false; };

	UFUNCTION(BlueprintPure, Category = "KMods|Production")
	virtual float GetModuleProductionCycleTime() const { return mMinerTask.mProductionTime * 3; };

	virtual void HandleState();;

	virtual void HandlePower(float dt) override;
	virtual void HandlePowerInit() override;

	// Start Production
	UFUNCTION(BlueprintNativeEvent, Category = "KMods|Production")
	void OnModuleProductionFinial();

	virtual void OnModuleProductionFinial_Implementation();
	virtual void ApplyPotentialToModule();
	virtual void SetPendingPotential(float newPendingPotential) override;

	UFUNCTION(BlueprintNativeEvent, Category = "KMods|Production")
	void OnFluidProductionFinial();

	virtual void OnFluidProductionFinial_Implementation();

	UFUNCTION(BlueprintNativeEvent, Category = "KMods|Production")
	void OnMainProductionFinial();

	virtual void OnMainProductionFinial_Implementation();

	/** ---------------------------------- END ------------------------- */

	UFUNCTION(BlueprintPure, Category = "KMods|Production")
	float GetProductionsPerMin() const;

	virtual void FlushFluids() override;
	virtual void Server_Flush();

	/** Getter */

	UFUNCTION(BlueprintPure, Category="KMods|Modular Extractor")
	float GetPurityMulti() const;

	UFUNCTION(BlueprintPure, Category="KMods|Modular Extractor")
	TSubclassOf<UFGResourceDescriptor> GetResourceClass() const;

	UFUNCTION(BlueprintPure, Category="KMods|Modular Extractor")
	EResourceForm GetResourceForm() const;

	/** Getter */
	UFUNCTION(BlueprintPure, Category="KMods|Modular Extractor")
	EResourcePurity GetPurity() const;

	UFUNCTION(BlueprintPure, Category="KMods|Modular Extractor")
	int GetOccupiedPoints(TSubclassOf<UKAPIModularAttachmentDescriptor> AttachmentClass) const;

	UFUNCTION(BlueprintPure, Category="KMods|Modular Extractor")
	bool HasModule(TSubclassOf<UKAPIModularAttachmentDescriptor> AttachmentClass) const;

	UFUNCTION(BlueprintPure, Category="KMods|Modular Extractor")
	bool HasEnergyModuleSlot(TSubclassOf<UKAPIModularAttachmentDescriptor> AttachmentClass, int idx) const;

	UFUNCTION(BlueprintPure, Category="KMods|Modular Extractor")
	TArray<bool> GetBoolArrayOfEnergySlots(TSubclassOf<UKAPIModularAttachmentDescriptor> AttachmentClass) const;

	UFUNCTION(BlueprintPure, Category="KMods|Modular Extractor")
	TArray<class AKLMMBuildableModule*> GetAllModules() const;

	UFUNCTION(BlueprintPure, Category="KMods|Modular Extractor")
	TArray<class AKLMMBuildableModule*> GetModule(TSubclassOf<UKAPIModularAttachmentDescriptor> AttachmentClass) const;

	UFUNCTION(BlueprintPure, Category="KMods|Modular Extractor")
	class AKLMMBuildableModule* GetFirstModule(TSubclassOf<UKAPIModularAttachmentDescriptor> AttachmentClass) const;

	UFUNCTION(BlueprintPure, Category="KMods|Modular Extractor")
	virtual bool IsModuleAllowed(TSubclassOf<AKLMMBuildableModule> AttachmentClass) const { return false; }

	UPROPERTY(BlueprintReadOnly, SaveGame, meta = ( FGReplicated ), Category="KMods|Modular Extractor")
	TSubclassOf<UFGItemDescriptor> mResourceToProduce;

	UFUNCTION(BlueprintPure, Category="KMods|Modular Extractor")
	virtual TSubclassOf<UFGItemDescriptor> GetResourceToProduce() const { return mResourceToProduce; }

	virtual void SetResourceToProduce();
	virtual void OnResourceToProduceChanged();

	virtual float GetMaxPowerConsume() const override;
	virtual float GetPowerConsume() const override;

	UPROPERTY(BlueprintReadOnly, meta = ( FGReplicated ), Category="KMods|Modular Extractor")
	FFullProductionHandle mModuleProductionTask;

	UFUNCTION(BlueprintPure, Category = "KMods|Production")
	FFullProductionHandle GetModuleProductionHandle() const { return mModuleProductionTask; }

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, meta = ( FGReplicated ), Category="KMods|Modular Extractor")
	FFullProductionHandle mFluidConsumeTask;

	UFUNCTION(BlueprintPure, Category = "KMods|Production")
	FFullProductionHandle GetFluidProductionHandle() const { return mFluidConsumeTask; }

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, meta = ( FGReplicated ), Category="KMods|Modular Extractor")
	FFullProductionHandle mMinerTask;

	UFUNCTION(BlueprintPure, Category = "KMods|Production")
	FFullProductionHandle GetMinerProductionHandle() const { return mMinerTask; }

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category="KMods|Modular Extractor")
	TSubclassOf<UKAPIModularAttachmentDescriptor> mWasteProducerAttachment;

	/** Get Production time (based also on Power Shards!) */
	virtual float GetProductionTime() const override;

	/** Get Production time (PENDING) (based also on Power Shards!) */
	virtual float GetPendingProductionTime() const override;

	// UI Stuff Start
	UPROPERTY(meta = ( FGReplicated ), BlueprintReadOnly)
	float mUIProductionBuff = 0.0f;
	// UI Stuff End

	UPROPERTY(EditDefaultsOnly)
	UFGInventoryComponent* mInventory;

	UPROPERTY(EditDefaultsOnly)
	UFGInventoryComponent* mBoosterInventory;
};
