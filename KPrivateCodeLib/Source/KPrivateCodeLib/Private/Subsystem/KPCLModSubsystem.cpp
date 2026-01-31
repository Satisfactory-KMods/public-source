// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystem/KPCLModSubsystem.h"

void FSubsystemTick::ExecuteTick(float DeltaTime, ELevelTick TickType, ENamedThreads::Type CurrentThread,
                                 const FGraphEventRef& MyCompletionGraphEvent)
{
	if (mTarget)
	{
		mTarget->SubsytemTick(DeltaTime);
	}
}

AKPCLModSubsystem::AKPCLModSubsystem()
{
	PrimaryActorTick.bCanEverTick = 1;
	mSubsystemTick.bCanEverTick = 1;
	mSubsystemTick.bRunOnAnyThread = 1;
	mSubsystemTick.bStartWithTickEnabled = 0;
	mSubsystemTick.TickGroup = TG_PrePhysics;
	mSubsystemTick.EndTickGroup = TG_EndPhysics;
}

void AKPCLModSubsystem::Init()
{
	Super::Init();

	// CALL EVENT
	OnInit();
}

void AKPCLModSubsystem::BeginPlay()
{
	Super::BeginPlay();

	// Register our Tick if we want it.
	if (bUseSubsystemTick)
	{
		mSubsystemTick.mTarget = this;
		mSubsystemTick.RegisterTickFunction(GetLevel());
	}
}

void AKPCLModSubsystem::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (mSubsystemTick.IsTickFunctionRegistered())
	{
		mSubsystemTick.UnRegisterTickFunction();
	}
	Super::EndPlay(EndPlayReason);
}
