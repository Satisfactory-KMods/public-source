// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "BFL/KBFL_Math.h"
#include "Buildable/KPCLProducerBase.h"
#include "Buildables/FGBuildableResourceExtractor.h"
#include "KPCLOverclockingInterface.h"
#include "Structures/KPCLFunctionalStructure.h"

#include "KPCLExtractorBase.generated.h"

UCLASS(Abstract)
class KPRIVATECODELIB_API AKPCLExtractorBase : public AFGBuildableResourceExtractor,
											   public IFGRecipeProducerInterface,
											   public IKPCLOverclockingInterface
{
	GENERATED_BODY()

public:
	AKPCLExtractorBase();

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
#endif

	//~ Begin AActor Interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void GetConditionalReplicatedProps(TArray<FFGCondReplicatedProperty>& outProps) const override;
	virtual bool ShouldSave_Implementation() const override;
	virtual void BeginPlay() override;
	//~ End AActor Interface

	//~ Begin AFGBuildableFactory Interface
	virtual void Factory_Tick(float dt) override;
	virtual bool CanProduce_Implementation() const override;
	virtual float GetProductionCycleTime() const override;
	virtual float GetDefaultProductionCycleTime() const override;
	virtual float GetProductionProgress() const override;
	virtual void SetPendingPotential(float newPendingPotential) override;
	virtual float GetCurrentMaxPotential() const override;
	virtual float CalcProductionCycleTimeForPotential(float potential) const override;
	virtual float GetProducingPowerConsumptionBase() const override;
	virtual void Factory_CollectInput_Implementation() override;
	virtual void Factory_PullPipeInput_Implementation(float dt) override;
	virtual void Factory_PushPipeOutput_Implementation(float dt) override;
	//~ End AFGBuildableFactory Interface

	//~ Begin IKPCLOverclockingInterface
	virtual bool Overclocking_IsConsumer_Implementation() override;
	virtual void UI_ApplyRelevantItems_Implementation(TArray<TSubclassOf<UFGItemDescriptor>>& OutSlots) override;
	virtual UFGInventoryComponent* Overclocking_GetInventory_Implementation() override;
	virtual bool Overclocking_ShouldUse_Implementation() override;
	virtual bool Overclocking_UseInventory_Implementation(int32& UnlockedSlots) override;
	virtual AFGBuildableFactory* Overclocking_GetFactory_Implementation() override;
	virtual void Overclocking_GetCostSlots_Implementation(TArray<FItemAmount>& OutSlots) override;
	virtual void Overclocking_GetInfo_Implementation(FKPCLOverclockingProductionInfo& OutProductionInfo) override;
	virtual void
	Overclocking_GetProductionResults_Implementation(TArray<FKPCLOverclockingProductionResults>& OutIngredients,
													 TArray<FKPCLOverclockingProductionResults>& OutProducts) override;
	//~ End IKPCLOverclockingInterface

	//~ Begin Clipboard
	virtual bool CanUseFactoryClipboard_Implementation() override;
	virtual UFGFactoryClipboardSettings* CopySettings_Implementation() override;
	virtual bool PasteSettings_Implementation(UFGFactoryClipboardSettings* factoryClipboard,
											  class AFGPlayerController* player) override;
	virtual void WriteClipboardSettings(UKPCLBaseClipboardSettings* ClipboardSettings);
	virtual void ApplyClipboardSettings(UKPCLBaseClipboardSettings* ClipboardSettings, AFGPlayerController* Player);
	//~ End Clipboard

	//~ Begin Interaction
	virtual void StartIsLookedAtForDismantle_Implementation(AFGCharacterPlayer* byCharacter) override;
	virtual void StartIsLookedAt_Implementation(AFGCharacterPlayer* byCharacter, const FUseState& state) override;
	virtual void StartIsAimedAtForColor_Implementation(AFGCharacterPlayer* byCharacter, bool isValid) override;
	virtual void StartIsLookedAtForConnection(AFGCharacterPlayer* byCharacter,
											  UFGCircuitConnectionComponent* overlappingConnection) override;
	void UpdateInstancesForOutline() const;
	//~ End Interaction

	virtual void OnBuildEffectFinished() override;
	virtual void OnBuildEffectActorFinished() override;
	virtual void ReadyForVisuelUpdate();

	//~ Begin AudioConfig
	virtual void InitAudioConfig();

	UFUNCTION(BlueprintImplementableEvent)
	void OnAudioConfigChanged();

	virtual void OnAudioConfigChanged_Native();
	//~ End AudioConfig

	UFUNCTION()
	virtual void OnOptionsUpdated(FString UpdatedCVar) {}

	UFUNCTION(BlueprintCallable, Category = "KMods")
	bool AIO_OverwriteInstanceData(UStaticMesh* Mesh, int32 Idx);

	UFUNCTION(BlueprintCallable, Category = "KMods")
	bool AIO_OverwriteInstanceData_Transform(UStaticMesh* Mesh, FTransform NewRelativTransform, int32 Idx);

	UFUNCTION(BlueprintCallable, Category = "KMods")
	bool AIO_UpdateCustomFloat(int32 FloatIndex, float Data, int32 InstanceIdx, bool MarkDirty = true);

	UFUNCTION(BlueprintCallable, Category = "KMods")
	bool AIO_UpdateCustomFloatAsColor(int32 StartFloatIndex, FLinearColor Data, int32 InstanceIdx,
									  bool MarkDirty = true);

	UFUNCTION(BlueprintCallable, Category = "KMods")
	bool AIO_SetInstanceHidden(int32 InstanceIdx, bool IsHidden);

	UFUNCTION(BlueprintCallable, Category = "KMods")
	bool AIO_SetInstanceWorldTransform(int32 InstanceIdx, FTransform Transform);

	/** This function resets the production time to 0. */
	UFUNCTION(BlueprintCallable, Category = "KMods ")
	void ResetProduction();

	/** {Only works on Server Side} Set a new Production Time. */
	UFUNCTION(BlueprintCallable, Category = "KMods ")
	void SetProductionTime(float NewTime, bool ShouldResetProduction = false);

	/** {Only works on Server Side} Set a new Power Option. */
	UFUNCTION(BlueprintCallable, Category = "KMods ")
	void SetPowerOption(FPowerOptions NewPowerOption);

	/** Get Power Option (Only a copy!) */
	UFUNCTION(BlueprintCallable, Category = "KMods ")
	FPowerOptions GetPowerOption() const;
	FPowerOptions& GetPowerOptionRef();

	/** Called when production cycle completes. Native implementations handle actual production logic. */
	UFUNCTION(BlueprintNativeEvent, Category = "KMods  Events")
	void onProducingFinal();

	virtual void onProducingFinal_Implementation();

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "KMods  Events")
	virtual float GetProductionTime() const;

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "KMods  Events")
	virtual float GetPendingProductionTime() const;

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "KMods|Production")
	FFullProductionHandle GetProductionHandle() const;

	UFUNCTION(BlueprintCallable, Category = "KMods|Cleaner")
	virtual void FlushFluids();
	FORCEINLINE virtual void Server_DoFlush() {};

	UFUNCTION()
	virtual void EndProductionTime();

	virtual void BeltPipeGrab(float dt);
	virtual void HandlePower(float dt);
	virtual void HandlePowerInit();
	virtual void HandleUiTick(float dt);
	virtual void InitComponents();

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "KMods|Production")
	virtual float GetMaxPowerConsume() const;

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "KMods|Production")
	virtual float GetPowerConsume() const;

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "KMods|Production")
	virtual bool GetHasPower() const;

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "KMods|Production")
	virtual UFGPowerConnectionComponent* GetPowerConnection() const;

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "KMods|Production")
	float GetCurrentPowerMultiplier() const;

	UFUNCTION(BlueprintCallable, Category = "KMods|Production")
	void SetPowerMultiplier(float NewMultiplier);

	UPROPERTY()
	TMap<FName, TObjectPtr<UFGInventoryComponent>> mCachedInventorys;

	UFUNCTION(BlueprintPure, Category = "KMods|Inventory")
	virtual UFGInventoryComponent* GetInventoryFromType(EKPCLInventoryType Type) const;

	UFUNCTION(BlueprintPure, Category = "KMods|Inventory")
	virtual UFGInventoryComponent* GetInventory() const;

	virtual void InitInputInventory();
	FORCEINLINE virtual bool FilterInputInventory(TSubclassOf<UObject> object, int32 idx) const { return true; }
	FORCEINLINE virtual bool FormFilterInputInventory(TSubclassOf<UFGItemDescriptor> object, int32 idx) const
	{
		return true;
	}

	UFUNCTION()
	virtual void OnInputItemRemoved(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved,
									UFGInventoryComponent* sourceInventory)
	{
	}

	UFUNCTION()
	virtual void OnInputItemAdded(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved,
								  UFGInventoryComponent* sourceInventory)
	{
	}

	virtual void InitOutputInventory();
	FORCEINLINE virtual bool FilterOutputInventory(TSubclassOf<UObject> object, int32 idx) const { return true; }
	FORCEINLINE virtual bool FormFilterOutputInventory(TSubclassOf<UFGItemDescriptor> object, int32 idx) const
	{
		return true;
	}

	UFUNCTION()
	virtual void OnOutputItemRemoved(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved,
									 UFGInventoryComponent* sourceInventory)
	{
	}

	UFUNCTION()
	virtual void OnOutputItemAdded(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved,
								   UFGInventoryComponent* sourceInventory)
	{
	}

	UFUNCTION(BlueprintPure, Category = "KMods|Inventory")
	virtual UFGInventoryComponent* GetBoosterInventory() const;

	virtual void InitBoosterInventory();
	FORCEINLINE virtual bool FilterBoosterInventory(TSubclassOf<UObject> object, int32 idx) const { return true; }
	FORCEINLINE virtual bool FormFilterBoosterInventory(TSubclassOf<UFGItemDescriptor> object, int32 idx) const
	{
		return true;
	}

	UFUNCTION()
	virtual void OnBoosterItemRemoved(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved,
									  UFGInventoryComponent* sourceInventory);

	UFUNCTION()
	virtual void OnBoosterItemAdded(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved,
									UFGInventoryComponent* sourceInventory);

	virtual void InitInventories();

	UFUNCTION(BlueprintCallable, Category = "KMods|Production")
	virtual void SetBelts();

	virtual void HandleIndicator();

	UFUNCTION(BlueprintCallable, Category = "KMods|Inventory")
	void ApplyNewProductionState(ENewProductionState NewState);

	UFUNCTION(BlueprintPure, Category = "KMods|Inventory")
	FORCEINLINE ENewProductionState GetCurrentProductionState() const { return mCurrentState; }

	UFUNCTION(BlueprintPure, Category = "KMods|Inventory")
	FORCEINLINE ENewProductionState GetLastProductionState() const { return mLastState; }

	UFUNCTION(BlueprintPure, Category = "KMods|Inventory")
	FORCEINLINE ENewProductionState GetCurrentCompProductionState() const
	{
		if (mCustomProductionStateIndicator)
		{
			return mCustomProductionStateIndicator->GetState();
		}
		return GetLastProductionState();
	}

	UFUNCTION(BlueprintPure)
	UKAPIDataAssetSubsystem* GetAssetSubsystem() const
	{
		if (!IsValid(mAssetSubsystem))
		{
			return UKAPIDataAssetSubsystem::GetChecked(GetWorld());
		}
		return mAssetSubsystem;
	}

	UFGFactoryConnectionComponent* GetConv(int Index, ECKPCLDirection Direction = KPCLInput) const;
	TArray<UFGFactoryConnectionComponent*> GetAllConv(ECKPCLDirection Direction = KPCLAny) const;
	UFGPipeConnectionFactory* GetPipe(int Index, ECKPCLDirection Direction = KPCLInput) const;
	TArray<UFGPipeConnectionFactory*> GetAllPipes(ECKPCLDirection Direction = KPCLAny) const;
	bool bInventoryHasInit = false;
	bool bCustomFactoryTickPreCustomLogic = false;
	bool bOneStateWasNotApplied = false;

	TMap<EKPCLConnectionType, TArray<UFGConnectionComponent*>> mConnectionMap;

	UPROPERTY(EditDefaultsOnly, Category = "KMods|Overclocking")
	bool bEnableCustomOverclocking = false;

	UPROPERTY(EditDefaultsOnly, Category = "KMods|UI")
	TArray<TSubclassOf<UFGItemDescriptor>> mRelevantItems;

	UPROPERTY(SaveGame, BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentState, Category = "KMods|Indicator")
	ENewProductionState mCurrentState = ENewProductionState::Max;

	UPROPERTY(BlueprintReadOnly, Category = "KMods|Indicator")
	ENewProductionState mLastState = ENewProductionState::Max;

	UPROPERTY(EditDefaultsOnly, Category = "KMods|Indicator")
	TArray<ENewProductionState> mPulsStates = {ENewProductionState::Error, ENewProductionState::Idle,
											   ENewProductionState::Paused};

	UPROPERTY(EditDefaultsOnly, SaveGame, meta = (FGReplicated), Category = "KMods")
	FFullProductionHandle mProductionHandle;

	UPROPERTY(EditDefaultsOnly, meta = (FGReplicated), Category = "KMods")
	FPowerOptions mPowerOptions;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<class UFGPowerConnectionComponent> mFGPowerConnection;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<class UKPCLBetterIndicator> mCustomProductionStateIndicator;

	UPROPERTY()
	TObjectPtr<UKAPIDataAssetSubsystem> mAssetSubsystem;

