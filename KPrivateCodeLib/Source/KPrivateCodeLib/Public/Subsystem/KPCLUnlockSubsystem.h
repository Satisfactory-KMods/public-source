// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "FGSchematic.h"

#include "KPCLModSubsystem.h"
#include "Looting/KPCLLootChest.h"
#include "Network/Buildings/KPCLNetworkCore.h"

#include "KPCLUnlockSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNetworkTierUnlocked, int32, Tier);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPointsUpdated, int64, mNewPointCount);

USTRUCT(BlueprintType)
struct KPRIVATECODELIB_API FKPCLRepeatPurchaseState
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Repeat Purchases")
	TSubclassOf<UFGSchematic> mSchematicClass;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Repeat Purchases")
	int32 mPurchaseCount = 0;
};

UCLASS(Blueprintable, BlueprintType)
class KPRIVATECODELIB_API AKPCLUnlockSubsystem : public AKPCLModSubsystem
{
	GENERATED_BODY()

	AKPCLUnlockSubsystem();

public:
	UFUNCTION(BlueprintPure, Category = "Subsystem", DisplayName = "GetKPCLUnlockSubsystem",
			  meta = (DefaultToSelf = "worldContext"))
	static AKPCLUnlockSubsystem* Get(UObject* worldContext);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void Init() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void PostLoadGame_Implementation(int32 saveVersion, int32 gameVersion) override;

	/** Resolves the KPCL unlock subsystem for static schematic APIs that provide no world context. */
	static AKPCLUnlockSubsystem* GetFromCurrentWorld();

	UFUNCTION(BlueprintPure, Category = "KMods|Unlocks|Repeat Purchases")
	int32 GetRepeatPurchaseCount(TSubclassOf<UFGSchematic> SchematicClass) const;

	/** Returns -1 for unlimited, otherwise remaining total purchases. */
	UFUNCTION(BlueprintPure, Category = "KMods|Unlocks|Repeat Purchases")
	int32 GetRemainingRepeatPurchases(TSubclassOf<UFGSchematic> SchematicClass) const;

	UFUNCTION(BlueprintPure, Category = "KMods|Unlocks|Repeat Purchases")
	bool CanPurchaseRepeatSchematic(TSubclassOf<UFGSchematic> SchematicClass) const;

	UFUNCTION(BlueprintPure, Category = "KMods|Unlocks|Repeat Purchases")
	float GetRepeatRewardMultiplier(TSubclassOf<UFGSchematic> SchematicClass) const;

	/** Called by UFGSchematic::GetCost hook. Caches vanilla cost on first call. */
	TArray<FItemAmount> GetScaledSchematicCost(TSubclassOf<UFGSchematic> SchematicClass,
											   const TArray<FItemAmount>& VanillaCost);

	UFUNCTION()
	void OnSchematicUnlocked(TSubclassOf<UFGSchematic> UnlockedSchematic);

	void RegisterLootChest(AKPCLLootChest* Chest);
	void UnregisterLootChest(AKPCLLootChest* Chest);
	const TArray<AKPCLLootChest*>& GetRegisteredLootChests() const;

private:
	UFUNCTION()
	void OnRep_RepeatPurchaseStates();

	/** Pushes authoritative count to connected clients immediately. Array replication handles late joins. */
	UFUNCTION(NetMulticast, Reliable)
	void MulticastRepeatPurchaseState(TSubclassOf<UFGSchematic> SchematicClass, int32 PurchaseCount);

	void RefreshRepeatPurchaseSchematics();
	void ApplyRepeatPurchaseState(TSubclassOf<UFGSchematic> SchematicClass, int32 PurchaseCount);
	const FKPCLRepeatPurchaseState* FindRepeatPurchaseState(TSubclassOf<UFGSchematic> SchematicClass) const;
	FKPCLRepeatPurchaseState* FindRepeatPurchaseState(TSubclassOf<UFGSchematic> SchematicClass);

	UPROPERTY(SaveGame, ReplicatedUsing = OnRep_RepeatPurchaseStates, BlueprintReadOnly,
			  Category = "KMods|Unlocks|Repeat Purchases", meta = (AllowPrivateAccess = "true"))
	TArray<FKPCLRepeatPurchaseState> mRepeatPurchaseStates;

	UPROPERTY(Replicated)
	TArray<TObjectPtr<AKPCLLootChest>> mRegisteredLootChests;
};
