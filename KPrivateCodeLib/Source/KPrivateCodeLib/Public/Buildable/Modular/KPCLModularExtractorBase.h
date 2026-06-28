// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "Buildable/KPCLExtractorBase.h"
#include "Buildable/KPCLProducerBase.h"
#include "FGInventoryComponent.h"
#include "KPCLModularBuildingBase.h"
#include "KPCLModularBuildingHandler.h"
#include "KPCLModularBuildingInterface.h"

#include "KPCLModularExtractorBase.generated.h"

UCLASS(Abstract)
class KPRIVATECODELIB_API AKPCLModularExtractorBase : public AKPCLExtractorBase, public IKPCLModularBuildingInterface
{
	GENERATED_BODY()

public:
	AKPCLModularExtractorBase();

	//~ Begin IFGUseableInterface
	virtual FText GetLookAtDecription_Implementation(class AFGCharacterPlayer* byCharacter,
													 const FUseState& state) const override;
	virtual bool IsUseable_Implementation() const override;
	virtual void OnUseStop_Implementation(AFGCharacterPlayer* byCharacter, const FUseState& state) override;
	virtual void OnUse_Implementation(AFGCharacterPlayer* byCharacter, const FUseState& state) override;
	//~ End IFGUseableInterface

	//~ Begin IKPCLModularBuildingInterface
	virtual bool IsAllowedToSnap_Implementation(AFGBuildable* Actor) override { return true; };
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
	//~ End IKPCLModularBuildingInterface

	//~ Begin AActor Interface
	virtual void GetChildDismantleActors_Implementation(TArray<AActor*>& out_ChildDismantleActors) const override;
	virtual void PostInitializeComponents() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End AActor Interface

	virtual void PreSaveGame_Implementation(int32 saveVersion, int32 gameVersion) override;
	virtual void OnBuildEffectFinished() override;

	UFUNCTION()
	virtual void OnRep_MasterBuilding();

	UFUNCTION()
	virtual void OnRep_ModularIndex();

	/** Called when a master building is assigned to this buildable. */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Km")
	void OnMasterBuildingReceived(AActor* Actor);

	virtual void OnMasterBuildingReceived_Implementation(AActor* Actor);

	/** Called when a module is added or removed. */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Modular Building Events")
	void OnModulesWasUpdated();

	/** Triggered after a module was received or removed. */
	UFUNCTION()
	virtual void OnMeshUpdate();

	/** Triggered after a module was received or removed. */
	UFUNCTION(BlueprintImplementableEvent)
	void Event_OnMeshUpdate();

	/** Helper Function to get Handler casted. */
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "KMods|Modular Building", meta = (BlueprintAutocast))
	UKPCLModularBuildingHandlerBase* GetHandler() const;

	template <class T>
	T* GetHandler() const;

	/** Helper Function to get Master Building casted. */
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "KMods|Modular Building")
	AFGBuildable* GetMasterBuildable() const;

	template <class T>
	T* GetMasterBuildable() const;

	UPROPERTY(SaveGame)
	bool mCanHaveBuildabled = false;

	UPROPERTY(BlueprintAssignable, Category = "Modular Building Events")
	FOnModulesUpdated OnModulesUpdated;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MasterBuilding, Category = "KMods|Modular Building")
	TWeakObjectPtr<AFGBuildable> mMasterBuilding;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "KMods|Modular Building")
	TObjectPtr<UKPCLModularBuildingHandlerBase> mModularHandler;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|Modular Building")
	bool mShouldUseUiFromMaster = false;

	UPROPERTY(SaveGame, ReplicatedUsing = OnRep_ModularIndex)
	int32 mModularIndex = -1;

	UPROPERTY(EditDefaultsOnly, Category = "KMods|Debug")
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
