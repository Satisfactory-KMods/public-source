

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
	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnOnServer_Replicate;
}

void AKPCLModSubsystem::Init()
{
	Super::Init();

	OnInit();
}

void AKPCLModSubsystem::BeginPlay()
{
	Super::BeginPlay();

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
