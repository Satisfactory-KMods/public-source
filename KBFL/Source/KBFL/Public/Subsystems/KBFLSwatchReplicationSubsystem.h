#pragma once

#include "CoreMinimal.h"
#include "FGFactoryColoringTypes.h"
#include "FGRemoteCallObject.h"
#include "FGSaveInterface.h"
#include "KBFLLogging.h"
#include "Logging/StructuredLog.h"
#include "Subsystem/ModSubsystem.h"

#include "KBFLSwatchReplicationSubsystem.generated.h"

class AFGBuildable;

USTRUCT(Blueprintable, BlueprintType)
struct FKBFLReplicatedColorSlot
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
	uint8 mSlotIndex = 255;

	UPROPERTY(SaveGame)
	FString mPath;

	UPROPERTY(SaveGame, VisibleAnywhere, BlueprintReadOnly)
	FString mModName;

	UPROPERTY(SaveGame, VisibleAnywhere, BlueprintReadOnly)
	bool bIsBaseGame = false;

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

	UPROPERTY(Replicated)
	bool bTest = true;
};

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
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	static int32 GetSwatchIdSafe(TSubclassOf<class UFGFactoryCustomizationDescriptor_Swatch> SwatchDesc);

	virtual bool ShouldSave_Implementation() const override { return true; }
	virtual void PostLoadGame_Implementation(int32 saveVersion, int32 gameVersion) override;

	void TryToPatchSwatches();

	void ApplyColorSlotsToWorld();

	bool IsSwatchUsed(TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch> InClass) const;
	bool IsColorIdDuplicate(TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch> InClass) const;
	static bool IsBaseGameSwatch(const TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch>& SwatchInfo);
	static bool ShouldRefreshPaintFinishCustomization(const FFactoryCustomizationData& CustomizationData);
	static FString GetModNameFromPath(const FString& ModPath);

	UFUNCTION(BlueprintCallable, Category = "KMods|Swatch Replication")
	bool GetSlotInformation(TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch> InClass,
							FKBFLReplicatedColorSlot& OutColorSlot) const;

	UFUNCTION(BlueprintPure, Category = "KMods|Swatch Replication")
	bool IsSwatchEditable(TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch> InClass) const;

	UPROPERTY(EditDefaultsOnly, Category = "Swatch")
	TSet<TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch>> mEditableSwatchNames;

private:

	void RefreshPaintFinishCustomizations();
	bool RefreshPaintFinishCustomization(AFGBuildable* Buildable);
	void SchedulePaintFinishCustomizationRefreshes();

	UFUNCTION()
	void OnBuildableAdded(AFGBuildable* Buildable);

	void DeduplicateSavedSlots(TSet<int32>& ReservedIDs);

	void PatchSwatchFromSave(TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch> Swatch, TSet<int32>& ReservedIDs);

	void PatchSwatchAssignNew(TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch> Swatch, TSet<int32>& ReservedIDs);

	UFUNCTION()
	void OnRep_ColorSlots();

	UPROPERTY(SaveGame, ReplicatedUsing = OnRep_ColorSlots)
	TArray<FKBFLReplicatedColorSlot> mReplicatedColorSlots;

	bool bColorSlotsApplied = false;

	bool bSwatchesPatched = false;
};
