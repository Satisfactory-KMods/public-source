// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Buildables/FGBuildablePipelineAttachment.h"
#include "FGFactoryClipboard.h"
#include "Structures/KPCLFunctionalStructure.h"

#include "KPCLPressureRegulatorValve.generated.h"

enum class EPipeConnectionType : uint8;

UCLASS()
class KPRIVATECODELIB_API UKPCLPressureRegulatorValveClipboardSettings : public UFGFactoryClipboardSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite)
	float mOnThreshold = 0.8f;

	UPROPERTY(BlueprintReadWrite)
	float mOffThreshold = 0.2f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKPCLOnValveOpenAlphaChanged, float, ValveOpenAlpha);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKPCLOnFillLevelChanged, float, NormalizedFillLevel);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FKPCLOnThresholdsChanged, float, OnThreshold, float, OffThreshold);

USTRUCT(BlueprintType)
struct KPRIVATECODELIB_API FKPCLPressureRegulatorValveStatsSample
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "KMods|Valve|Statistics")
	float mFlowRate = 0.f;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "KMods|Valve|Statistics")
	float mOffThreshold = 0.f;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "KMods|Valve|Statistics")
	float mOnThreshold = 0.f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKPCLOnValveStatisticsChanged,
											const TArray<FKPCLPressureRegulatorValveStatsSample>&, Samples);

UCLASS()
class KPRIVATECODELIB_API AKPCLPressureRegulatorValve : public AFGBuildablePipelineAttachment
{
	GENERATED_BODY()

public:
	AKPCLPressureRegulatorValve();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;

	virtual void GetConditionalReplicatedProps(TArray<FFGCondReplicatedProperty>& outProps) const override;

	virtual void OnFluidDescriptorSet() override;

	virtual bool CanUseFactoryClipboard_Implementation() override;
	virtual UFGFactoryClipboardSettings* CopySettings_Implementation() override;
	virtual bool PasteSettings_Implementation(UFGFactoryClipboardSettings* factoryClipboard,
											  class AFGPlayerController* player) override;

	virtual bool CanProduce_Implementation() const override;

	virtual void Factory_Tick(float Dt) override;

	UFUNCTION(BlueprintPure, Category = "KMods|Valve")
	float GetNormalizedFillLevel() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Valve")
	FORCEINLINE float GetMinFillLevel() const { return 0.f; }

	UFUNCTION(BlueprintPure, Category = "KMods|Valve")
	float GetMaxFillLevel() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Valve")
	float GetFillLevel() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Valve")
	FORCEINLINE float GetValveOpenAlpha() const { return mValveOpenAlpha; }

	UFUNCTION(BlueprintPure, Category = "KMods|Valve")
	FORCEINLINE bool IsValveOpen() const { return bValveIsOpen; }

	UFUNCTION(BlueprintPure, Category = "KMods|Valve")
	FORCEINLINE float GetOnThreshold() const { return mOnThreshold; }

	UFUNCTION(BlueprintPure, Category = "KMods|Valve")
	FORCEINLINE float GetOffThreshold() const { return mOffThreshold; }

	UFUNCTION(BlueprintPure, Category = "KMods|Valve")
	FORCEINLINE bool IsInverted() const { return mOnThreshold < mOffThreshold; }

	UFUNCTION(BlueprintPure, Category = "KMods|Valve")
	FORCEINLINE float GetMaxFlowRate() const { return mMaxFlowRate; }

	UFUNCTION(BlueprintPure, Category = "KMods|Valve")
	FORCEINLINE TSubclassOf<class UFGItemDescriptor> GetFluidDescriptor() const { return mCachedFluidDescriptor; }

	UFUNCTION(BlueprintPure, Category = "KMods|Valve|Statistics")
	FORCEINLINE TArray<FKPCLPressureRegulatorValveStatsSample> GetStatisticsSamples() const
	{
		return mStatisticsSamples;
	}

	UFUNCTION(BlueprintPure, Category = "KMods|Valve|Statistics")
	bool GetLastStatisticsSample(FKPCLPressureRegulatorValveStatsSample& OutSample) const;

	UFUNCTION(BlueprintPure, Category = "KMods|Valve|Statistics")
	float GetStatisticsSampleInterval() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Valve|Statistics")
	float GetStatisticsWindowSeconds() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Valve|Statistics")
	int32 GetMaxStatisticsSamples() const;

	UFUNCTION(BlueprintCallable, Category = "KMods|Valve")
	void SetOnThreshold(float NewThreshold);

