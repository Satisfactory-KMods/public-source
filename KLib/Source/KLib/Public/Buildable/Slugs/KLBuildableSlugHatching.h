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

	virtual bool Overclocking_IsConsumer_Implementation() override;
	virtual void UI_ApplyRelevantItems_Implementation(TArray<TSubclassOf<UFGItemDescriptor>>& OutSlots) override;
	virtual void Overclocking_GetInfo_Implementation(FKPCLOverclockingProductionInfo& OutProductionInfo) override;
	virtual void Overclocking_GetProductionResults_Implementation(
		TArray<FKPCLOverclockingProductionResults>& OutIngredients,
		TArray<FKPCLOverclockingProductionResults>& OutProducts) override;

	// Repl
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void GetConditionalReplicatedProps(TArray<FFGCondReplicatedProperty>& outProps) const override;

	virtual UFGFactoryClipboardSettings* CopySettings_Implementation() override;
	virtual bool PasteSettings_Implementation(UFGFactoryClipboardSettings* factoryClipboard,
	                                          class AFGPlayerController* player) override;
	virtual bool CanUseFactoryClipboard_Implementation() override;

	virtual void SetPendingPotential(float newPendingPotential) override;

	virtual void BeginPlay() override;
	virtual void Factory_Tick(float dt) override;

	virtual void CollectBelts() override;
	virtual void CollectAndPushPipes(float dt, bool IsPush) override;
	virtual void HandlePower(float dt) override;

	// Start IKPCLModularBuildingInterface
	virtual bool IsAllowedToSnap_Implementation(AFGBuildable* Actor) override;
	virtual void POST_RemoveAttachedActor_Implementation() override;
	virtual void OnModulesWasUpdated_Implementation() override;
	virtual bool AttachedActor_Implementation(AFGBuildable* Actor,
	                                          TSubclassOf<UKAPIModularAttachmentDescriptor> Attachment,
	                                          FTransform Location, float Distance) override;
	// End IKPCLModularBuildingInterface

	// Start Can Produce
	virtual bool CanProduce_Implementation() const override;
	bool CheckFeelingOfEgg() const;
	bool CheckModules() const;
	bool CanStoreSlugs() const;
	bool IsHatchingValid() const;
	// End Can Produce

	// Start Production
	virtual float GetDefaultProductionCycleTime() const override;
	virtual float GetPendingProductionTime() const override;
	float GetRawProductionTime() const;
	virtual float GetProductionTime() const override;

	virtual void onProducingFinal_Implementation() override;

	/** Overwrite for more stats
	* ExtraState_1 : Modules are not correct!
	* ExtraState_1 : Feeling of Egg is invalid!
	**/
	virtual void HandleIndicator() override;
	virtual void Server_DoFlush() override;
	// End Production

	virtual void GetChildDismantleActors_Implementation(TArray<AActor*>& out_ChildDismantleActors) const override;

	// Filter Inventorys to be Slug Classes!
	virtual bool FilterOutputInventory(TSubclassOf<UObject> object, int32 idx) const override;
	virtual bool FilterInputInventory(TSubclassOf<UObject> object, int32 idx) const override;

	// Check is a new Egg in the Hatcher
	void CheckEgg();

	// Called on All but only send from Server;
	virtual void OnMeshUpdate() override;

	// Events
	virtual void OnModulesUpdated_Implementation() override;

	UFUNCTION(BlueprintPure, BlueprintCallable, Category="KMods|Hatching")
	TSubclassOf<UFGItemDescriptor> GetFluidClass() const;

	UFUNCTION(BlueprintPure, BlueprintCallable, Category="KMods|Hatching")
	TSubclassOf<UFGItemDescriptor> GetInventoryEgg() const;

	UFUNCTION(BlueprintPure, BlueprintCallable, Category="KMods|Hatching")
	TArray<TSubclassOf<UFGItemDescriptor>> GetAllPossibleSlugs() const;

	UFUNCTION()
	void OnDayTimeUpdated(bool isDayTime);

	//Module Functions
	void DoGatherStats();

	/** Events */
	UFUNCTION(BlueprintNativeEvent, Category="KMods|Hatching")
	void OnTankChanged(bool bHas);

	UFUNCTION(BlueprintNativeEvent, Category="KMods|Hatching")
	void OnIncubatorChanged(bool bHas);

	UFUNCTION(BlueprintNativeEvent, Category="KMods|Hatching")
	void OnTimeModuleChanged(bool bHas);

	UFUNCTION(BlueprintNativeEvent, Category="KMods|Hatching")
	void OnFluidFinial();

	/** GetCurrentEggHatching Class */
	UFUNCTION(BlueprintPure, Category="KMods|Hatching")
	UKAPISugHatchingData* GetHatchingData() const;

	UFUNCTION(BlueprintPure, Category="KMods|Hatching")
	EKAPISlugTime GetCurrentSlugTime() const;

	UFUNCTION(BlueprintPure, Category="KMods|Hatching")
	float GetChanceForSlug(TSubclassOf<UFGItemDescriptor> SlugClass) const;

	/** Multicast on New Egg > Called BP OnNewEgg */
	UFUNCTION(NetMulticast, Reliable)
	void OnNewEgg_Native(UKAPISugHatchingData* NewEgg, bool bSlugTypeChanged = false);

	/** Called on Server & Host if a new Egg was found */
	UFUNCTION(BlueprintImplementableEvent, Category="KMods|Events")
	void OnNewEgg(UKAPISugHatchingData* NewEgg, bool bSlugTypeChanged = false);

