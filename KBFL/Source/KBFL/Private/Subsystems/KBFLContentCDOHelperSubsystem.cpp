#include "Subsystems/KBFLContentCDOHelperSubsystem.h"

#include "Async/Async.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "KBFLDeveloperSettings.h"
#include "KBFLLogging.h"
#include "TimerManager.h"
#include "Subsystems/KBFLAssetDataSubsystem.h"
#include "Subsystems/KBFLWorldCDOSubsystem.h"
#include "UObject/UObjectGlobals.h"

// Several functions below use the `#if WITH_EDITOR return; #endif` editor-skip pattern, which makes
// the remaining bodies unreachable in editor builds and triggers C4702. The dead code is intentional
// (editor-skip), so the warning is suppressed for this translation unit.
#pragma warning(push)
#pragma warning(disable : 4702)


void UKBFLContentCDOHelperSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	// Optionally mute CDO logs. Setting the category to NoLogging makes every UE_LOG/UE_LOGFMT
	// short-circuit before building its (often large) string args — a big speedup with many CDOs.
	if (UKBFLDeveloperSettings::Get()->bMuteCDOLogs)
	{
		LogKBFLCDOOverwrite.SetVerbosity(ELogVerbosity::NoLogging);
		ContentCDOHelperSubsystem.SetVerbosity(ELogVerbosity::NoLogging);
	}

	Collection.InitializeDependency(UKBFLAssetDataSubsystem::StaticClass());

	// Cache once — the lazy-load listener uses it per newly-loaded class.
	mAssetDataSubsystem = GetGameInstance()->GetSubsystem<UKBFLAssetDataSubsystem>();

	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(FName("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	// Must wait until all assets are discovered before populating list of assets.
	if (AssetRegistry.IsLoadingAssets())
	{
		AssetRegistry.OnFilesLoaded().AddWeakLambda(this, [this]() { WaitForWorldAndScheduleCallback(); });
	}
	else
	{
		WaitForWorldAndScheduleCallback();
	}

	Super::Initialize(Collection);
}

void UKBFLContentCDOHelperSubsystem::WaitForWorldAndScheduleCallback()
{
	if (UWorld* World = GetWorld())
	{
		UE_LOGFMT(ContentCDOHelperSubsystem, Log,
				  "WaitForWorldAndScheduleCallback > World '{0}' already valid, scheduling callback", World->GetName());
		World->GetTimerManager().SetTimerForNextTick(this, &UKBFLContentCDOHelperSubsystem::OnTimerCallback);
		return;
	}

	// World not yet available – wait for the engine to add a game world that belongs to our GameInstance.
	UE_LOGFMT(ContentCDOHelperSubsystem, Warning,
			  "WaitForWorldAndScheduleCallback > GetWorld() returned null, waiting for first game UWorld");

	mWorldAddedHandle = GEngine->OnWorldAdded().AddWeakLambda(
		this,
		[this](UWorld* World)
		{
			if (!World || World->WorldType != EWorldType::Game)
			{
				return;
			}
			if (World->GetGameInstance() != GetGameInstance())
			{
				return;
			}

			UE_LOGFMT(ContentCDOHelperSubsystem, Log,
					  "WaitForWorldAndScheduleCallback > Received game world '{0}', scheduling callback",
					  World->GetName());

			GEngine->OnWorldAdded().Remove(mWorldAddedHandle);
			mWorldAddedHandle.Reset();

			World->GetTimerManager().SetTimerForNextTick(this, &UKBFLContentCDOHelperSubsystem::OnTimerCallback);
		});
}

void UKBFLContentCDOHelperSubsystem::Deinitialize()
{
	mCalledClasses.Empty();

	// Remove world-added listener in case we never received a valid world.
	if (mWorldAddedHandle.IsValid() && GEngine)
	{
		GEngine->OnWorldAdded().Remove(mWorldAddedHandle);
		mWorldAddedHandle.Reset();
	}

#if WITH_EDITOR
	if (mPackageLoadHandle.IsValid())
	{
		FCoreUObjectDelegates::OnEndLoadPackage.Remove(mPackageLoadHandle);
		mPackageLoadHandle.Reset();
	}
#else
	GUObjectArray.RemoveUObjectCreateListener(this);
#endif

	Initialized = false;

	mModulesToCall.Empty();
	mCalledObjects.Empty();

	for (UKBFLCDOOverwriteBase* ToCall : mCDOOverwritesToCall)
	{
		if (IsValid(Cast<UKBFLCDOOverwriteWorldBasedBase>(ToCall)))
		{
			continue;
		}

		ToCall->Clear();
	}
	mCDOOverwritesToCall.Empty();
	mNonWorldOverwritesToCall.Empty();

	Super::Deinitialize();
}

UKBFLContentCDOHelperSubsystem* UKBFLContentCDOHelperSubsystem::Get(UObject* Context)
{
	if (IsValid(Context))
	{
		return Context->GetWorld()->GetGameInstance()->GetSubsystem<UKBFLContentCDOHelperSubsystem>();
	}
	return nullptr;
}

void UKBFLContentCDOHelperSubsystem::OnTimerCallback()
{
#if WITH_EDITOR
	return;
#endif

	UE_LOGFMT(ContentCDOHelperSubsystem, Log, "UKBFLContentCDOHelperSubsystem::OnTimerCallback Start CDO Calls");
	FindAllDataAssetsOfClass(mCDOOverwritesToCall);

	// Sort by mCallPrio (ascending - lower numbers first)
	mCDOOverwritesToCall.Sort([](const UKBFLCDOOverwriteBase& A, const UKBFLCDOOverwriteBase& B)
							  { return A.mCallPrio < B.mCallPrio; });

	// Separate world-based overwrites from regular overwrites
	TArray<UKBFLCDOOverwriteWorldBasedBase*> WorldBasedOverwrites;

	for (UKBFLCDOOverwriteBase* ToCall : mCDOOverwritesToCall)
	{
		// Check if this is a world-based overwrite
		if (UKBFLCDOOverwriteWorldBasedBase* WorldBasedOverwrite = Cast<UKBFLCDOOverwriteWorldBasedBase>(ToCall))
		{
			WorldBasedOverwrites.Add(WorldBasedOverwrite);
			continue;
		}

		// Apply regular (non-world-based) overwrites immediately
		UE_LOGFMT(ContentCDOHelperSubsystem, Log, "Start CDO Call for Overwrite: {0}", ToCall->GetName());
		ToCall->mSubsystem = this;
		ToCall->Start();

		// Cache for the lazy-load listener (no per-class Cast/skip).
		mNonWorldOverwritesToCall.Add(ToCall);
	}

	// Register world-based overwrites to all existing worlds
	if (WorldBasedOverwrites.Num() > 0)
	{
		UE_LOGFMT(ContentCDOHelperSubsystem, Log, "Found {0} world-based CDO overwrites to register",
				  WorldBasedOverwrites.Num());

		// Get all worlds and register the overwrites
		for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
		{
			UWorld* World = WorldContext.World();
			if (!World || World->WorldType != EWorldType::Game)
			{
				continue;
			}

			if (UKBFLWorldCDOSubsystem* WorldCDOSubsystem = World->GetSubsystem<UKBFLWorldCDOSubsystem>())
			{
				for (UKBFLCDOOverwriteWorldBasedBase* WorldBasedOverwrite : WorldBasedOverwrites)
				{
					if (!WorldBasedOverwrite || !WorldBasedOverwrite->bEnabled)
					{
						continue;
					}

					WorldBasedOverwrite->mSubsystem = this;
					WorldBasedOverwrite->SetWorld(World);

					UE_LOGFMT(ContentCDOHelperSubsystem, Log, "Registering world-based overwrite '{0}' to world '{1}'",
							  WorldBasedOverwrite->GetName(), World->GetName());

					WorldCDOSubsystem->RegisterWorldCDOOverwrite(WorldBasedOverwrite);
				}
			}
		}
	}

	// Re-apply non-world overwrites when a class is lazily loaded after the initial CDO pass.
	// Editor: OnEndLoadPackage fires once per package after its objects are fully loaded & linked.
	// Shipping: that delegate is WITH_EDITOR-only, so use the FUObjectArray create listener
	// (NotifyUObjectCreated), which defers per-class work to the game thread once linking is done.
#if WITH_EDITOR
	if (!mPackageLoadHandle.IsValid())
	{
		mPackageLoadHandle = FCoreUObjectDelegates::OnEndLoadPackage.AddWeakLambda(
			this,
			[this](const FEndLoadPackageContext& Params)
			{
				for (UPackage* Package : Params.LoadedPackages)
				{
					if (!Package)
					{
						continue;
					}
					TArray<UObject*> PackageObjects;
					GetObjectsWithOuter(Package, PackageObjects, true, RF_NoFlags);
					for (UObject* Obj : PackageObjects)
					{
						UClass* NewClass = Cast<UClass>(Obj);
						if (!NewClass || !NewClass->HasAnyClassFlags(CLASS_CompiledFromBlueprint))
						{
							continue;
						}
						for (UKBFLCDOOverwriteBase* Overwrite : mCDOOverwritesToCall)
						{
							if (!Overwrite || Cast<UKBFLCDOOverwriteWorldBasedBase>(Overwrite))
							{
								continue;
							}
							Overwrite->TryApplyToClass(NewClass);
						}
					}
				}
			});
	}
#else
	GUObjectArray.AddUObjectCreateListener(this);
#endif
}

UObject* UKBFLContentCDOHelperSubsystem::GetAndStoreDefaultObject(UClass* Class)
{
#if WITH_EDITOR
	return Class->GetDefaultObject();
#endif

	if (IsValid(Class))
	{
		mCalledClasses.Add(Class);
		mCalledObjects.Add(Class->GetDefaultObject());
		return Class->GetDefaultObject();
	}
	return nullptr;
}

UObject* UKBFLContentCDOHelperSubsystem::GetAndStoreCDO(UClass* Class, UObject* Context)
{
	if (UKBFLContentCDOHelperSubsystem* Sub = Get(Context))
	{
		return Sub->GetAndStoreDefaultObject(Class);
	}
	return nullptr;
}

void UKBFLContentCDOHelperSubsystem::StoreClass(UClass* Class) { mCalledClasses.Add(Class); }

void UKBFLContentCDOHelperSubsystem::StoreObject(UObject* Object) { mCalledObjects.Add(Object); }

void UKBFLContentCDOHelperSubsystem::RemoveClass(UClass* Class) { mCalledClasses.Remove(Class); }

void UKBFLContentCDOHelperSubsystem::RemoveObject(UObject* Object) { mCalledObjects.Remove(Object); }

void UKBFLContentCDOHelperSubsystem::NotifyUObjectCreated(const UObjectBase* Object, int32 /*Index*/)
{
#if !WITH_EDITOR
	if (!Object || mNonWorldOverwritesToCall.IsEmpty())
	{
		return;
	}

	// We only care about newly-created class objects (their meta-class is UBlueprintGeneratedClass).
	// Instances of blueprint classes are filtered out: their GetClass() (the generated class) is a child
	// of AActor/UObject, not of UBlueprintGeneratedClass.
	const UClass* MetaClass = Object->GetClass();
	if (!MetaClass || !MetaClass->IsChildOf(UBlueprintGeneratedClass::StaticClass()))
	{
		return;
	}

	// The created object IS the new class. Dedupe on it so each class is processed at most once
	// (and so classes already handled by the initial pass are skipped).
	UClass* NewClass = (UClass*)static_cast<const UClass*>(Object);
	if (!NewClass || mCalledClasses.Contains(NewClass))
	{
		return;
	}
	mCalledClasses.Add(NewClass);

	// On the game thread we can apply immediately (avoids one task per loaded class). Off-thread we must
	// defer: TryApplyToClass touches UObject/CDO state that is game-thread-only. Exactly one of the two
	// paths runs (else), so a class is never applied twice (important for additive numeric behaviors).
	if (IsInGameThread())
	{
		if (mAssetDataSubsystem)
		{
			mAssetDataSubsystem->NotifyClassLoaded(NewClass);
		}

		for (UKBFLCDOOverwriteBase* Overwrite : mNonWorldOverwritesToCall)
		{
			if (Overwrite)
			{
				Overwrite->TryApplyToClass(NewClass);
			}
		}
	}
	else
	{
		TWeakObjectPtr<UKBFLContentCDOHelperSubsystem> WeakThis(this);
		TWeakObjectPtr<UClass> WeakClass(NewClass);
		AsyncTask(ENamedThreads::GameThread,
				  [WeakThis, WeakClass]()
				  {
					  UKBFLContentCDOHelperSubsystem* Sub = WeakThis.Get();
					  UClass* Cls = WeakClass.Get();
					  if (!Sub || !Cls)
					  {
						  return;
					  }

					  if (Sub->mAssetDataSubsystem)
					  {
						  Sub->mAssetDataSubsystem->NotifyClassLoaded(Cls);
					  }

					  for (UKBFLCDOOverwriteBase* Overwrite : Sub->mNonWorldOverwritesToCall)
					  {
						  if (Overwrite)
						  {
							  Overwrite->TryApplyToClass(Cls);
						  }
					  }
				  });
	}
#endif
}

void UKBFLContentCDOHelperSubsystem::OnUObjectArrayShutdown()
{
#if !WITH_EDITOR
	GUObjectArray.RemoveUObjectCreateListener(this);
#endif
}

#pragma warning(pop)
