// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#include "Subsystems/KBFLContentCDOHelperSubsystem.h"

#include "Async/Async.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "KBFLDeveloperSettings.h"
#include "KBFLLogging.h"
#include "Subsystems/KBFLAssetDataSubsystem.h"
#include "Subsystems/KBFLWorldCDOSubsystem.h"
#include "TimerManager.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"

#pragma warning(push)
#pragma warning(disable : 4702)

namespace
{
	bool IsReadyForLazyCDOOverwrite(const UObject* Object)
	{
		constexpr EObjectFlags PendingLoadFlags =
			RF_NeedInitialization | RF_NeedLoad | RF_NeedPostLoad | RF_NeedPostLoadSubobjects;
		constexpr EInternalObjectFlags PendingInternalFlags =
			EInternalObjectFlags::Async | EInternalObjectFlags::AsyncLoadingPhase1 |
			EInternalObjectFlags::AsyncLoadingPhase2 | EInternalObjectFlags::PendingConstruction;
		return IsValid(Object) && !Object->HasAnyFlags(PendingLoadFlags) &&
			!Object->HasAnyInternalFlags(PendingInternalFlags);
	}
}

void UKBFLContentCDOHelperSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{

	if (UKBFLDeveloperSettings::Get()->bMuteCDOLogs)
	{
		LogKBFLCDOOverwrite.SetVerbosity(ELogVerbosity::NoLogging);
		ContentCDOHelperSubsystem.SetVerbosity(ELogVerbosity::NoLogging);
	}

	Collection.InitializeDependency(UKBFLAssetDataSubsystem::StaticClass());

	mAssetDataSubsystem = GetGameInstance()->GetSubsystem<UKBFLAssetDataSubsystem>();

	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(FName("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	mPreWorldInitHandle = FWorldDelegates::OnPreWorldInitialization.AddWeakLambda(
		this,
		[this](UWorld* World, const UWorld::InitializationValues)
		{
			if (!World || World->WorldType != EWorldType::Game)
			{
				return;
			}

			UE_LOGFMT(ContentCDOHelperSubsystem, Log,
					  "OnPreWorldInitialization > Game world '{0}' initializing, applying CDOs first",
					  World->GetName());
			EnsureCDOsApplied();
		});

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
	if (Initialized)
	{
		return;
	}

	UE_LOGFMT(ContentCDOHelperSubsystem, Log, "WaitForWorldAndScheduleCallback > scheduling CDO pass on core ticker");

	mBootstrapTickerHandle =
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this,
																			   [this](float)
																			   {
																				   OnTimerCallback();
																				   return false;
																			   }));
}

