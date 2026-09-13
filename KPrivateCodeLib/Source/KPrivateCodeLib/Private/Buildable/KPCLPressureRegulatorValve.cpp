// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#include "Buildable/KPCLPressureRegulatorValve.h"

#include "Async/TaskGraphInterfaces.h"
#include "Buildable/KPCLPressureRegulatorValveLogic.h"
#include "FGPipeConnectionComponent.h"
#include "Net/UnrealNetwork.h"
#include "Replication/KPCLDefaultRCO.h"

namespace
{
	constexpr float MinimumThreshold = 0.01f;
	constexpr float MaximumThreshold = 1.25f;
	constexpr int32 PipeAFallbackIndex = 0;
	constexpr int32 PipeBFallbackIndex = 1;

	FKPCLPressureValvePipeData MakePipeData(const FFluidBox* FluidBox)
	{
		if (!FluidBox)
		{
			return {};
		}

		return {FluidBox->Content, FluidBox->MaxContent};
	}

	bool IsReadablePipe(const FKPCLPressureValvePipeData& Pipe)
	{
		return FMath::IsFinite(Pipe.Content) && FMath::IsFinite(Pipe.MaxContent) && Pipe.MaxContent > 0.f;
	}

	float SmoothFillPercent(float PreviousValue, float NewValue, float Dt, float TimeConstant)
	{
		if (!FMath::IsFinite(NewValue))
		{
			return 0.f;
		}
		if (!FMath::IsFinite(PreviousValue) || !FMath::IsFinite(TimeConstant) || TimeConstant <= 0.f)
		{
			return NewValue;
		}
		if (!FMath::IsFinite(Dt) || Dt <= 0.f)
		{
			return PreviousValue;
		}

		const float Alpha = 1.f - FMath::Exp(-Dt / TimeConstant);
		return FMath::Lerp(PreviousValue, NewValue, FMath::Clamp(Alpha, 0.f, 1.f));
	}
}

AKPCLPressureRegulatorValve::AKPCLPressureRegulatorValve()
{
	mStatisticsSampler = FSmartTimer(mStatisticsSampleInterval, true);
	mFactoryTickFunction.bCanEverTick = true;
	mFactoryTickFunction.bStartWithTickEnabled = true;
	mFactoryTickFunction.bAllowTickOnDedicatedServer = true;
	mFactoryTickFunction.TickInterval = 0.f;
	mAddToSignificanceManager = true;
	mIsTickRateManaged = true;

	mFluidBoxVolume = 25.f;
	mRadius = 0.5f;
}

void AKPCLPressureRegulatorValve::BeginPlay()
{
	Super::BeginPlay();

	mStatisticsSampler.mTime = mStatisticsSampleInterval;
	const int32 MaxStatisticsSamples = GetMaxStatisticsSamples();
	mStatisticsSampler.mIsActive = MaxStatisticsSamples > 0;
	if (mStatisticsSampler.mIsActive && FMath::IsFinite(mStatisticsSampler.mTimer))
	{
		mStatisticsSampler.mTimer = FMath::Clamp(mStatisticsSampler.mTimer, 0.f, mStatisticsSampler.mTime);
	}
	else
	{
		mStatisticsSampler.Reset();
	}
	if (HasAuthority())
	{
		const int32 SamplesToRemove = mStatisticsSamples.Num() - MaxStatisticsSamples;
		if (SamplesToRemove > 0)
		{
			mStatisticsSamples.RemoveAt(0, SamplesToRemove, EAllowShrinking::No);
			mPropertyReplicator.MarkPropertyDirty(FName("mStatisticsSamples"));
		}
	}

	mFluidBox.Height = mFluidBoxHeight;
	mFluidBox.LaminarHeight = FMath::Min(mFluidBox.LaminarHeight, mFluidBoxHeight);
	if (mFluidBox.LaminarHeight <= 0.f)
	{
		mFluidBox.LaminarHeight = mFluidBoxHeight;
	}

	mValveOpenAlpha = bValveIsOpen ? 1.f : 0.f;

	if (HasAuthority())
	{
		ApplyValveFlowLimit();
	}
}

