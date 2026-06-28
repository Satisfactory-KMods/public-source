// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "Components/SplineComponent.h"
#include "Hologram/FGWallAttachmentHologram.h"

#include "EnumStruc/RssEnum.h"
#include "EnumStruc/RssStruc.h"

#include "RSSSignHologramWallCeilings.generated.h"

class ARssDataManagerSubsystem;

UCLASS()
class RSS_API ARSSSignHologramWallCeilings : public AFGWallAttachmentHologram
{
	GENERATED_BODY()

public:
	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End AActor Interface

	//~ Begin AFGBuildableHologram Interface
	virtual int32 GetRotationStep() const override;
	virtual void OnBuildModeChanged(TSubclassOf<UFGHologramBuildModeDescriptor> buildMode) override;
	virtual void GetSupportedBuildModes_Implementation(
		TArray<TSubclassOf<UFGBuildGunModeDescriptor>>& out_buildmodes) const override;
	virtual bool IsValidHitResult(const FHitResult& hitResult) const override;
	virtual void SetHologramLocationAndRotation(const FHitResult& hitResult) override;
	virtual void Scroll(int32 delta) override;
	virtual void CheckValidPlacement() override;
	//~ End AFGBuildableHologram Interface

	virtual void HandlePoleSnap(const FHitResult& hitResult);

private:
	float RoundInSteps(float HitLocation, float MainLocation, bool CanBeNegative = false, int32 maxStepSize = -1) const;
	float GetRotationFromSteps() const;
	ERssTargetActorType GetTargetActor(const FHitResult& hitResult) const;

	UPROPERTY(EditDefaultsOnly, Category = "RSS|BuildModes")
	TSubclassOf<UFGHologramBuildModeDescriptor> mFineRotationMode;

	UPROPERTY()
	TObjectPtr<ARssDataManagerSubsystem> mSubsystem;

	UPROPERTY()
	TObjectPtr<AFGBuildable> mDefaultBuildable;

	UPROPERTY(EditDefaultsOnly, Category = "RSS")
	float mGridSteps = 25.0f;

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
