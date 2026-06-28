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

	UFUNCTION()
	void OnSchematicUnlocked(TSubclassOf<UFGSchematic> UnlockedSchematic);

	void RegisterLootChest(AKPCLLootChest* Chest);
	void UnregisterLootChest(AKPCLLootChest* Chest);
	const TArray<AKPCLLootChest*>& GetRegisteredLootChests() const;

private:
	UPROPERTY(Replicated)
	TArray<TObjectPtr<AKPCLLootChest>> mRegisteredLootChests;
};
