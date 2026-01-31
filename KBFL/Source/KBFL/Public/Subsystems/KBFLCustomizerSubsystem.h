#pragma once

#include "CoreMinimal.h"
#include "DataAssets/KBFLCustomizationProvider.h"
#include "FGFactoryColoringTypes.h"
#include "FGGameMode.h"
#include "FGSwatchGroup.h"
#include "KBFLLogging.h"
#include "Module/WorldModule.h"
#include "Subsystems/WorldSubsystem.h"

#include "KBFLCustomizerSubsystem.generated.h"

USTRUCT()
struct FKBFPSwatchSaveInformation
{
	GENERATED_BODY()

	UPROPERTY()
	int32 mSwatchID;

	UPROPERTY()
	FString mPath;

	UPROPERTY()
	FString mModName;

	UPROPERTY()
	bool bIsBaseGame = false;

	TSoftClassPtr<UFGFactoryCustomizationDescriptor_Swatch> GetSoftClass() const
	{
		return TSoftClassPtr<UFGFactoryCustomizationDescriptor_Swatch>(FSoftObjectPath(mPath));
	}

	TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch> LoadClass() const
	{
		TSoftClassPtr<UFGFactoryCustomizationDescriptor_Swatch> SoftClass = GetSoftClass();
		if (!SoftClass.IsValid())
		{
			UE_LOG(CustomizerSubsystem, Warning, TEXT("Failed to load Swatch Class from path: %s"), *mPath);
			return nullptr;
		}
		return GetSoftClass().LoadSynchronous();
	}
};

UCLASS(Blueprintable, BlueprintType)
class KBFL_API UKBFLactoryCustomizationDescriptor_Swatch : public UFGFactoryCustomizationDescriptor_Swatch
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = "Swatch")
	FFactoryCustomizationColorSlot mDefaultColorSlot;
};

/**
 *
 */
UCLASS()
class KBFL_API UKBFLCustomizerSubsystem : public UWorldSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	/** Implement this for initialization of instances of the system */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	bool GatherDefaultCollections();
	bool GatherProviders();
	bool RegisterSwatchesInSubsystem(
		const TMap<TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch>, FKBFLSwatchInformation>& SwatchMap);
	bool
	RegisterSwatchGroups(TMap<TSubclassOf<UFGSwatchGroup>, TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch>> Map);
	void CDOMaterials(TArray<FKBFLMaterialDescriptorInformation> CDOInformation);

	UFUNCTION(BlueprintCallable)
	void BeginForProvider(UKBFLCustomizationProvider* Provider);

	/**
	 * Set Default Swatch to Swatch group (should call early before the player can place something)
	 * otherwise it will crash
	 */
	UFUNCTION(BlueprintCallable, Category = "KMods|Customizer Subsystem")
	bool SetDefaultToSwatchGroup(TSubclassOf<UFGSwatchGroup> SwatchGroup,
								 TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch> Swatch);

	UFUNCTION(BlueprintPure, Category = "KMods|Customizer Subsystem")
	TMap<int32, TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch>> GetSwatchMap() const;

	void SaveDirty();
	void LoadSaved();
	void TryToPatchSwatches();

	UFUNCTION(BlueprintCallable, Category = "KMods|Customizer Subsystem")
	void PatchSwatch(TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch> Swatch);

	UFUNCTION(BlueprintPure, Category = "KMods|Customizer Subsystem")
	FString GetModNameFromPath(FString modPath);

	UFUNCTION(BlueprintPure, Category = "KMods|Customizer Subsystem")
	static bool IsBaseGameSwatch(const TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch>& SwatchInfo);

private:
	// Save/Load helpers specific to FKBFPSwatchSaveInformation
	bool SaveSwatchArrayToFile(const TArray<FKBFPSwatchSaveInformation>& Swatches, const FString& FilePath);
	bool LoadSwatchArrayFromFile(TArray<FKBFPSwatchSaveInformation>& OutSwatches, const FString& FilePath);
	FString GetSwatchSavePath() const;

	UPROPERTY()
	TArray<FKBFPSwatchSaveInformation> mSavedSwatches;

	UPROPERTY()
	TMap<int32, TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch>> mSwatchIDMap;

	UPROPERTY()
	TSubclassOf<UFGFactoryCustomizationCollection> mDefaultSwatchCollection;

	UPROPERTY()
	TSubclassOf<UFGFactoryCustomizationCollection> mDefaultMaterialCollection;

	UPROPERTY()
	TSubclassOf<UFGFactoryCustomizationCollection> mDefaultPatternCollection;

	UPROPERTY()
	TSubclassOf<UFGFactoryCustomizationCollection> mDefaultSkinCollection;

	UPROPERTY()
	TObjectPtr<AFGGameMode> mGameMode;

	UPROPERTY(Transient)
	TSet<TObjectPtr<UKBFLCustomizationProvider>> mCustomizationProviders;

	bool bDefaultGathered = false;
	bool bInitialized = false;
	bool bSwatchesPatched = false;
};