void AKPCLPressureRegulatorValve::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AKPCLPressureRegulatorValve, bValveIsOpen);
	DOREPLIFETIME(AKPCLPressureRegulatorValve, mReplicatedFillLevel);
}

void AKPCLPressureRegulatorValve::GetConditionalReplicatedProps(TArray<FFGCondReplicatedProperty>& outProps) const
{
	Super::GetConditionalReplicatedProps(outProps);

	FG_DOREPCONDITIONAL_WITH_NOTIFY(ThisClass, mOnThreshold, OnRep_Thresholds);
	FG_DOREPCONDITIONAL_WITH_NOTIFY(ThisClass, mOffThreshold, OnRep_Thresholds);
	FG_DOREPCONDITIONAL_WITH_NOTIFY(ThisClass, mStatisticsSamples, OnRep_StatisticsSamples);
}

void AKPCLPressureRegulatorValve::OnFluidDescriptorSet()
{
	mCachedFluidDescriptor = nullptr;
	for (const TObjectPtr<UFGPipeConnectionComponent>& Connection : mPipeConnections)
	{
		if (IsValid(Connection))
		{
			if (const TSubclassOf<UFGItemDescriptor> FluidDescriptor = Connection->GetFluidDescriptor())
			{
				mCachedFluidDescriptor = FluidDescriptor;
				break;
			}
		}
	}

	OnUiRequireUpdate();
}

bool AKPCLPressureRegulatorValve::CanProduce_Implementation() const { return true; }

void AKPCLPressureRegulatorValve::Factory_Tick(float Dt)
{
	Super::Factory_Tick(Dt);

	if (!HasAuthority())
	{
		return;
	}

	const float SafeDt = FMath::IsFinite(Dt) && Dt > 0.f ? Dt : 0.f;
	const float FillPct = UpdateValveState(SafeDt);

	if (!FMath::IsNearlyEqual(mReplicatedFillLevel, FillPct, 0.005f))
	{
		mReplicatedFillLevel = FillPct;

		const float LocalFill = mReplicatedFillLevel;
		TWeakObjectPtr<AKPCLPressureRegulatorValve> WeakThis(this);
		FFunctionGraphTask::CreateAndDispatchWhenReady(
			[WeakThis, LocalFill]()
			{
				if (AKPCLPressureRegulatorValve* Valve = WeakThis.Get())
				{
					Valve->OnFillLevelChanged.Broadcast(LocalFill);
					Valve->OnUiRequireUpdate();
				}
			},
			TStatId(), nullptr, ENamedThreads::GameThread);
	}

	if (mStatisticsSampler.Tick(SafeDt))
	{
		CaptureStatisticsSample();
	}
}

float AKPCLPressureRegulatorValve::GetNormalizedFillLevel() const { return mReplicatedFillLevel; }

float AKPCLPressureRegulatorValve::GetMaxFillLevel() const
{
	const FFluidBox* FluidBox = GetMonitoredFluidBox();
	return FluidBox ? FluidBox->MaxContent : 0.f;
}

float AKPCLPressureRegulatorValve::GetFillLevel() const
{
	const FFluidBox* FluidBox = GetMonitoredFluidBox();
	return FluidBox ? FluidBox->Content : 0.f;
}

bool AKPCLPressureRegulatorValve::GetLastStatisticsSample(FKPCLPressureRegulatorValveStatsSample& OutSample) const
{
	OutSample = FKPCLPressureRegulatorValveStatsSample();
	if (mStatisticsSamples.IsEmpty())
	{
		return false;
	}

	OutSample = mStatisticsSamples.Last();
	return true;
}

float AKPCLPressureRegulatorValve::GetStatisticsSampleInterval() const { return mStatisticsSampleInterval; }

float AKPCLPressureRegulatorValve::GetStatisticsWindowSeconds() const { return mStatisticsWindowSeconds; }

