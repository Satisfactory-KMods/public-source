// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "FGActorRepresentationInterface.h"

#include "Buildable/KPCLProducerBase.h"
#include "Buildable/Modular/KPCLModularBuildingBase.h"
#include "Interfaces/KPCLNetworkDataInterface.h"

#include "KPCLNetworkBuildingBase.generated.h"

USTRUCT(BlueprintType)
struct FKPCLItemTransferQueue
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool mAddAmount = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FItemAmount mAmount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 mInventoryIndex = 0;

	bool IsValid() const { return mAmount.Amount > 0 && mInventoryIndex > INDEX_NONE && mAmount.ItemClass != nullptr; }
};

USTRUCT(BlueprintType)
struct FKPCLNetworkDistanceModifier
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float mSafeDistance = 25000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float mEveryMeter = 2000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float mDefaultValue = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float mValue = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float mMaxValue = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bNegative = false;

	float GetValue(float Distance) const;
	float GetValue(AActor* ActorA, AActor* ActorB) const;
};

USTRUCT(BlueprintType)
struct FKPCLSinkQueue
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 mIndex = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FItemAmount mAmount;
};

UCLASS()
class KPRIVATECODELIB_API UKPCLFaxitBasicClipboardSettings : public UFGFactoryClipboardSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite)
	bool bIsPaused;

	UPROPERTY(BlueprintReadWrite)
	FString mNetworkId = FString();

	UPROPERTY(BlueprintReadWrite)
	FString mConnectedNetworkId = FString();
};

