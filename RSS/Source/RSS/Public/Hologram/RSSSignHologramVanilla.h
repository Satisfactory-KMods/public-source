// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "Hologram/FGFactoryHologram.h"
#include "Hologram/FGStandaloneSignHologram.h"

#include "EnumStruc/RssEnum.h"

#include "RSSSignHologramVanilla.generated.h"

class ARssDataManagerSubsystem;

UCLASS()
class RSS_API ARSSSignHologramVanilla : public AFGStandaloneSignHologram
{
	GENERATED_BODY()

public:
	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End AActor Interface

	//~ Begin AFGBuildableHologram Interface
	virtual void GetSupportedBuildModes_Implementation(
		TArray<TSubclassOf<UFGBuildGunModeDescriptor>>& out_buildmodes) const override;
	virtual bool IsValidHitResult(const FHitResult& hitResult) const override;
	virtual bool TrySnapToActor(const FHitResult& hitResult) override;
	virtual void Scroll(int32 delta) override;
	virtual int32 GetRotationStep() const override;
	virtual void SpawnChildren(AActor* hologramOwner, FVector spawnLocation, APawn* hologramInstigator) override;
	virtual void CheckValidFloor() override;
	//~ End AFGBuildableHologram Interface

	virtual void HandlePoleSnap(const FHitResult& hitResult);

private:
	float RoundInSteps(float HitLocation, float MainLocation, bool CanBeNegative = false, int32 maxStepSize = -1) const;
	float GetRotationFromSteps() const;

	UPROPERTY(EditDefaultsOnly, Category = "RSS|BuildModes")
	TSubclassOf<UFGHologramBuildModeDescriptor> mFineRotationMode;

	UPROPERTY()
	TObjectPtr<ARssDataManagerSubsystem> mSubsystem;

	UPROPERTY()
	TObjectPtr<AFGBuildable> mDefaultBuildable;

	UPROPERTY(EditDefaultsOnly, Category = "RSS")
	float mGridSteps = 25.0f;

	UPROPERTY(EditDefaultsOnly, Category = "RSS")
	float mPoleRoationYaw = -90.0f;

	UPROPERTY(EditDefaultsOnly, Category = "RSS")
	float mTargetPoleOffset = 30.0f;

	UPROPERTY(EditDefaultsOnly, Category = "RSS")
	bool mShouldSnapOnSignPole = true;

	UPROPERTY(EditDefaultsOnly, Category = "RSS")
	float mFineRotationSteps = 5.0f;

	UPROPERTY()
	float mCachedRotationSteps;

	UPROPERTY()
	int32 Rotation = 0;

	AActor* mTargetActor;
	ERssTargetActorType mLastActorType;
	bool mIsValid;
};
