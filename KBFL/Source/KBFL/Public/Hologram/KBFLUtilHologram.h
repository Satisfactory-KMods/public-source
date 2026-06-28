// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "Hologram/FGBuildableHologram.h"
#include "Hologram/FGFactoryBuildingHologram.h"
#include "KBFLUtilWidget.h"
#include "Subsystems/KBFLLocationSubsystem.h"

#include "KBFLUtilHologram.generated.h"

UCLASS()
class KBFL_API AKBFLUtilHologram : public AFGFactoryBuildingHologram
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual bool IsValidHitResult(const FHitResult& hitResult) const override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void GetSupportedBuildModes_Implementation(
		TArray<TSubclassOf<UFGBuildGunModeDescriptor>>& out_buildmodes) const override;

	virtual void SetHologramLocationAndRotation(const FHitResult& hitResult) override;
	;

	virtual void Scroll(int32 delta) override;
	;

	virtual void ScrollRotate(int32 delta, int32 step) override;
	virtual void ConfigureComponents(AFGBuildable* inBuildable) const override;

	void UpdateWidget();

	FRotator AddRotation = FRotator(0);
	FVector AddVector = FVector(0);
	FVector Scale = FVector(1);

	UPROPERTY(EditDefaultsOnly, Category = "Utils")
	float ScaleMulti = 0.05;

	UPROPERTY(EditDefaultsOnly, Category = "Utils")
	float ScrollMulti = 2;

	UPROPERTY(EditDefaultsOnly, Category = "Utils")
	bool UseRotationFromSubsystem = true;

	UPROPERTY(EditDefaultsOnly, Category = "Utils")
	bool UseVectorFromSubsystem = true;

	UPROPERTY(EditDefaultsOnly, Category = "Utils")
	bool UseScaleFromSubsystem = true;

	UPROPERTY(EditDefaultsOnly, Category = "Utils")
	TSubclassOf<UKBFLUtilWidget> WidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UKBFLUtilWidget> Widget;

	UPROPERTY(EditDefaultsOnly, Category = "Utils")
	TArray<TSubclassOf<UFGHologramBuildModeDescriptor>> mBuildModes;
};