int32 AKPCLPressureRegulatorValve::GetMaxStatisticsSamples() const
{
	if (!FMath::IsFinite(mStatisticsWindowSeconds) || mStatisticsWindowSeconds <= 0.f ||
		!FMath::IsFinite(mStatisticsSampleInterval) || mStatisticsSampleInterval <= 0.f)
	{
		return 0;
	}

	const double SampleCount =
		static_cast<double>(mStatisticsWindowSeconds) / static_cast<double>(mStatisticsSampleInterval);
	if (SampleCount >= static_cast<double>(MAX_int32))
	{
		return MAX_int32;
	}

	return FMath::Max(1, FMath::FloorToInt(SampleCount));
}

UFGPipeConnectionComponent* AKPCLPressureRegulatorValve::ResolvePipeConnection(EPipeConnectionType ConnectionType,
																			   int32 FallbackIndex) const
{
	for (const TObjectPtr<UFGPipeConnectionComponent>& Connection : mPipeConnections)
	{
		if (IsValid(Connection.Get()) && Connection->GetPipeConnectionType() == ConnectionType)
		{
			return Connection.Get();
		}
	}

	return mPipeConnections.IsValidIndex(FallbackIndex) && IsValid(mPipeConnections[FallbackIndex].Get())
		? mPipeConnections[FallbackIndex].Get()
		: nullptr;
}

const FFluidBox* AKPCLPressureRegulatorValve::GetConnectedFluidBox(EPipeConnectionType ConnectionType,
																   int32 FallbackIndex) const
{
	UFGPipeConnectionComponent* ValveConnection = ResolvePipeConnection(ConnectionType, FallbackIndex);
	UFGPipeConnectionComponent* NeighborConnection = ValveConnection ? ValveConnection->GetPipeConnection() : nullptr;
	IFGFluidIntegrantInterface* FluidIntegrant = NeighborConnection && NeighborConnection->HasFluidIntegrant()
		? NeighborConnection->GetFluidIntegrant()
		: nullptr;
	FFluidBox* FluidBox = FluidIntegrant ? FluidIntegrant->GetFluidBox() : nullptr;
	return FluidBox != &mFluidBox ? FluidBox : nullptr;
}

const FFluidBox* AKPCLPressureRegulatorValve::GetMonitoredFluidBox() const
{
	return mOnThreshold >= mOffThreshold ? GetConnectedFluidBox(EPipeConnectionType::PCT_CONSUMER, PipeAFallbackIndex)
										 : GetConnectedFluidBox(EPipeConnectionType::PCT_PRODUCER, PipeBFallbackIndex);
}