UCLASS()
class KPRIVATECODELIB_API AKPCLNetworkBuildingBase : public AKPCLModularBuildingBase,
													 public IFGActorRepresentationInterface,
													 public IKPCLNetworkDataInterface
{
	GENERATED_BODY()

public:
	AKPCLNetworkBuildingBase();

	virtual bool Overclocking_ShouldUse_Implementation() override;
	virtual UFGFactoryClipboardSettings* CopySettings_Implementation() override;
	virtual bool PasteSettings_Implementation(UFGFactoryClipboardSettings* factoryClipboard,
											  class AFGPlayerController* player) override;
	virtual void UI_ApplyRelevantItems_Implementation(TArray<TSubclassOf<UFGItemDescriptor>>& OutSlots) override;
	virtual bool CanUseFactoryClipboard_Implementation() override;

	//~ Begin IFGActorRepresentationInterface
	UFUNCTION()
	virtual bool AddAsRepresentation() override;
	UFUNCTION()
	virtual bool UpdateRepresentation() override;
	UFUNCTION()
	virtual bool RemoveAsRepresentation() override;
	UFUNCTION()
	virtual bool IsActorStatic() override;
	UFUNCTION()
	virtual FVector GetRealActorLocation() override;
	UFUNCTION()
	virtual FRotator GetRealActorRotation() override;
	UFUNCTION()
	virtual class UTexture2D* GetActorRepresentationTexture() override;
	UFUNCTION()
	virtual FText GetActorRepresentationText() override;
	UFUNCTION()
	virtual void SetActorRepresentationText(const FText& newText) override;
	UFUNCTION()
	virtual FLinearColor GetActorRepresentationColor() override;
	UFUNCTION()
	virtual void SetActorRepresentationColor(FLinearColor newColor) override;
	UFUNCTION()
	virtual ERepresentationType GetActorRepresentationType() override;
	UFUNCTION()
	virtual bool GetActorShouldShowInCompass() override;
	UFUNCTION()
	virtual bool GetActorShouldShowOnMap() override;
	UFUNCTION()
	virtual EFogOfWarRevealType GetActorFogOfWarRevealType() override;
	UFUNCTION()
	virtual float GetActorFogOfWarRevealRadius() override;
	UFUNCTION()
	virtual ECompassViewDistance GetActorCompassViewDistance() override;
	UFUNCTION()
	virtual void SetActorCompassViewDistance(ECompassViewDistance compassViewDistance) override;
	UFUNCTION()
	virtual UMaterialInterface* GetActorRepresentationCompassMaterial() override;
	UFUNCTION()
	virtual FPlayerInfoHandle GetLastEditedBy() const override;
	UFUNCTION()
	virtual void SetActorLastEditedByHandle(const FPlayerInfoHandle& playerInfoHandle) override;
	//~ End IFGActorRepresentationInterface

	//~ Begin IKPCLNetworkDataInterface
	virtual bool HasCore_Implementation() const override;
	virtual AKPCLNetworkCore* GetCore_Implementation() override;
	virtual UKPCLNetwork* GetNetwork_Implementation() const override;
	virtual FNetworkUIData GetUIDData_Implementation() const override;
	virtual FKPCLFaxitNetwork GetNetworkData_Implementation() const override;
	virtual TArray<EKPCLNetworkError> GetNetworkErrors_Implementation() const override;
	virtual bool HasCoreInNetwork_Implementation() const override;
	void SetNetworkCore(AKPCLNetworkCore* Core, FKPCLFaxitNetwork* Network);
	virtual bool Factory_HasFaxitCableConnection() const;

	virtual void HandlePower(float dt) override;
	virtual void UpdateDistanceToCore();

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "KMods|Network")
	virtual bool HasCableBoost() const;

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "KMods|Network")
	virtual bool HasFaxitCore() const;

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "KMods|Network")
	virtual const AKPCLNetworkCore* GetFaxitCoreConst() const;

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "KMods|Network")
	virtual AKPCLNetworkCore* GetFaxitCore();

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "KMods|Network")
	virtual UKPCLNetwork* GetFaxitCableNetwork() const;

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "KMods|Network")
	virtual FKPCLFaxitNetwork GetFaxitNetwork() const;

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "KMods|Network")
	virtual FString GetNetworkId() const;

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "KMods|Network")
	bool GetCableNetworkHasToManyCores() const;

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "KMods|Network")
	virtual TArray<EKPCLNetworkError> GetNetworkErrorCodes() const;

	UFUNCTION(BlueprintCallable, Category = "KMods|Network")
	void TryToConnectToNearstCore();

	UFUNCTION(BlueprintPure, Category = "KMods|Network")
	virtual float GetRealPowerConsume() const;

	virtual void OnNetworkDestoryed_Internal();
	virtual void OnNetworkAdded_Internal(AKPCLNetworkCore* Core);
	//~ End IKPCLNetworkDataInterface

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Factory_Tick(float dt) override;
	virtual bool Factory_HasPower() const override;
	virtual bool Factory_IsProducing() const override;
	virtual void GetConditionalReplicatedProps(TArray<FFGCondReplicatedProperty>& outProps) const override;
	virtual float GetPowerConsume() const override;

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "KMods|Network")
	virtual float GetPowerConsumeOnDistance() const;

	int32 SinkItems(FItemAmount Items, int32 MaxToSink = INT32_MAX);

	virtual void RegisterInteractingPlayer_Implementation(AFGCharacterPlayer* player) override;
	virtual void UnregisterInteractingPlayer_Implementation(AFGCharacterPlayer* player) override;

	virtual bool CanProduce_Implementation() const override;

	UFUNCTION()
	virtual void OnCircuitChanged(UFGCircuitConnectionComponent* Component);

	UFUNCTION(BlueprintPure, Category = "KMods|Network")
	virtual bool IsCore() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Network")
	float GetCoreDistance() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Network")
	virtual class UKPCLNetworkInfoComponent* GetNetworkInfoComponent() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Network")
	virtual class UKPCLNetworkConnectionComponent* GetNetworkConnectionComponent() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Network")
	virtual UFGPowerInfoComponent* GetPowerInfoExplicit() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Network")
	virtual UFGPowerConnectionComponent* GetPowerConnectionExplicit() const;

	//~ Begin AActor
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ End AActor

	void AddDownloadToStats(TSubclassOf<UFGItemDescriptor> Item, int32 Amount);
	void AddUploadToStats(TSubclassOf<UFGItemDescriptor> Item, int32 Amount);

	UPROPERTY(EditDefaultsOnly, Category = "KMods|UI")
	FNetworkUIData mNetworkUIData;

	/**
	 * Runtime distance (in cm) from this building to the nearest Faxit core access point.
	 * Updated every 5 s by mDistanceChecker. Replicated so clients can display accurate
	 * distance-based power consumption.
	 */
	UPROPERTY(SaveGame, BlueprintReadOnly, meta = (FGReplicated), Category = "KMods|NetworkPower")
	float mDistanceToNetworkCore = 0.f;

	UPROPERTY(SaveGame, BlueprintReadOnly, meta = (FGReplicated))
	bool bHasCableBoost = false;

