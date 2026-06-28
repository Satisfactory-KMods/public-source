// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "Hologram/FGFactoryHologram.h"
#include "Hologram/FGGenericBuildableHologram.h"

#include "RSSSignHologramDrone.generated.h"

UENUM()
enum class EDroneBuildStep : uint8
{
	Placement,
	UpDown,
	Build,
};

class ARssDataManagerSubsystem;

UCLASS()
class RSS_API ARSSSignHologramDrone : public AFGGenericBuildableHologram
{
	GENERATED_BODY()

public:
	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End AActor Interface

	//~ Begin AFGGenericBuildableHologram Interface
	virtual void GetSupportedBuildModes_Implementation(
		TArray<TSubclassOf<UFGBuildGunModeDescriptor>>& out_buildmodes) const override;
	virtual bool IsValidHitResult(const FHitResult& hitResult) const override;
	virtual int32 GetRotationStep() const override;
	virtual void SetHologramLocationAndRotation(const FHitResult& hitResult) override;
	virtual bool TrySnapToActor(const FHitResult& hitResult) override;
	virtual void Scroll(int32 delta) override;
	virtual bool DoMultiStepPlacement(bool isInputFromARelease) override;
	//~ End AFGGenericBuildableHologram Interface

private:
	float RoundInSteps(float HitLocation, float MainLocation, bool CanBeNegative = false, int32 maxStepSize = -1) const;
	float GetRotationFromSteps() const;

	UPROPERTY(EditDefaultsOnly, Category = "RSS|BuildModes")
	TSubclassOf<UFGHologramBuildModeDescriptor> mRotationMode;

	UPROPERTY(EditDefaultsOnly, Category = "RSS|BuildModes")
	TSubclassOf<UFGHologramBuildModeDescriptor> mUpDownMode;

	UPROPERTY()
	EDroneBuildStep mBuildStep = EDroneBuildStep::Placement;

	UPROPERTY()
	TObjectPtr<ARssDataManagerSubsystem> mSubsystem;

	UPROPERTY()
	FVector mPlaceLocation;

	UPROPERTY(EditDefaultsOnly, Category = "RSS")
	float mDownMin = 100.0f;

	UPROPERTY(EditDefaultsOnly, Category = "RSS")
	float mUpMax = 2000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "RSS")
	float mRoationStep = 5.0f;

	UPROPERTY(EditDefaultsOnly, Category = "RSS")
	float mGridSteps = 50.0f;

	UPROPERTY()
	int32 Rotation = 0;

	UPROPERTY()
	float DroneHeight = 0;
};