float AKPCLPressureRegulatorValve::UpdateValveState(float Dt)
{
	FKPCLPressureValvePipeData PipeA =
		MakePipeData(GetConnectedFluidBox(EPipeConnectionType::PCT_CONSUMER, PipeAFallbackIndex));
	FKPCLPressureValvePipeData PipeB =
		MakePipeData(GetConnectedFluidBox(EPipeConnectionType::PCT_PRODUCER, PipeBFallbackIndex));
	const bool bHighFillMode = mOnThreshold >= mOffThreshold;
	FKPCLPressureValvePipeData& MonitoredPipe = bHighFillMode ? PipeA : PipeB;

	if (!IsReadablePipe(MonitoredPipe))
	{
		mSmoothedFillLevel = 0.f;
		bHasSmoothedFillLevel = false;
		bSmoothedHighFillMode = bHighFillMode;
	}
	else
	{
		const float RawFillLevel = FKPCLPressureRegulatorValveLogic::CalculateFillPercent(MonitoredPipe);
		if (!bHasSmoothedFillLevel || bSmoothedHighFillMode != bHighFillMode)
		{
			mSmoothedFillLevel = RawFillLevel;
		}
		else
		{
			mSmoothedFillLevel = SmoothFillPercent(mSmoothedFillLevel, RawFillLevel, Dt, mFillSmoothingTimeConstant);
		}
		bHasSmoothedFillLevel = true;
		bSmoothedHighFillMode = bHighFillMode;

		MonitoredPipe.Content = mSmoothedFillLevel * MonitoredPipe.MaxContent;
	}

	const FKPCLPressureValveEvaluation Evaluation =
		FKPCLPressureRegulatorValveLogic::Evaluate(PipeA, PipeB, mOnThreshold, mOffThreshold, bValveIsOpen);

	if (Evaluation.bShouldBeOpen == bValveIsOpen)
	{
		return Evaluation.MonitoredFillPercent;
	}

	bValveIsOpen = Evaluation.bShouldBeOpen;
	mValveOpenAlpha = bValveIsOpen ? 1.f : 0.f;

	ApplyValveFlowLimit();

	const float LocalAlpha = mValveOpenAlpha;
	const bool bLocalOpen = bValveIsOpen;
	TWeakObjectPtr<AKPCLPressureRegulatorValve> WeakThis(this);
	FFunctionGraphTask::CreateAndDispatchWhenReady(
		[WeakThis, LocalAlpha, bLocalOpen]()
		{
			if (AKPCLPressureRegulatorValve* Valve = WeakThis.Get())
			{

				Valve->OnValveOpenAlphaChanged.Broadcast(LocalAlpha);
				Valve->OnValveStateChanged(bLocalOpen);
				Valve->OnUiRequireUpdate();
			}
		},
		TStatId(), nullptr, ENamedThreads::GameThread);

	return Evaluation.MonitoredFillPercent;
}

void AKPCLPressureRegulatorValve::ApplyValveFlowLimit()
{

	mFluidBox.FlowLimit = bValveIsOpen ? mMaxFlowRate : 0.f;
}

void AKPCLPressureRegulatorValve::CaptureStatisticsSample()
{
	FKPCLPressureRegulatorValveStatsSample Sample;
	Sample.mFlowRate = FMath::IsFinite(mFluidBox.FlowThrough) ? FMath::Abs(mFluidBox.FlowThrough) : 0.f;
	Sample.mOffThreshold = mOffThreshold;
	Sample.mOnThreshold = mOnThreshold;
	AddStatisticsSample(Sample);
}

void AKPCLPressureRegulatorValve::AddStatisticsSample(FKPCLPressureRegulatorValveStatsSample Sample)
{
	const int32 MaxStatisticsSamples = GetMaxStatisticsSamples();
	if (MaxStatisticsSamples <= 0)
	{
		return;
	}

	mStatisticsSamples.Add(Sample);
	const int32 SamplesToRemove = mStatisticsSamples.Num() - MaxStatisticsSamples;
	if (SamplesToRemove > 0)
	{
		mStatisticsSamples.RemoveAt(0, SamplesToRemove, EAllowShrinking::No);
	}

	mPropertyReplicator.MarkPropertyDirty(FName("mStatisticsSamples"));
	if (!IsInGameThread())
	{
		const TArray<FKPCLPressureRegulatorValveStatsSample> SamplesSnapshot = mStatisticsSamples;
		TWeakObjectPtr<AKPCLPressureRegulatorValve> WeakThis(this);
		FFunctionGraphTask::CreateAndDispatchWhenReady(
			[WeakThis, SamplesSnapshot]()
			{
				if (AKPCLPressureRegulatorValve* Valve = WeakThis.Get())
				{
					Valve->OnValveStatisticsChanged.Broadcast(SamplesSnapshot);
					Valve->OnUiRequireUpdate();
				}
			},
			TStatId(), nullptr, ENamedThreads::GameThread);
		return;
	}

	OnValveStatisticsChanged.Broadcast(mStatisticsSamples);
	OnUiRequireUpdate();
}

void AKPCLPressureRegulatorValve::OnRep_ValveIsOpen()
{

	mValveOpenAlpha = bValveIsOpen ? 1.f : 0.f;
	OnValveOpenAlphaChanged.Broadcast(mValveOpenAlpha);
	OnValveStateChanged(bValveIsOpen);
	OnUiRequireUpdate();
}

