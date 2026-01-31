// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "FGInventoryComponent.h"
#include "KPCLModularBuildingHandler.h"
#include "KPCLModularBuildingInterface.h"
#include "Buildable/KPCLProducerBase.h"

#include "KPCLModularBuildingBase.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnModulesUpdated);

/**
 * 
 */
UCLASS(Abstract)
class KPRIVATECODELIB_API AKPCLModularBuildingBase : public AKPCLProducerBase, public IKPCLModularBuildingInterface
{
	GENERATED_BODY()

public:
	AKPCLModularBuildingBase();

	// Begin IFGUseableInterface
	virtual FText GetLookAtDecription_Implementation(class AFGCharacterPlayer* byCharacter,
	                                                 const FUseState& state) const override;

	virtual bool IsUseable_Implementation() const override;

	virtual void OnUseStop_Implementation(AFGCharacterPlayer* byCharacter, const FUseState& state) override;

	virtual void OnUse_Implementation(AFGCharacterPlayer* byCharacter, const FUseState& state) override;

	virtual UFGFactoryClipboardSettings* CopySettings_Implementation() override;
	virtual bool PasteSettings_Implementation(UFGFactoryClipboardSettings* factoryClipboard,
	                                          class AFGPlayerController* player) override;

	// End IFGUseableInterface

	virtual void PreSaveGame_Implementation(int32 saveVersion, int32 gameVersion) override
	{
		Super::PreSaveGame_Implementation(saveVersion, gameVersion);
		mCanHaveBuildabled = true;
	};

	virtual void OnBuildEffectFinished() override
	{
		Super::OnBuildEffectFinished();
		mCanHaveBuildabled = true;
	};

	UPROPERTY(SaveGame)
	bool mCanHaveBuildabled = false;

	// Begin IKPCLModularBuildingInterface
	virtual bool IsAllowedToSnap_Implementation(AFGBuildable* Actor) override { return true; };
	FORCEINLINE virtual bool GetCanHaveModules_Implementation() override
	{
		return (mCanHaveBuildabled || !IsPlayingBuildEffect()) && IsValid(mModularHandler);
	}

	FORCEINLINE virtual AFGBuildable* GetMasterBuilding_Implementation() override
	{
		if (mMasterBuilding.IsValid())
		{
			return mMasterBuilding.Get();
		}
		return nullptr;
	}

	FORCEINLINE virtual FPowerOptions GetPowerOptions_Implementation() override
	{
		return mPowerOptions;
	}

	FORCEINLINE virtual UKPCLModularBuildingHandlerBase* GetModularHandler_Implementation() override
	{
		return mModularHandler;
	}

	FORCEINLINE virtual void SetMasterBuilding_Implementation(AFGBuildable* Actor) override
	{
		mMasterBuilding = Actor;
		OnMasterBuildingReceived(Actor);
	}

	virtual TSubclassOf<UKAPIModularAttachmentDescriptor> GetModularAttachmentClass_Implementation() override;

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

	FORCEINLINE virtual void OnMasterBuildingReceived_Implementation(AActor* Actor)
	{
	}

	/** Called on Add new Actor to Buildable */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Modular Building Events")
	void OnModulesWasUpdated();

	UPROPERTY(BlueprintAssignable, Category = "Modular Building Events")
	FOnModulesUpdated OnModulesUpdated;

	/** Trigger after a Module was recieved or removed */
	virtual void OnMeshUpdate()
	{
	};

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="KMods|Modular Building")
	TSubclassOf<UKAPIModularAttachmentDescriptor> mUpgradeClass;

	/** Helper Function to get Handler casted */
	UFUNCTION(BlueprintPure, BlueprintCallable, Category="KMods|Modular Building", meta=(BlueprintAutocast))
	UKPCLModularBuildingHandlerBase* GetHandler() const
	{
		return mModularHandler;
	}

	template <class T>
	FORCEINLINE T* GetHandler() const
	{
		return Cast<T>(GetHandler());
	}

	/** Helper Function to get Master Building casted */
	UFUNCTION(BlueprintPure, BlueprintCallable, Category="KMods|Modular Building")
	AFGBuildable* GetMasterBuildable() const
	{
		if (mMasterBuilding.IsValid())
		{
			return mMasterBuilding.Get();
		}
		return nullptr;
	}

	template <class T>
	FORCEINLINE T* GetMasterBuildable() const
	{
		return Cast<T>(GetMasterBuildable());
	}

	/** ----- Components ----- */
	UPROPERTY(SaveGame, Replicated)
	int32 mModularIndex = -1;

	UPROPERTY(EditDefaultsOnly, Category="KMods|Debug")
	bool mDismantleAllChilds = false;
};
