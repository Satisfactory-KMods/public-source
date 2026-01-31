// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FGColoredInstanceMeshProxy.h"
#include "FGCrate.h"
#include "FGDestructibleActor.h"
#include "FGItemPickup.h"
#include "FGSignificanceInterface.h"
#include "ItemAmount.h"
#include "Kismet/KismetMathLibrary.h"
#include "KPCLLootChest.generated.h"

USTRUCT(BlueprintType)
struct FKPCLRange
{
	GENERATED_BODY()

	FKPCLRange()
	{
	}

	FKPCLRange(int32 A, int32 B)
	{
		mMin = A;
		mMax = B;
	}

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 mMin = 5;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 mMax = 15;

	int32 GetRandom() const
	{
		return UKismetMathLibrary::RandomIntegerInRange(mMin, mMax);
	}
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
class KPRIVATECODELIB_API AKPCLLootChest : public AFGDestructibleActor, public IFGUseableInterface
{
	GENERATED_BODY()

public:
	/** Decide on what properties to replicate */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool ShouldSave_Implementation() const override;
	virtual float TakeDamage(float DamageAmount, const struct FDamageEvent& DamageEvent,
	                         class AController* EventInstigator, AActor* DamageCauser) override;

	// Begin IFGUseableInterface
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
	// End IFGUseableInterface

	// Sets default values for this actor's properties
	AKPCLLootChest();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void GenerateLoot();
	void UpdateActorState();

	UFUNCTION(BlueprintPure, Category="KMods|LootChest")
	bool WasLooted() const;

	UFUNCTION(BlueprintPure, Category="KMods|LootChest")
	UFGInventoryComponent* GetInventory() const;

	UFUNCTION(BlueprintCallable, Category="KMods|LootChest")
	void Loot(AFGCharacterPlayer* Player);

	UPROPERTY(BlueprintAssignable, Category="KMods|LootChest")
	FOnLootedUpdated OnLootedChanged;

	UFUNCTION(BlueprintImplementableEvent, Category="KMods|LootChest")
	void OnLootedUpdated(bool Looted);

private:
	UFUNCTION()
	bool FilterItemClasses(TSubclassOf<UObject> object, int32 idx) const;

	UFUNCTION()
	void OnInputItemRemoved(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved,
	                        UFGInventoryComponent* sourceInventory);

	friend class UKPCLLootChestSpawnDesc;

public:
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "KMods|Interaction")
	FText mDisplayName;

	/** The widget that will present our UI. */
	UPROPERTY(EditDefaultsOnly, Category = "KMods|Interaction",
		meta = ( DisplayName = "Interact Widget Class" ))
	TSoftClassPtr<class UFGInteractWidget> mInteractWidgetSoftClass;

	UPROPERTY(EditDefaultsOnly, Replicated, SaveGame, Category = "KMods|Inventory")
	UFGInventoryComponent* mInventory;

	UPROPERTY(EditAnywhere, Category="KMods|LootChest")
	TArray<FKPCLLootChestRandomData> mRandomData;

	UPROPERTY(EditAnywhere, Category="KMods|LootChest")
	FKPCLRange mRandomTrys = FKPCLRange(5, 20);
};
