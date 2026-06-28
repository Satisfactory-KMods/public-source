// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "FGColoredInstanceMeshProxy.h"
#include "FGCrate.h"
#include "FGDestructibleActor.h"
#include "FGDismantleInterface.h"
#include "FGItemPickup.h"
#include "FGSignificanceInterface.h"
#include "ItemAmount.h"
#include "Kismet/KismetMathLibrary.h"

#include "KPCLLootChest.generated.h"

USTRUCT(BlueprintType)
struct FKPCLRange
{
	GENERATED_BODY()

	FKPCLRange() {}

	FKPCLRange(int32 A, int32 B)
	{
		mMin = A;
		mMax = B;
	}

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 mMin = 5;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 mMax = 15;

	int32 GetRandom() const { return UKismetMathLibrary::RandomIntegerInRange(mMin, mMax); }
};

USTRUCT(BlueprintType)
struct FKPCLLootChestRandomData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSubclassOf<UFGItemDescriptor> mItemClass;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FKPCLRange mAmountRange = FKPCLRange(5, 10);
};

UCLASS(BlueprintType)
class KPRIVATECODELIB_API UKPCLScannableDetailsLootChest : public UFGScannableDetails
{
	GENERATED_BODY()

public:
	virtual FScannableActorDetails FindClosestRelevantActor(class UWorld* world, const FVector& scanLocation,
															const float maxRangeSquare,
															TSubclassOf<AActor> actorClassToScanFor) const override;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLootedUpdated, bool, Looted);

UCLASS()
class KPRIVATECODELIB_API AKPCLLootChest : public AFGDestructibleActor,
										   public IFGUseableInterface,
										   public IFGDismantleInterface
{
	GENERATED_BODY()

public:
	AKPCLLootChest();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool ShouldSave_Implementation() const override;
	virtual float TakeDamage(float DamageAmount, const struct FDamageEvent& DamageEvent,
							 class AController* EventInstigator, AActor* DamageCauser) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	//~ Begin IFGUseableInterface
	virtual void UpdateUseState_Implementation(class AFGCharacterPlayer* byCharacter, const FVector& atLocation,
											   class UPrimitiveComponent* componentHit,
											   FUseState& out_useState) override;
	virtual void OnUse_Implementation(class AFGCharacterPlayer* byCharacter, const FUseState& state) override;
	virtual void OnUseStop_Implementation(class AFGCharacterPlayer* byCharacter, const FUseState& state) override;
	virtual bool IsUseable_Implementation() const override;
	virtual void StartIsLookedAt_Implementation(AFGCharacterPlayer* byCharacter, const FUseState& state) override;
	virtual void StopIsLookedAt_Implementation(AFGCharacterPlayer* byCharacter, const FUseState& state) override;
	virtual FText GetLookAtDecription_Implementation(class AFGCharacterPlayer* byCharacter,
													 const FUseState& state) const override;
	virtual void RegisterInteractingPlayer_Implementation(class AFGCharacterPlayer* player) override;
	virtual void UnregisterInteractingPlayer_Implementation(class AFGCharacterPlayer* player) override;
	//~ End IFGUseableInterface

	//~ Begin IFGDismantleInterface
	virtual bool CanDismantle_Implementation() const override;
	virtual void GetDismantleRefund_Implementation(TArray<FInventoryStack>& out_refund,
												   bool noBuildCostEnabled) const override;
	virtual FVector GetRefundSpawnLocationAndArea_Implementation(const FVector& aimHitLocation,
																 float& out_radius) const override;
	virtual void PreUpgrade_Implementation() override;
	virtual void Upgrade_Implementation(AActor* newActor) override;
	virtual void Dismantle_Implementation() override;
	virtual void StartIsLookedAtForDismantle_Implementation(class AFGCharacterPlayer* byCharacter) override;
	virtual void StopIsLookedAtForDismantle_Implementation(class AFGCharacterPlayer* byCharacter) override;
	virtual void GetChildDismantleActors_Implementation(TArray<AActor*>& out_ChildDismantleActors) const override;
	virtual FText GetDismantleDisplayName_Implementation(AFGCharacterPlayer* byCharacter) const override;
	virtual void GetDismantleDependencies_Implementation(TArray<AActor*>& out_dismantleDependencies) const override;
	//~ End IFGDismantleInterface

	void GenerateLoot();
	void UpdateActorState(bool IsEmpty);
	void SetEmpty(bool NewIsEmpty);

	UFUNCTION(BlueprintPure, Category = "KMods|LootChest")
	bool WasLooted() const;

	UFUNCTION(BlueprintPure, Category = "KMods|LootChest")
	UFGInventoryComponent* GetInventory() const;

	UFUNCTION(BlueprintCallable, NetMulticast, Reliable)
	void AnnounceActorStateUpdate(bool IsEmpty);

	UFUNCTION(BlueprintCallable, Category = "KMods|LootChest")
	void Loot(AFGCharacterPlayer* Player);

	UFUNCTION(BlueprintImplementableEvent, Category = "KMods|LootChest")
	void OnLootedUpdated(bool Looted);

	UFUNCTION()
	void OnRep_IsEmpty();

	UPROPERTY(BlueprintAssignable, Category = "KMods|LootChest")
	FOnLootedUpdated OnLootedChanged;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "KMods|Interaction")
	FText mDisplayName;

	UPROPERTY(EditDefaultsOnly, Category = "KMods|Interaction", meta = (DisplayName = "Interact Widget Class"))
	TSoftClassPtr<class UFGInteractWidget> mInteractWidgetSoftClass;

	UPROPERTY(EditDefaultsOnly, Replicated, SaveGame, Category = "KMods|Inventory")
	TObjectPtr<UFGInventoryComponent> mInventory;

	UPROPERTY(EditAnywhere, Category = "KMods|LootChest")
	TArray<FKPCLLootChestRandomData> mRandomData;

	UPROPERTY(EditAnywhere, Category = "KMods|LootChest")
	FKPCLRange mRandomTrys = FKPCLRange(5, 20);

	bool bIsDismantled = false;

	UPROPERTY(SaveGame, ReplicatedUsing = OnRep_IsEmpty)
	bool bIsEmpty = false;

private:
	UFUNCTION()
	bool FilterItemClasses(TSubclassOf<UObject> object, int32 idx) const;

	UFUNCTION()
	void OnInputItemRemoved(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved,
							UFGInventoryComponent* sourceInventory);

	friend class UKPCLLootChestSpawnDesc;
};
