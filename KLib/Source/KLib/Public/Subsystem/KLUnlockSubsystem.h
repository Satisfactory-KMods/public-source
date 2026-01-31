// ILikeBanas

#pragma once

#include "CoreMinimal.h"
#include "FGSchematic.h"
#include "BFL/KBFL_Util.h"
#include "DataAssets/KAPICleanerItemDescription.h"
#include "DataAssets/KAPIModularMinerDescription.h"
#include "Subsystem/KPCLModSubsystem.h"
#include "Subsystems/KAPIDataAssetSubsystem.h"

#include "KLUnlockSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCleanerDescUnlocked, TSubclassOf< UFGItemDescriptor >,
                                            CleanerDescriptor);

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class KLIB_API AKLUnlockSubsystem : public AKPCLModSubsystem
{
	GENERATED_BODY()

public:
	AKLUnlockSubsystem();

	virtual void BeginPlay() override;

	virtual void Init() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category = "KMods|Subsystem", DisplayName = "GetKLUnlockSubsystem",
		meta = ( DefaultToSelf = "WorldContext" ))
	static AKLUnlockSubsystem* Get(UObject* worldContext);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool ShouldSave_Implementation() const override { return true; }
	virtual void PostLoadGame_Implementation(int32 saveVersion, int32 gameVersion) override;

	void UnlockCleanerDesc(TSubclassOf<UFGItemDescriptor> CleanerDesc);

	UFUNCTION(BlueprintPure, Category="KMods|CleanerUnlock")
	UKAPIModularMinerDescription* GetInformationAboutOre(TSubclassOf<UFGResourceDescriptor> Desc,
	                                                     bool bSkipUnlockCheck = false) const;

	UFUNCTION(BlueprintPure, Category="KMods|CleanerUnlock")
	bool HasInformationAboutOre(TSubclassOf<UFGResourceDescriptor> Desc) const;

	UFUNCTION(BlueprintCallable, Category="KMods|CleanerUnlock")
	bool IsCleanerDescUnlock(TSubclassOf<UFGItemDescriptor> CleanerDesc);

	UFUNCTION(BlueprintCallable, Category="KMods|CleanerUnlock")
	UKAPICleanerItemDescription* GetUnlockedCleanerDesc(TSubclassOf<UFGItemDescriptor> CleanerDesc);

	UFUNCTION(BlueprintPure, Category="KMods|CleanerUnlock")
	TArray<UKAPIModularMinerDescription*> GetUnlockedExtractDescriptor() const;

	UFUNCTION(BlueprintCallable)
	TArray<UKAPICleanerItemDescription*> GetAllUnlockedCleanerDesc();

	UFUNCTION()
	void CleanerDescUnlocked();

private:
	UPROPERTY(SaveGame, Replicated, EditDefaultsOnly, Category="KMods|Miner")
	TArray<TSubclassOf<UFGItemDescriptor>> mUnlockedCleanerDescriptor;

public:
	UPROPERTY(BlueprintAssignable, Category = "KMods|Events")
	FCleanerDescUnlocked onCleanerDescUnlocked;

	UPROPERTY()
	UKAPIDataAssetSubsystem* mAssetSubsystem;

	UPROPERTY(Transient)
	TArray<UKAPICleanerItemDescription*> mCachedUnlockedCleanerDesc;

	UFUNCTION(BlueprintPure)
	UKAPIDataAssetSubsystem* GetAssetSubsystem() const;
};
