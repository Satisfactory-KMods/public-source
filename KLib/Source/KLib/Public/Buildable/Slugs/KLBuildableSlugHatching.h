// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "FGTimeSubsystem.h"
#include "KLBuildableSlugBuildingBase.h"
#include "KLBuildableSlugHatchingModule.h"

#include "KLBuildableSlugHatching.generated.h"

USTRUCT(Blueprintable, BlueprintType)
struct KLIB_API FKLSlugHatcherModuleSetting
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	EHatchingModuleType mModuleType;

	UPROPERTY(BlueprintReadWrite)
	uint8 mModuleTier;

	UPROPERTY(BlueprintReadWrite)
	float mTemperature;

	UPROPERTY(BlueprintReadWrite)
	float mHumidity;

	UPROPERTY(BlueprintReadWrite)
	EKAPISlugTime mOverwriteTime = EKAPISlugTime::NONE;
};

UCLASS(Blueprintable, BlueprintType)
class KLIB_API UKLSlugHatcherClipboardSettings : public UKPCLBaseClipboardSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite)
	TArray<FKLSlugHatcherModuleSetting> mModuleSettings;
};

/**
 *
 */
UCLASS(Blueprintable, BlueprintType)
class KLIB_API AKLBuildableSlugHatching : public AKLBuildableSlugBuildingBase
{
	GENERATED_BODY()

public:
	AKLBuildableSlugHatching();

	//~ Begin AFGBuildableFactory Interface
	virtual bool CanProduce_Implementation() const override;
	virtual bool CanUseFactoryClipboard_Implementation() override;
	virtual void CollectAndPushPipes(float dt, bool IsPush) override;
	virtual void CollectBelts() override;
	virtual void Factory_Tick(float dt) override;
	virtual void GetConditionalReplicatedProps(TArray<FFGCondReplicatedProperty>& outProps) const override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void HandleIndicator() override;
	virtual void onProducingFinal_Implementation() override;
	virtual UFGFactoryClipboardSettings* CopySettings_Implementation() override;
	virtual bool FilterInputInventory(TSubclassOf<UObject> object, int32 idx) const override;
	virtual bool FilterOutputInventory(TSubclassOf<UObject> object, int32 idx) const override;
	virtual bool PasteSettings_Implementation(UFGFactoryClipboardSettings* factoryClipboard,
											  class AFGPlayerController* player) override;
	virtual void Server_DoFlush() override;
	//~ End AFGBuildableFactory Interface

	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	//~ End AActor Interface

	//~ Begin AFGBuildable Interface
	virtual void GetChildDismantleActors_Implementation(TArray<AActor*>& out_ChildDismantleActors) const override;
	//~ End AFGBuildable Interface

	//~ Begin AKPCLProducerBase Interface
	virtual void Overclocking_GetInfo_Implementation(FKPCLOverclockingProductionInfo& OutProductionInfo) override;
	virtual void
	Overclocking_GetProductionResults_Implementation(TArray<FKPCLOverclockingProductionResults>& OutIngredients,
													 TArray<FKPCLOverclockingProductionResults>& OutProducts) override;
	virtual bool Overclocking_IsConsumer_Implementation() override;
	virtual void SetPendingPotential(float newPendingPotential) override;
	virtual void UI_ApplyRelevantItems_Implementation(TArray<TSubclassOf<UFGItemDescriptor>>& OutSlots) override;
	//~ End AKPCLProducerBase Interface

	//~ Begin IKPCLModularBuildingInterface Interface
	virtual bool AttachedActor_Implementation(AFGBuildable* Actor,
											  TSubclassOf<UKAPIModularAttachmentDescriptor> Attachment,
											  FTransform Location, float Distance) override;
	virtual bool IsAllowedToSnap_Implementation(AFGBuildable* Actor) override;
	virtual void OnModulesWasUpdated_Implementation() override;
	virtual void POST_RemoveAttachedActor_Implementation() override;
	//~ End IKPCLModularBuildingInterface Interface

	//~ Begin AKLBuildableSlugBuildingBase Interface
	virtual void HandlePower(float dt) override;
	virtual void OnMeshUpdate() override;
	virtual void OnModulesUpdated_Implementation() override;
	//~ End AKLBuildableSlugBuildingBase Interface

	UFUNCTION(BlueprintNativeEvent, Category = "KMods|Hatching")
	void OnFluidFinial();

	UFUNCTION(BlueprintNativeEvent, Category = "KMods|Hatching")
	void OnIncubatorChanged(bool bHas);

	UFUNCTION(BlueprintNativeEvent, Category = "KMods|Hatching")
	void OnTankChanged(bool bHas);