private:
	UPROPERTY(SaveGame, Replicated)
	UKAPISugHatchingData* mCurrentHatchingData;

	UPROPERTY(SaveGame)
	TMap<TSubclassOf<UFGItemDescriptor>, float> mFixedChanceMap;

	UPROPERTY(Transient)
	AFGTimeOfDaySubsystem* mTimeSub;

	UPROPERTY(Transient)
	TArray<TSubclassOf<UFGItemDescriptor>> mEggFilter;

	// Consts
	int32 FLUID_IDX = 1;
	int32 EGG_IDX = 0;

public:
	UPROPERTY(EditDefaultsOnly, Category="KMods|Hatching")
	UKAPISugHatchingData* mDefaultHatchingData;

	UPROPERTY(EditDefaultsOnly, Category="KMods|Hatching")
	float mToppingZOffset = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="KMods|Hatching")
	int32 mEggsToConsume = 1;

	UPROPERTY(EditDefaultsOnly, Category="KMods|Hatching")
	TMap<EHatchingModuleType, int32> mModuleTypeLimit;

	UPROPERTY(SaveGame, BlueprintReadOnly, meta = ( FGReplicated ), Category="KMods|Hatching")
	float mTemperature = 25.0f;

	UPROPERTY(SaveGame, BlueprintReadOnly, meta = ( FGReplicated ), Category="KMods|Hatching")
	float mHumidity = .5f;

	UPROPERTY(SaveGame, BlueprintReadOnly, meta = ( FGReplicated ), Category="KMods|Hatching")
	bool bHasTankModule = false;

	UPROPERTY(SaveGame, BlueprintReadOnly, meta = ( FGReplicated ), Category="KMods|Hatching")
	bool bHasIncubatorModule = false;

	UPROPERTY(SaveGame, BlueprintReadOnly, meta = ( FGReplicated ), Category="KMods|Hatching")
	bool bHasTimeModule = false;

	UPROPERTY(SaveGame, BlueprintReadOnly, meta = ( FGReplicated ), Category="KMods|Hatching")
	EKAPISlugTime mTime = EKAPISlugTime::Any;

	UPROPERTY(SaveGame, BlueprintReadOnly, meta = ( FGReplicated ), Category="KMods|Hatching")
	EKAPISlugTime mOverwriteTime = EKAPISlugTime::NONE;

	UPROPERTY(meta = ( FGReplicated ), SaveGame)
	FFullProductionHandle mFluidProductionHandle;
};