protected:
	friend struct FKPCLFaxitNetwork;
	friend class AKPCLFaxitSubsystem;
	friend class AKPCLNetworkCore;

	/** Setters for FGReplicated properties — each centralises the MarkPropertyDirty call. */
	void SetHasCableBoost(bool NewValue);
	void SetNetworkIdDirect(const FString& NewId);
	void SetConnectedNetworkIdDirect(const FString& NewId);
	/** Setter for mDistanceToNetworkCore — marks the FGReplicated property dirty. */
	void SetDistanceToNetworkCore(float NewDist);

	virtual class AFGResourceSinkSubsystem* GetSinkSub();
	bool bBindNetworkComponent = false;

	UPROPERTY()
	TObjectPtr<class UKPCLNetworkConnectionComponent> mNetworkConnection;

	UPROPERTY()
	TObjectPtr<class UKPCLNetworkInfoComponent> mNetworkInfoComponent;

	UPROPERTY(Transient)
	TObjectPtr<class AKPCLFaxitSubsystem> mFaxitSubsystem = nullptr;

	/**
	 * Per-cycle upload/download statistics for this building.
	 * Accessed from the factory worker thread (AddDownloadToStats / AddUploadToStats)
	 * and from the game thread (GetAndResetStats via SaveStateBundle).
	 * Protected by mStatsMutex on all access paths.
	 */
	UPROPERTY(SaveGame)
	TArray<FKPCLFaxitNetworkStatData> mCurrentStates;

	UPROPERTY(SaveGame, meta = (FGReplicated))
	FString mNetworkId = FString();

	UPROPERTY(SaveGame, meta = (FGReplicated))
	FString mConnectedNetworkId = FString();

private:
	FKPCLFaxitNetworkStatData* GetState(TSubclassOf<UFGItemDescriptor> Item);
	TArray<FKPCLFaxitNetworkStatData> GetAndResetStats();

	UPROPERTY(EditDefaultsOnly, Category = "KMods|Faxit")
	FSmartTimer mDistanceChecker = FSmartTimer(5.0f, true);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|Faxit", meta = (AllowPrivateAccess = "true"))
	FKPCLNetworkDistanceModifier mDistancePowerModifier;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "KMods|Represenation",
			  meta = (AllowPrivateAccess = "true"))
	bool bRepresentationEnabled = false;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "KMods|Represenation",
			  meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTexture2D> mRepresentationIcon = nullptr;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "KMods|Represenation",
			  meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMaterialInterface> mRepresentationMaterial = nullptr;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "KMods|Represenation",
			  meta = (AllowPrivateAccess = "true"))
	FLinearColor mRepresentationColor = FLinearColor();

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "KMods|Represenation",
			  meta = (AllowPrivateAccess = "true"))
	ECompassViewDistance mCompassViewDistance = ECompassViewDistance::CVD_Mid;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "KMods|Represenation",
			  meta = (AllowPrivateAccess = "true"))
	float mFogOfWarRevealRadius = 2000.f;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "KMods|Represenation",
			  meta = (AllowPrivateAccess = "true"))
	bool bShouldShowInCompass = true;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "KMods|Represenation",
			  meta = (AllowPrivateAccess = "true"))
	bool bShouldShowOnMap = true;

	/**
	 * Guards mCurrentStates against concurrent access.
	 * AddDownloadToStats / AddUploadToStats / GetState run on the factory worker thread;
	 * GetAndResetStats runs on the game thread (called from SaveStateBundle).
	 */
	mutable FCriticalSection mStatsMutex;
};
