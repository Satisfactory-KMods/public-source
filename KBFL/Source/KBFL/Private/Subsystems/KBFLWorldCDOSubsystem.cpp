#include "Subsystems/KBFLWorldCDOSubsystem.h"

#include "Engine/World.h"
#include "KBFLLogging.h"
#include "Logging/StructuredLog.h"
#include "Subsystems/HelperClasses/KBFLCDOOverwriteBase.h"
#include "Subsystems/KBFLContentCDOHelperSubsystem.h"
#include "TimerManager.h"

void UKBFLWorldCDOSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	bInitialized = true;
	bWorldPostInitCalled = false;

	UE_LOGFMT(LogKBFLModule, Log, "UKBFLWorldCDOSubsystem::Initialize");

	// Register for OnPostWorldInitialization event - called AFTER loading but BEFORE BeginPlay
	if (UWorld* World = GetWorld())
	{
		World->OnWorldBeginPlay.AddUObject(this, &UKBFLWorldCDOSubsystem::OnWorldPostInit);
	}
}

void UKBFLWorldCDOSubsystem::Deinitialize()
{
	bInitialized = false;

	UE_LOGFMT(LogKBFLModule, Log, "UKBFLWorldCDOSubsystem::Deinitialize");

	// Unregister from OnPostWorldInitialization event
	if (mOnWorldPostInitHandle.IsValid())
	{
		FWorldDelegates::OnPostWorldInitialization.Remove(mOnWorldPostInitHandle);
		mOnWorldPostInitHandle.Reset();
	}

	// Clear all registered overwrites
	for (UKBFLCDOOverwriteWorldBasedBase* Overwrite : mRegisteredWorldOverwrites)
	{
		if (IsValid(Overwrite))
		{
			Overwrite->Clear();
		}
	}
	mRegisteredWorldOverwrites.Empty();

	Super::Deinitialize();
}

void UKBFLWorldCDOSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	for (UKBFLCDOOverwriteWorldBasedBase* Overwrite : mTickableWorldOverwrites)
	{
		if (IsValid(Overwrite))
		{
			Overwrite->Tick(DeltaTime);
		}
	}
}

TStatId UKBFLWorldCDOSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UKBFLWorldCDOSubsystem, STATGROUP_Tickables);
}

UKBFLWorldCDOSubsystem* UKBFLWorldCDOSubsystem::Get(UObject* WorldContext)
{
	if (IsValid(WorldContext))
	{
		if (UWorld* World = WorldContext->GetWorld())
		{
			return World->GetSubsystem<UKBFLWorldCDOSubsystem>();
		}
	}
	return nullptr;
}

void UKBFLWorldCDOSubsystem::OnWorldPostInit()
{
	UWorld* World = GetWorld();
	// Only apply to the correct world
	if (!World || !World->GetGameInstance())
	{
		return;
	}

	if (bWorldPostInitCalled)
	{
		return;
	}

	bWorldPostInitCalled = true;

#if WITH_EDITOR
	return;
#endif

	UE_LOGFMT(LogKBFLModule, Log,
			  "UKBFLWorldCDOSubsystem::OnWorldPostInit - Applying world-based CDO overwrites (AFTER save load, BEFORE "
			  "BeginPlay)");


	// Safety check: Ensure CDOHelperSubsystem is available
	UKBFLContentCDOHelperSubsystem* CDOHelperSubsystem =
		World->GetGameInstance()->GetSubsystem<UKBFLContentCDOHelperSubsystem>();
	if (!IsValid(CDOHelperSubsystem))
	{
		UE_LOGFMT(
			LogKBFLModule, Warning,
			"UKBFLWorldCDOSubsystem::OnWorldPostInit - CDOHelperSubsystem not available yet, deferring application");

		// Defer the application to the next tick
		World->GetTimerManager().SetTimerForNextTick(
			[this, World]()
			{
				if (UKBFLContentCDOHelperSubsystem* DeferredSubsystem =
						World->GetGameInstance()->GetSubsystem<UKBFLContentCDOHelperSubsystem>())
				{
					DeferredSubsystem->FindAllDataAssetsOfClass(mRegisteredWorldOverwrites);
					ApplyAllWorldCDOOverwrites();
				}
				else
				{
					UE_LOGFMT(LogKBFLModule, Error,
							  "UKBFLWorldCDOSubsystem::OnWorldPostInit - CDOHelperSubsystem still not available after "
							  "deferring");
				}
			});
		return;
	}

	CDOHelperSubsystem->FindAllDataAssetsOfClass(mRegisteredWorldOverwrites);

	// Apply all registered world-based CDO overwrites
	ApplyAllWorldCDOOverwrites();
}

void UKBFLWorldCDOSubsystem::RegisterWorldCDOOverwrite(UKBFLCDOOverwriteWorldBasedBase* Overwrite)
{
	if (!IsValid(Overwrite))
	{
		return;
	}

	if (mRegisteredWorldOverwrites.Contains(Overwrite))
	{
		UE_LOGFMT(LogKBFLModule, Warning,
				  "UKBFLWorldCDOSubsystem::RegisterWorldCDOOverwrite - Overwrite already registered: {0}",
				  Overwrite->GetName());
		return;
	}

	mRegisteredWorldOverwrites.Add(Overwrite);

	UE_LOGFMT(LogKBFLModule, Log, "UKBFLWorldCDOSubsystem::RegisterWorldCDOOverwrite - Registered: {0}",
			  Overwrite->GetName());

	// If OnWorldPostInit was already called, apply immediately
	if (bWorldPostInitCalled)
	{
		UKBFLContentCDOHelperSubsystem* CDOHelperSubsystem = UKBFLContentCDOHelperSubsystem::Get(GetWorld());
		if (!IsValid(CDOHelperSubsystem))
		{
			UE_LOGFMT(LogKBFLModule, Warning,
					  "UKBFLWorldCDOSubsystem::RegisterWorldCDOOverwrite - CDOHelperSubsystem not available, cannot "
					  "apply overwrite: {0}",
					  Overwrite->GetName());
			return;
		}

		Overwrite->mSubsystem = CDOHelperSubsystem;
		Overwrite->SetWorld(GetWorld());
		Overwrite->Start();

		if (Overwrite->ShouldTick())
		{
			mTickableWorldOverwrites.Add(Overwrite);
		}
	}
}

void UKBFLWorldCDOSubsystem::ApplyAllWorldCDOOverwrites()
{
	UKBFLContentCDOHelperSubsystem* CDOHelperSubsystem = UKBFLContentCDOHelperSubsystem::Get(GetWorld());
	if (!IsValid(CDOHelperSubsystem))
	{
		UE_LOGFMT(LogKBFLModule, Error,
				  "UKBFLWorldCDOSubsystem::ApplyAllWorldCDOOverwrites - CDOHelperSubsystem not available, cannot apply "
				  "overwrites");
		return;
	}

	// Sort by priority
	mRegisteredWorldOverwrites.Sort([](const UKBFLCDOOverwriteWorldBasedBase& A,
									   const UKBFLCDOOverwriteWorldBasedBase& B) { return A.mCallPrio < B.mCallPrio; });

	for (UKBFLCDOOverwriteWorldBasedBase* Overwrite : mRegisteredWorldOverwrites)
	{
		if (!IsValid(Overwrite))
		{
			continue;
		}

		Overwrite->mSubsystem = CDOHelperSubsystem;
		Overwrite->SetWorld(GetWorld());
		Overwrite->Start();

		if (Overwrite->ShouldTick())
		{
			mTickableWorldOverwrites.Add(Overwrite);
		}
	}
}