protected:
	virtual void Factory_TickAuthOnly(float dt);
	virtual void Factory_TickClientOnly(float dt);

	virtual void ReApplyColorForIndex(int32 Idx, const FFactoryCustomizationData& customizationData);
	virtual void ApplyCustomizationData_Native(const FFactoryCustomizationData& customizationData) override;
	virtual void SetCustomizationData_Native(const FFactoryCustomizationData& customizationData,
											 bool skipCombine) override;

	virtual void InitMeshOverwriteInformation();
	void ApplyMeshOverwriteInformation(int32 Idx);
	void ApplyMeshInformation(FKPCLMeshOverwriteInformation Information);
	virtual bool ShouldOverwriteIndexHandle(int32 Idx, FKPCLMeshOverwriteInformation& Information);

	virtual void CollectBelts();
	virtual void CollectAndPushPipes(float dt, bool IsPush);

	TMap<int32, TMap<int32, float>> mCachedCustomData;
	TMap<int32, FTransform> mCachedTransforms;

	UFUNCTION()
	virtual void OnRep_MeshOverwriteInformations();

	UFUNCTION()
	virtual void OnRep_CurrentState();

	void SetCurrentProductionState(ENewProductionState NewState);

	UPROPERTY(EditDefaultsOnly, Category = "KMods|Mesh")
	TArray<FKPCLMeshOverwriteInformation> mDefaultMeshOverwriteInformations;

	UPROPERTY(ReplicatedUsing = OnRep_MeshOverwriteInformations, SaveGame)
	TArray<FKPCLMeshOverwriteInformation> mMeshOverwriteInformations;

	UPROPERTY(EditDefaultsOnly, Category = "KMods|Mesh")
	TArray<int32> mCustomIndicatorHandleIndexes;

	UPROPERTY(EditDefaultsOnly, Category = "KMods|Indicator", meta = (ArraySizeEnum = "ENewProductionState"))
	FLinearColor mStateColors[11];

	UPROPERTY(EditDefaultsOnly, Category = "KMods|Indicator")
	int32 mDefaultIndex = 20;

	UPROPERTY(EditDefaultsOnly, Category = "KMods|Indicator")
	int32 mIntensity = 2;

	UPROPERTY(EditDefaultsOnly, Category = "KMods|Config")
	FKPCLModConfigHelper_Float mAudioConfig;
	TArray<FKPCLAudioComponent> mAudioComponents;
};
