// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "FGInventoryComponent.h"
#include "KPCLModularBuildingBase.h"
#include "KPCLModularBuildingHandler.h"
#include "KPCLModularBuildingInterface.h"
#include "Buildable/KPCLExtractorBase.h"
#include "Buildable/KPCLProducerBase.h"

#include "KPCLModularExtractorBase.generated.h"

/**
 * 
 */
UCLASS(Abstract)
class KPRIVATECODELIB_API AKPCLModularExtractorBase : public AKPCLExtractorBase, public IKPCLModularBuildingInterface
{
	GENERATED_BODY()

public:
	AKPCLModularExtractorBase();

	// Begin IFGUseableInterface
	virtual FText GetLookAtDecription_Implementation(class AFGCharacterPlayer* byCharacter,
	                                                 const FUseState& state) const override;

	virtual bool IsUseable_Implementation() const override;

	virtual void OnUseStop_Implementation(AFGCharacterPlayer* byCharacter, const FUseState& state) override;

	virtual void OnUse_Implementation(AFGCharacterPlayer* byCharacter, const FUseState& state) override;

	// End IFGUseableInterface

	virtual void PreSaveGame_Implementation(int32 saveVersion, int32 gameVersion) override;;

	virtual void OnBuildEffectFinished() override;;

	UPROPERTY(SaveGame)
	bool mCanHaveBuildabled = false;

	// Begin IKPCLModularBuildingInterface
	virtual bool GetCanHaveModules_Implementation() override;

	virtual AFGBuildable* GetMasterBuilding_Implementation() override;

	virtual FPowerOptions GetPowerOptions_Implementation() override;

	virtual UKPCLModularBuildingHandlerBase* GetModularHandler_Implementation() override;

	virtual void SetMasterBuilding_Implementation(AFGBuildable* Actor) override;

	virtual int32 GetModularIndex_Implementation() override;

	virtual void ApplyModularIndex_Implementation(int32 Index) override;

	virtual void SetAttachedActor_Implementation(AFGBuildable* Actor) override;

	virtual void RemoveAttachedActor_Implementation(AFGBuildable* Actor) override;

	virtual bool AttachedActor_Implementation(AFGBuildable* Actor,
	                                          TSubclassOf<UKAPIModularAttachmentDescriptor> Attachment,
	                                          FTransform Location, float Distance) override;

	virtual void OnModulesUpdated_Implementation() override;

	virtual void ReadyForVisuelUpdate() override;

	// End IKPCLModularBuildingInterface

	/** ----- Overwrite ----- */
	// Begin AActor
	virtual bool IsAllowedToSnap_Implementation(AFGBuildable* Actor) override { return true; };
	virtual void GetChildDismantleActors_Implementation(TArray<AActor*>& out_ChildDismantleActors) const override;

	virtual void PostInitializeComponents() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Gather Components directly after Initialize */
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// End AActor

	/** ------ Events ------ */
	/** Called on Set new Actor to Buildable */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Km")
	void OnMasterBuildingReceived(AActor* Actor);

	virtual void OnMasterBuildingReceived_Implementation(AActor* Actor);

	/** Called on Add new Actor to Buildable */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Modular Building Events")
	void OnModulesWasUpdated();

	UPROPERTY(BlueprintAssignable, Category = "Modular Building Events")
	FOnModulesUpdated OnModulesUpdated;

	/** Trigger after a Module was recieved or removed */
	virtual void OnMeshUpdate();;

	/** Trigger after a Module was recieved or removed */
	UFUNCTION(BlueprintImplementableEvent)
	void Event_OnMeshUpdate();

	/** ------ Modular Building ------ */
	UPROPERTY(BlueprintReadOnly, Replicated, Category="KMods|Modular Building")
	TWeakObjectPtr<AFGBuildable> mMasterBuilding;

	UPROPERTY(BlueprintReadOnly, Replicated, Category="KMods|Modular Building")
	UKPCLModularBuildingHandlerBase* mModularHandler;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="KMods|Modular Building")
	bool mShouldUseUiFromMaster = false;

	/** Helper Function to get Handler casted */
	UFUNCTION(BlueprintPure, BlueprintCallable, Category="KMods|Modular Building", meta=(BlueprintAutocast))
	UKPCLModularBuildingHandlerBase* GetHandler() const;

	template <class T>
	T* GetHandler() const;

	/** Helper Function to get Master Building casted */
	UFUNCTION(BlueprintPure, BlueprintCallable, Category="KMods|Modular Building")
	AFGBuildable* GetMasterBuildable() const;

	template <class T>
	T* GetMasterBuildable() const;

	/** ----- Components ----- */
	UPROPERTY(SaveGame, Replicated)
	int32 mModularIndex = -1;

	UPROPERTY(EditDefaultsOnly, Category="KMods|Debug")
	bool mDismantleAllChilds = false;
};

template <class T>
T* AKPCLModularExtractorBase::GetHandler() const
{
	return Cast<T>(GetHandler());
}

template <class T>
T* AKPCLModularExtractorBase::GetMasterBuildable() const
{
	return Cast<T>(GetMasterBuildable());
}
