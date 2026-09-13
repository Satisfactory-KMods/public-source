// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "FGSaveInterface.h"
#include "Subsystem/ModSubsystem.h"

#include "KDFStateSubsystem.generated.h"

USTRUCT(BlueprintType)
struct KDATAFORGE_API FKDFPackHistoryEntry
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "KDataForge")
	FString mPackRef;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "KDataForge")
	FString mVersion;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "KDataForge")
	FString mFirstApplied;
};

UCLASS()
class KDATAFORGE_API AKDFStateSubsystem : public AModSubsystem, public IFGSaveInterface
{
	GENERATED_BODY()

public:
	AKDFStateSubsystem();

	virtual void BeginPlay() override;

	virtual bool ShouldSave_Implementation() const override { return true; }

	UFUNCTION(BlueprintPure, Category = "KDataForge")
	const TArray<FKDFPackHistoryEntry>& GetPackHistory() const { return mPackHistory; }

private:
	void RecordAppliedPacks();

	UPROPERTY(SaveGame)
	TArray<FKDFPackHistoryEntry> mPackHistory;
};
