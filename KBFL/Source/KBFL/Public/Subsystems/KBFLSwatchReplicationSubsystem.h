#pragma once

#include "CoreMinimal.h"
#include "FGFactoryColoringTypes.h"
#include "FGRemoteCallObject.h"
#include "FGSaveInterface.h"
#include "KBFLLogging.h"
#include "Logging/StructuredLog.h"
#include "Subsystem/ModSubsystem.h"

#include "KBFLSwatchReplicationSubsystem.generated.h"

/**
 * Struct to hold a single color slot assignment that needs to be replicated.
 */
USTRUCT(Blueprintable, BlueprintType)
struct FKBFLReplicatedColorSlot
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
	FText mSlotName;

	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
	int32 mSlotIndex = INDEX_NONE;

	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
	FFactoryCustomizationColorSlot mColorSlot;

	UPROPERTY(SaveGame)
	FString mPath;

	UPROPERTY(SaveGame, VisibleAnywhere, BlueprintReadOnly)
	FString mModName;

	UPROPERTY(SaveGame, VisibleAnywhere, BlueprintReadOnly)
	bool bIsBaseGame = false;

	void PatchName(const FText& Name,
				   const TSet<TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch>>& EditableSwatch);

	UPROPERTY()
	TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch> mCachedClass = nullptr;

	TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch> GetCachedClass() const { return mCachedClass; }

	TSoftClassPtr<UFGFactoryCustomizationDescriptor_Swatch> GetSoftClass() const;

	TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch> LoadClass();
};

UCLASS(Blueprintable)
class KBFL_API UKBFLColorRCO : public UFGRemoteCallObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	static UKBFLColorRCO* Get(UObject* WorldContext);

	UFUNCTION(Server, WithValidation, Reliable, BlueprintCallable)
	void UpdateSwatchName(class AKBFLSwatchReplicationSubsystem* Subsystem,
						  TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch> Swatch, const FText& NewName);

	UPROPERTY(Replicated)
	bool bTest = true;
};

/**
 * Replicated ModSubsystem that handles swatch ID assignment, color slot setup, and replication.
 * Implements IFGSaveInterface so that color slot data persists across save/load cycles
 * (mReplicatedColorSlots is marked SaveGame).
 *
 * On the server:
 *  - Discovers all swatch classes, assigns stable IDs (using saved data from mReplicatedColorSlots),
 *    sets up color slots in AFGBuildableSubsystem / AFGGameState, and stores results back into
 *    mReplicatedColorSlots which then replicates to clients.
 *  - Uses IsSwatchUsed() to preserve IDs of swatches that are actively placed in the world.
 *  - Uses IsColorIdDuplicate() to avoid ID collisions.
 *
 * On clients:
 *  - Receives replicated color slots via OnRep_ColorSlots and applies them to the local world.
 */
UCLASS()
class KBFL_API AKBFLSwatchReplicationSubsystem : public AModSubsystem, public IFGSaveInterface
{
	GENERATED_BODY()

public:
	AKBFLSwatchReplicationSubsystem();

	UFUNCTION(BlueprintPure, meta = (WorldContext = "WorldContext", DisplayName = "GetSwatchReplicationSubsystem"))
	static AKBFLSwatchReplicationSubsystem* Get(UObject* WorldContext);

	UFUNCTION(BlueprintCallable)
	static TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch>
	GetCachedSwatchClass(const FKBFLReplicatedColorSlot& ColorSlot);

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	static int32 GetSwatchIdSafe(TSubclassOf<class UFGFactoryCustomizationDescriptor_Swatch> SwatchDesc);

	// Begin IFGSaveInterface
	virtual bool ShouldSave_Implementation() const override { return true; }
	virtual void PostLoadGame_Implementation(int32 saveVersion, int32 gameVersion) override;
	// End IFGSaveInterface

	/**
	 * Discovers all swatch classes, assigns stable IDs, sets up color slots,
	 * and stores the results in mReplicatedColorSlots for persistence & replication.
	 * Only runs on the server (HasAuthority).
	 */
	void TryToPatchSwatches();

	/**
	 * Apply replicated color slots to local AFGBuildableSubsystem and AFGGameState.
	 * Called on clients when data is received, and on server after setting the data.
	 */
	void ApplyColorSlotsToWorld();

	bool IsSwatchUsed(TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch> InClass) const;
	bool IsColorIdDuplicate(TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch> InClass) const;
	static bool IsBaseGameSwatch(const TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch>& SwatchInfo);
	static FString GetModNameFromPath(const FString& ModPath);

	UFUNCTION(BlueprintCallable, Category = "KMods|Swatch Replication")
	bool GetSlotInformation(TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch> InClass,
							FKBFLReplicatedColorSlot& OutColorSlot) const;

	UFUNCTION(BlueprintPure, Category = "KMods|Swatch Replication")
	bool IsSwatchEditable(TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch> InClass) const;

	UFUNCTION(BlueprintCallable, Category = "KMods|Swatch Replication")
	void UpdateSwatchName(TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch> Swatch, const FText& NewName);

	UPROPERTY(EditDefaultsOnly, Category = "Swatch")
	TSet<TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch>> mEditableSwatchNames;

private:
	/**
	 * Phase 1a: Assigns saved IDs or keeps world-used IDs for non-base-game swatches.
	 * Swatches with ID 0 are never kept (0 is reserved for base game).
	 * Updates ReservedIDs with all locked-in IDs.
	 */
	void PatchSwatchFromSave(TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch> Swatch, TSet<int32>& ReservedIDs);

	/**
	 * Phase 1b: Assigns new unique IDs to non-base-game swatches that weren't assigned in PatchSwatchFromSave.
	 * Uses ReservedIDs to avoid conflicts.
	 */
	void PatchSwatchAssignNew(TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch> Swatch, TSet<int32>& ReservedIDs);

	UFUNCTION()
	void OnRep_ColorSlots();

	UPROPERTY(SaveGame, ReplicatedUsing = OnRep_ColorSlots)
	TArray<FKBFLReplicatedColorSlot> mReplicatedColorSlots;

	/** Whether the color slots have been applied locally */
	bool bColorSlotsApplied = false;

	/** Guard against double-patching in a single session */
	bool bSwatchesPatched = false;
};