	UFUNCTION(BlueprintCallable, Category = "KMods|Valve")
	void SetOffThreshold(float NewThreshold);

	UFUNCTION(BlueprintCallable, Category = "KMods|Valve")
	void SetThresholds(float NewOnThreshold, float NewOffThreshold);

	UFUNCTION(BlueprintImplementableEvent, Category = "KMods|Valve")
	void OnValveStateChanged(bool bIsOpen);

	UFUNCTION(BlueprintImplementableEvent, Category = "KMods|Valve")
	void OnUiRequireUpdate();

	UPROPERTY(BlueprintAssignable, Category = "KMods|Valve")
	FKPCLOnValveOpenAlphaChanged OnValveOpenAlphaChanged;

	UPROPERTY(BlueprintAssignable, Category = "KMods|Valve")
	FKPCLOnFillLevelChanged OnFillLevelChanged;

	UPROPERTY(BlueprintAssignable, Category = "KMods|Valve")
	FKPCLOnThresholdsChanged OnThresholdsChanged;

	UPROPERTY(BlueprintAssignable, Category = "KMods|Valve|Statistics")
	FKPCLOnValveStatisticsChanged OnValveStatisticsChanged;

protected:

	void NotifyThresholdsChanged();

	float UpdateValveState(float Dt = 0.f);

	void ApplyValveFlowLimit();

	void CaptureStatisticsSample();

	void AddStatisticsSample(FKPCLPressureRegulatorValveStatsSample Sample);

	class UFGPipeConnectionComponent* ResolvePipeConnection(EPipeConnectionType ConnectionType,
															int32 FallbackIndex) const;

	const FFluidBox* GetConnectedFluidBox(EPipeConnectionType ConnectionType, int32 FallbackIndex) const;

	const FFluidBox* GetMonitoredFluidBox() const;

	UFUNCTION()
	virtual void OnRep_ValveIsOpen();

	UFUNCTION()
	void OnRep_FillLevel();

	UFUNCTION()
	void OnRep_Thresholds();

	UFUNCTION()
	void OnRep_StatisticsSamples();

	UPROPERTY(EditDefaultsOnly, Category = "KMods|Valve", meta = (ClampMin = "0.01"))
	float mMaxFlowRate = 10.f;

	UPROPERTY(EditDefaultsOnly, Category = "KMods|Valve", meta = (ClampMin = "0.1"))
	float mFluidBoxHeight = 6.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|Valve",
			  meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "5.0", Units = "s"))
	float mFillSmoothingTimeConstant = 0.1f;

	UPROPERTY(SaveGame, EditDefaultsOnly, BlueprintReadOnly,
			  meta = (FGReplicatedUsing = OnRep_Thresholds, ClampMin = "0.01", ClampMax = "1.25"),
			  Category = "KMods|Valve")
	float mOnThreshold = 0.8f;

	UPROPERTY(SaveGame, EditDefaultsOnly, BlueprintReadOnly,
			  meta = (FGReplicatedUsing = OnRep_Thresholds, ClampMin = "0.01", ClampMax = "1.25"),
			  Category = "KMods|Valve")
	float mOffThreshold = 0.2f;

	UPROPERTY(SaveGame, BlueprintReadOnly, ReplicatedUsing = OnRep_ValveIsOpen, Category = "KMods|Valve")
	bool bValveIsOpen = false;

	UPROPERTY(BlueprintReadOnly, Category = "KMods|Valve")
	float mValveOpenAlpha = 0.f;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_FillLevel, Category = "KMods|Valve")
	float mReplicatedFillLevel = 0.f;

	UPROPERTY(SaveGame, BlueprintReadOnly, meta = (FGReplicatedUsing = OnRep_StatisticsSamples),
			  Category = "KMods|Valve|Statistics")
	TArray<FKPCLPressureRegulatorValveStatsSample> mStatisticsSamples;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|Valve|Statistics",
			  meta = (ClampMin = "0.01", UIMin = "0.01", Units = "s"))
	float mStatisticsSampleInterval = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|Valve|Statistics", meta = (Units = "s"))
	float mStatisticsWindowSeconds = 120.f;

	UPROPERTY(SaveGame)
	FSmartTimer mStatisticsSampler;

	UPROPERTY(SaveGame)
	float mSmoothedFillLevel = 0.f;

	UPROPERTY(SaveGame)
	bool bHasSmoothedFillLevel = false;

	UPROPERTY(SaveGame)
	bool bSmoothedHighFillMode = true;
};