void UKBFLContentCDOHelperSubsystem::EnsureCDOsApplied()
{
	if (Initialized)
	{
		return;
	}

	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(FName("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	if (AssetRegistry.IsLoadingAssets())
	{
		UE_LOGFMT(ContentCDOHelperSubsystem, Warning,
				  "EnsureCDOsApplied > AssetRegistry still scanning, waiting for it rather than applying CDOs late");
		AssetRegistry.WaitForCompletion();
	}

	OnTimerCallback();
}

void UKBFLContentCDOHelperSubsystem::Deinitialize()
{
	bHasNonWorldOverwrites.store(false, std::memory_order_release);
	mCalledClasses.Empty();

	if (mWorldAddedHandle.IsValid() && GEngine)
	{
		GEngine->OnWorldAdded().Remove(mWorldAddedHandle);
		mWorldAddedHandle.Reset();
	}

	if (mPreWorldInitHandle.IsValid())
	{
		FWorldDelegates::OnPreWorldInitialization.Remove(mPreWorldInitHandle);
		mPreWorldInitHandle.Reset();
	}

	if (mBootstrapTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(mBootstrapTickerHandle);
		mBootstrapTickerHandle.Reset();
	}

#if WITH_EDITOR
	if (mPackageLoadHandle.IsValid())
	{
		FCoreUObjectDelegates::OnEndLoadPackage.Remove(mPackageLoadHandle);
		mPackageLoadHandle.Reset();
	}
#else
	if (bCreateListenerRegistered)
	{
		GUObjectArray.RemoveUObjectCreateListener(this);
		bCreateListenerRegistered = false;
	}
#endif
	if (mLazyClassTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(mLazyClassTickerHandle);
		mLazyClassTickerHandle.Reset();
	}
	mPendingLazyClasses.Empty();
	mAppliedLazyClasses.Empty();

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
	if (Initialized)
	{
		return;
	}
	Initialized = true;

	UE_LOGFMT(ContentCDOHelperSubsystem, Log, "UKBFLContentCDOHelperSubsystem::OnTimerCallback Start CDO Calls");
	FindAllDataAssetsOfClass(mCDOOverwritesToCall);

	mCDOOverwritesToCall.Sort([](const UKBFLCDOOverwriteBase& A, const UKBFLCDOOverwriteBase& B)
							  { return A.mCallPrio < B.mCallPrio; });

	TArray<UKBFLCDOOverwriteWorldBasedBase*> WorldBasedOverwrites;

	for (UKBFLCDOOverwriteBase* ToCall : mCDOOverwritesToCall)
	{

		if (UKBFLCDOOverwriteWorldBasedBase* WorldBasedOverwrite = Cast<UKBFLCDOOverwriteWorldBasedBase>(ToCall))
		{
			WorldBasedOverwrites.Add(WorldBasedOverwrite);
			continue;
		}

		UE_LOGFMT(ContentCDOHelperSubsystem, Log, "Start CDO Call for Overwrite: {0}", ToCall->GetName());
		ToCall->mSubsystem = this;
		ToCall->Start();

		mNonWorldOverwritesToCall.Add(ToCall);
	}

	if (WorldBasedOverwrites.Num() > 0)
	{
		UE_LOGFMT(ContentCDOHelperSubsystem, Log, "Found {0} world-based CDO overwrites to register",
				  WorldBasedOverwrites.Num());

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
	bHasNonWorldOverwrites.store(!mNonWorldOverwritesToCall.IsEmpty(), std::memory_order_release);
	if (bHasNonWorldOverwrites.load(std::memory_order_acquire) && !bCreateListenerRegistered)
	{
		GUObjectArray.AddUObjectCreateListener(this);
		bCreateListenerRegistered = true;
	}
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

void UKBFLContentCDOHelperSubsystem::NotifyUObjectCreated(const UObjectBase* Object, int32  )
{
#if !WITH_EDITOR

	if (!Object || !bHasNonWorldOverwrites.load(std::memory_order_acquire))
	{
		return;
	}

	const UClass* MetaClass = Object->GetClass();
	if (!MetaClass || !MetaClass->IsChildOf(UBlueprintGeneratedClass::StaticClass()))
	{
		return;
	}

	UClass* NewClass = const_cast<UClass*>(static_cast<const UClass*>(Object));

	TWeakObjectPtr<UKBFLContentCDOHelperSubsystem> WeakThis(this);
	TWeakObjectPtr<UClass> WeakClass(NewClass);
	AsyncTask(ENamedThreads::GameThread,
			  [WeakThis, WeakClass]()
			  {
				  UKBFLContentCDOHelperSubsystem* Subsystem = WeakThis.Get();
				  UClass* Class = WeakClass.Get();
				  if (Subsystem && Class)
				  {
					  Subsystem->QueueLazyClassForProcessing(Class);
				  }
			  });
#endif
}

void UKBFLContentCDOHelperSubsystem::QueueLazyClassForProcessing(UClass* NewClass)
{
	check(IsInGameThread());
	if (!bHasNonWorldOverwrites.load(std::memory_order_acquire) || !IsValid(NewClass) ||
		mAppliedLazyClasses.Contains(NewClass))
	{
		return;
	}

	mPendingLazyClasses.Add(NewClass);
	if (!mLazyClassTickerHandle.IsValid())
	{
		mLazyClassTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateUObject(this, &UKBFLContentCDOHelperSubsystem::ProcessPendingLazyClasses));
	}
}

bool UKBFLContentCDOHelperSubsystem::ProcessPendingLazyClasses(float  )
{
	check(IsInGameThread());
	if (!bHasNonWorldOverwrites.load(std::memory_order_acquire))
	{
		mPendingLazyClasses.Empty();
		mLazyClassTickerHandle.Reset();
		return false;
	}

	for (auto It = mPendingLazyClasses.CreateIterator(); It; ++It)
	{
		UClass* NewClass = It->Get();
		if (!IsValid(NewClass))
		{
			It.RemoveCurrent();
			continue;
		}
		if (mAppliedLazyClasses.Contains(NewClass))
		{
			It.RemoveCurrent();
			continue;
		}
		if (!IsReadyForLazyCDOOverwrite(NewClass) || NewClass->ClassWithin == nullptr ||
			NewClass->ClassConstructor == nullptr)
		{
			continue;
		}

		const UPackage* Package = NewClass->GetOutermost();
		if (Package && Package->HasAnyPackageFlags(PKG_ContainsMap | PKG_ContainsMapData))
		{
			It.RemoveCurrent();
			continue;
		}

		UObject* CDO = NewClass->GetDefaultObject(false);
		if (!IsReadyForLazyCDOOverwrite(CDO))
		{
			continue;
		}

		It.RemoveCurrent();

		mAppliedLazyClasses.Add(NewClass);
		if (!mAssetDataSubsystem)
		{
			mAssetDataSubsystem = GetGameInstance()->GetSubsystem<UKBFLAssetDataSubsystem>();
		}
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

	if (mPendingLazyClasses.IsEmpty())
	{
		mLazyClassTickerHandle.Reset();
		return false;
	}
	return true;
}

void UKBFLContentCDOHelperSubsystem::OnUObjectArrayShutdown()
{
#if !WITH_EDITOR
	bHasNonWorldOverwrites.store(false, std::memory_order_release);
	if (bCreateListenerRegistered)
	{
		GUObjectArray.RemoveUObjectCreateListener(this);
		bCreateListenerRegistered = false;
	}
#endif
}

#pragma warning(pop)
