// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "Components/SplineComponent.h"
#include "Hologram/FGWallAttachmentHologram.h"

#include "EnumStruc/RssEnum.h"
#include "EnumStruc/RssStruc.h"

#include "RSSSignHologramPipeAndBelts.generated.h"

class ARssDataManagerSubsystem;

UCLASS()
class RSS_API ARSSSignHologramPipeAndBelts : public AFGBuildableHologram
{
	GENERATED_BODY()

public:
	//~ Begin AFGBuildableHologram Interface
	virtual void HandlePipeSnap(const FHitResult& hitResult);
	virtual void HandleBeltSnap(const FHitResult& hitResult);
	//~ End AFGBuildableHologram Interface

protected:
	//~ Begin AFGBuildableHologram Interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual int32 GetRotationStep() const override;
	virtual bool IsValidHitResult(const FHitResult& hitResult) const override;
	virtual void SetHologramLocationAndRotation(const FHitResult& hitResult) override;
	virtual void Scroll(int32 delta) override;
	virtual void CheckValidPlacement() override;
	virtual void OnBuildModeChanged(TSubclassOf<UFGHologramBuildModeDescriptor> buildMode) override;
	virtual void GetSupportedBuildModes_Implementation(
		TArray<TSubclassOf<UFGBuildGunModeDescriptor>>& out_buildmodes) const override;
	//~ End AFGBuildableHologram Interface

	static FVector FindSplineLocation(const FVector& WorldLocation, USplineComponent* Spline);
	static FRotator FindSplineRotator(const FVector& WorldLocation, USplineComponent* Spline);

private:
	float GetRotationFromSteps() const;
	ERssTargetActorType GetTargetActor(const FHitResult& hitResult) const;

	UPROPERTY()
	TObjectPtr<ARssDataManagerSubsystem> mSubsystem;

	UPROPERTY()
	TObjectPtr<AFGBuildable> mDefaultBuildable;

	UPROPERTY(EditDefaultsOnly, Category = "RSS")
	bool mShouldSnapOnBelt = true;

	UPROPERTY()
	bool mIsMir = false;

	UPROPERTY(EditDefaultsOnly, Category = "RSS")
	bool mShouldSnapOnPipe = true;

	UPROPERTY(EditDefaultsOnly, Category = "RSS")
	float mRoationStep = 5.0f;

	UPROPERTY(EditDefaultsOnly, Category = "RSS|BuildModes")
	TSubclassOf<UFGHologramBuildModeDescriptor> mReversedBuildMode;

	UPROPERTY()
	int32 Rotation = 0;

	UPROPERTY()
	int32 mBuildStep = 0;

	AActor* mTargetActor;
	ERssTargetActorType mLastActorType;
	bool mIsValid;
};
