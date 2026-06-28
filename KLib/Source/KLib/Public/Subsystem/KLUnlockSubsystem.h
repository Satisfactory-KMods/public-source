// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "BFL/KBFL_Util.h"
#include "DataAssets/KAPICleanerItemDescription.h"
#include "DataAssets/KAPIModularMinerDescription.h"
#include "FGSchematic.h"
#include "Subsystem/KPCLModSubsystem.h"
#include "Subsystems/KAPIDataAssetSubsystem.h"

#include "KLUnlockSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCleanerDescUnlocked, TSubclassOf<UFGItemDescriptor>, CleanerDescriptor);

UCLASS(Blueprintable, BlueprintType)
class KLIB_API AKLUnlockSubsystem : public AKPCLModSubsystem
{
	GENERATED_BODY()

public:
	AKLUnlockSubsystem();

	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	//~ End AActor Interface

	//~ Begin AKPCLModSubsystem Interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Init() override;
	virtual void PostLoadGame_Implementation(int32 saveVersion, int32 gameVersion) override;
	virtual bool ShouldSave_Implementation() const override { return true; }
	//~ End AKPCLModSubsystem Interface

	UFUNCTION(BlueprintPure, Category = "KMods|Subsystem", DisplayName = "GetKLUnlockSubsystem",
			  meta = (DefaultToSelf = "WorldContext"))
	static AKLUnlockSubsystem* Get(UObject* worldContext);

	UFUNCTION(BlueprintCallable)
	TArray<UKAPICleanerItemDescription*> GetAllUnlockedCleanerDesc();

	UFUNCTION(BlueprintCallable, Category = "KMods|CleanerUnlock")
	UKAPICleanerItemDescription* GetUnlockedCleanerDesc(TSubclassOf<UFGItemDescriptor> CleanerDesc);

	UFUNCTION(BlueprintPure, Category = "KMods|CleanerUnlock")
	UKAPIModularMinerDescription* GetInformationAboutOre(TSubclassOf<UFGResourceDescriptor> Desc,
														 bool bSkipUnlockCheck = false) const;

	UFUNCTION(BlueprintPure, Category = "KMods|CleanerUnlock")
	TArray<UKAPIModularMinerDescription*> GetUnlockedExtractDescriptor() const;

	UFUNCTION(BlueprintPure, Category = "KMods|CleanerUnlock")
	bool HasInformationAboutOre(TSubclassOf<UFGResourceDescriptor> Desc) const;

	UFUNCTION(BlueprintPure)
	UKAPIDataAssetSubsystem* GetAssetSubsystem() const;

	UFUNCTION(BlueprintCallable, Category = "KMods|CleanerUnlock")
	bool IsCleanerDescUnlock(TSubclassOf<UFGItemDescriptor> CleanerDesc);

	UFUNCTION()
	void CleanerDescUnlocked();

	/** Rebuilds the client-side cache whenever the replicated unlock list changes. */
	UFUNCTION()
	void OnRep_UnlockedCleanerDescriptor();

	void UnlockCleanerDesc(TSubclassOf<UFGItemDescriptor> CleanerDesc);

	UPROPERTY(SaveGame, ReplicatedUsing = OnRep_UnlockedCleanerDescriptor, EditDefaultsOnly, Category = "KMods|Miner")
	TArray<TSubclassOf<UFGItemDescriptor>> mUnlockedCleanerDescriptor;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UKAPICleanerItemDescription>> mCachedUnlockedCleanerDesc;

	UPROPERTY(BlueprintAssignable, Category = "KMods|Events")
	FCleanerDescUnlocked onCleanerDescUnlocked;

	UPROPERTY()
	TObjectPtr<UKAPIDataAssetSubsystem> mAssetSubsystem;
};
