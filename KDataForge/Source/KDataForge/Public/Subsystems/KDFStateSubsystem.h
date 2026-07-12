#pragma once

#include "CoreMinimal.h"

#include "FGSaveInterface.h"
#include "Subsystem/ModSubsystem.h"

#include "KDFStateSubsystem.generated.h"

/** One applied-pack history entry persisted in the save. */
USTRUCT(BlueprintType)
struct KDATAFORGE_API FKDFPackHistoryEntry
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "KDataForge")
	FString mPackRef;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "KDataForge")
	FString mVersion;

	/** UTC ISO-8601 of the first session this pack+version applied in. */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "KDataForge")
	FString mFirstApplied;
};

/**
 * Per-save KDataForge state (spawned via UKDFWorldModule::ModSubsystems, server only).
 * Records which packs (and versions) have been applied to this save — the patch history that the
 * diff viewer and future migration logic read.
 */
UCLASS()
class KDATAFORGE_API AKDFStateSubsystem : public AModSubsystem, public IFGSaveInterface
{
	GENERATED_BODY()

public:
	AKDFStateSubsystem();

	virtual void BeginPlay() override;

	// IFGSaveInterface
	virtual bool ShouldSave_Implementation() const override { return true; }

	UFUNCTION(BlueprintPure, Category = "KDataForge")
	const TArray<FKDFPackHistoryEntry>& GetPackHistory() const { return mPackHistory; }

private:
	void RecordAppliedPacks();

	UPROPERTY(SaveGame)
	TArray<FKDFPackHistoryEntry> mPackHistory;
};
