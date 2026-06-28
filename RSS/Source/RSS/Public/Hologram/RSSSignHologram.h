// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "Engine/EngineTypes.h"
#include "Hologram/FGFactoryHologram.h"
#include "Hologram/FGGenericBuildableHologram.h"

#include "RSSSignHologram.generated.h"

class ARssDataManagerSubsystem;

UCLASS()
class RSS_API ARSSSignHologram : public AFGGenericBuildableHologram
{
	GENERATED_BODY()

public:
	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End AActor Interface

	//~ Begin AFGBuildableHologram Interface
	virtual int32 GetRotationStep() const override;
	virtual void GetSupportedBuildModes_Implementation(
		TArray<TSubclassOf<UFGBuildGunModeDescriptor>>& out_buildmodes) const override;
	virtual void SetHologramLocationAndRotation(const FHitResult& hitResult) override;
	virtual bool TrySnapToActor(const FHitResult& hitResult) override;
	virtual void Scroll(int32 delta) override;
	//~ End AFGBuildableHologram Interface

private:
	float GetRotationFromSteps() const;

	UPROPERTY(EditDefaultsOnly, Category = "RSS|BuildModes")
	TSubclassOf<UFGHologramBuildModeDescriptor> mFineRotationMode;

	UPROPERTY()
	TObjectPtr<ARssDataManagerSubsystem> mSubsystem;

	UPROPERTY(EditDefaultsOnly, Category = "RSS")
	float mFineRotationSteps = 5.0f;

	UPROPERTY()
	float mCachedRotationSteps;

	UPROPERTY()
	int32 Rotation = 0;
};