	UFUNCTION(BlueprintNativeEvent, Category = "KMods|Hatching")
	void OnTimeModuleChanged(bool bHas);

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "KMods|Hatching")
	TArray<TSubclassOf<UFGItemDescriptor>> GetAllPossibleSlugs() const;

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "KMods|Hatching")
	TSubclassOf<UFGItemDescriptor> GetInventoryEgg() const;

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "KMods|Hatching")
	TSubclassOf<UFGItemDescriptor> GetFluidClass() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Hatching")
	EKAPISlugTime GetCurrentSlugTime() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Hatching")
	float GetChanceForSlug(TSubclassOf<UFGItemDescriptor> SlugClass) const;

	UFUNCTION(BlueprintPure, Category = "KMods|Hatching")
	float GetFluidProductionTime() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Hatching")
	float GetFluidRawProductionTime() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Hatching")
	FFullProductionHandle GetFluidHandle() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Hatching")
	UKAPISugHatchingData* GetHatchingData() const;

	UFUNCTION(BlueprintImplementableEvent, Category = "KMods|Events")
	void OnNewEgg(UKAPISugHatchingData* NewEgg, bool bSlugTypeChanged = false);

	UFUNCTION(NetMulticast, Reliable)
	void OnNewEgg_Native(UKAPISugHatchingData* NewEgg, bool bSlugTypeChanged = false);

	UFUNCTION()
	void OnDayTimeUpdated(bool isDayTime);

	void CheckEgg();
	void DoGatherStats();
	bool CanStoreSlugs() const;

	void SetHasIncubatorModule(bool bNewValue);
	void SetHasTankModule(bool bNewValue);
	void SetHasTimeModule(bool bNewValue);
	void SetOverwriteTime(EKAPISlugTime NewTime);
	void SetSlugTime(EKAPISlugTime NewTime);
	void SetHatching_Humidity(float NewHumidity);
	void SetHatching_Temperature(float NewTemperature);
	void CommitFluidProductionHandle();
	void CommitFixedChanceList();
	void SetCurrentHatchingData(UKAPISugHatchingData* NewData);
	bool CheckFeelingOfEgg() const;
	bool CheckModules() const;
	bool IsHatchingValid() const;
	float GetDefaultProductionCycleTime() const override;
	float GetPendingProductionTime() const override;
	float GetProductionTime() const override;
	float GetRawProductionTime() const;

	static constexpr int32 EGG_IDX = 0;
	static constexpr int32 FLUID_IDX = 1;

	UPROPERTY(EditDefaultsOnly, Category = "KMods|Hatching")
	TMap<EHatchingModuleType, int32> mModuleTypeLimit;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|Hatching")
	int32 mEggsToConsume = 1;

	UPROPERTY(EditDefaultsOnly, Category = "KMods|Hatching")
	float mToppingZOffset = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "KMods|Hatching")
	TObjectPtr<UKAPISugHatchingData> mDefaultHatchingData;

	UPROPERTY(SaveGame, BlueprintReadOnly, meta = (FGReplicated), Category = "KMods|Hatching")
	bool bHasIncubatorModule = false;

	UPROPERTY(SaveGame, BlueprintReadOnly, meta = (FGReplicated), Category = "KMods|Hatching")
	bool bHasTankModule = false;

	UPROPERTY(SaveGame, BlueprintReadOnly, meta = (FGReplicated), Category = "KMods|Hatching")
	bool bHasTimeModule = false;

	UPROPERTY(SaveGame, BlueprintReadOnly, meta = (FGReplicated), Category = "KMods|Hatching")
	EKAPISlugTime mOverwriteTime = EKAPISlugTime::NONE;

	UPROPERTY(SaveGame, BlueprintReadOnly, meta = (FGReplicated), Category = "KMods|Hatching")
	EKAPISlugTime mTime = EKAPISlugTime::Any;

	UPROPERTY(SaveGame, BlueprintReadOnly, meta = (FGReplicated), Category = "KMods|Hatching")
	float mHumidity = .5f;

	UPROPERTY(SaveGame, BlueprintReadOnly, meta = (FGReplicated), Category = "KMods|Hatching")
	float mTemperature = 25.0f;

	UPROPERTY(SaveGame, meta = (FGReplicated))
	FFullProductionHandle mFluidProductionHandle;

	UPROPERTY(SaveGame, meta = (FGReplicated))
	TArray<FKAPISlugFixedChance> mFixedChanceList;

	UFUNCTION()
	void OnRep_CurrentHatchingData();

	UPROPERTY(SaveGame, ReplicatedUsing = OnRep_CurrentHatchingData)
	TObjectPtr<UKAPISugHatchingData> mCurrentHatchingData;

	UPROPERTY(Transient)
	TArray<TSubclassOf<UFGItemDescriptor>> mEggFilter;

	UPROPERTY(Transient)
	TObjectPtr<AFGTimeOfDaySubsystem> mTimeSub;

	bool bGatherStatsPending = false;
};