void AKPCLPressureRegulatorValve::OnRep_FillLevel()
{
	OnFillLevelChanged.Broadcast(mReplicatedFillLevel);
	OnUiRequireUpdate();
}

void AKPCLPressureRegulatorValve::OnRep_Thresholds() { NotifyThresholdsChanged(); }

void AKPCLPressureRegulatorValve::OnRep_StatisticsSamples()
{
	OnValveStatisticsChanged.Broadcast(mStatisticsSamples);
	OnUiRequireUpdate();
}

void AKPCLPressureRegulatorValve::NotifyThresholdsChanged()
{
	OnThresholdsChanged.Broadcast(mOnThreshold, mOffThreshold);
	OnUiRequireUpdate();
}

void AKPCLPressureRegulatorValve::SetOnThreshold(float NewThreshold)
{
	if (!HasAuthority())
	{
		if (UKPCLDefaultRCO* RCO = UKPCLDefaultRCO::Get(this))
		{
			RCO->Server_Valve_SetOnThreshold(this, NewThreshold);
		}
		return;
	}
	mOnThreshold = FMath::Clamp(NewThreshold, MinimumThreshold, MaximumThreshold);
	mPropertyReplicator.MarkPropertyDirty(FName("mOnThreshold"));
	UpdateValveState();
	NotifyThresholdsChanged();
}

void AKPCLPressureRegulatorValve::SetOffThreshold(float NewThreshold)
{
	if (!HasAuthority())
	{
		if (UKPCLDefaultRCO* RCO = UKPCLDefaultRCO::Get(this))
		{
			RCO->Server_Valve_SetOffThreshold(this, NewThreshold);
		}
		return;
	}
	mOffThreshold = FMath::Clamp(NewThreshold, MinimumThreshold, MaximumThreshold);
	mPropertyReplicator.MarkPropertyDirty(FName("mOffThreshold"));
	UpdateValveState();
	NotifyThresholdsChanged();
}

void AKPCLPressureRegulatorValve::SetThresholds(float NewOnThreshold, float NewOffThreshold)
{
	if (!HasAuthority())
	{
		if (UKPCLDefaultRCO* RCO = UKPCLDefaultRCO::Get(this))
		{
			RCO->Server_Valve_SetThresholds(this, NewOnThreshold, NewOffThreshold);
		}
		return;
	}

	mOnThreshold = FMath::Clamp(NewOnThreshold, MinimumThreshold, MaximumThreshold);
	mOffThreshold = FMath::Clamp(NewOffThreshold, MinimumThreshold, MaximumThreshold);
	mPropertyReplicator.MarkPropertyDirty(FName("mOnThreshold"));
	mPropertyReplicator.MarkPropertyDirty(FName("mOffThreshold"));

	UpdateValveState();
	NotifyThresholdsChanged();
}

bool AKPCLPressureRegulatorValve::CanUseFactoryClipboard_Implementation() { return true; }

UFGFactoryClipboardSettings* AKPCLPressureRegulatorValve::CopySettings_Implementation()
{
	UKPCLPressureRegulatorValveClipboardSettings* ClipboardSettings =
		NewObject<UKPCLPressureRegulatorValveClipboardSettings>(
			this, UKPCLPressureRegulatorValveClipboardSettings::StaticClass());
	ClipboardSettings->mOnThreshold = mOnThreshold;
	ClipboardSettings->mOffThreshold = mOffThreshold;
	return ClipboardSettings;
}

bool AKPCLPressureRegulatorValve::PasteSettings_Implementation(UFGFactoryClipboardSettings* factoryClipboard,
															   AFGPlayerController* player)
{
	const UKPCLPressureRegulatorValveClipboardSettings* ClipboardSettings =
		Cast<UKPCLPressureRegulatorValveClipboardSettings>(factoryClipboard);
	if (!ClipboardSettings)
	{
		return false;
	}

	SetThresholds(ClipboardSettings->mOnThreshold, ClipboardSettings->mOffThreshold);
	return true;
}
