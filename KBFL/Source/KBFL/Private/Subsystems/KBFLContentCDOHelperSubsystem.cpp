#include "Subsystems/KBFLContentCDOHelperSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "FGGameState.h"
#include "FGItemCategory.h"
#include "KBFLLogging.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Subsystems/HelperClasses/KBFL_CDOHelperClass_Recipes.h"
#include "Subsystems/KBFLAssetDataSubsystem.h"
#include "Subsystems/KBFLWorldCDOSubsystem.h"


void UKBFLContentCDOHelperSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Collection.InitializeDependency(UKBFLAssetDataSubsystem::StaticClass());

	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &UKBFLContentCDOHelperSubsystem::OnTimerCallback, 5.0f,
										   false);

	Super::Initialize(Collection);
}

void UKBFLContentCDOHelperSubsystem::Deinitialize()
{
	Initialized = false;

	mModulesToCall.Empty();
	mCDOCalled.Empty();
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

	for (UKBFL_CDOHelperClass_CategoryOrder* CDOHelper : mCategoriesToCall)
	{
		CDOHelper->ModifyValues();
		CDOHelper->DoCDO();
		CDOHelper->ExecuteBlueprintCDO();
		UE_LOG(ContentCDOHelperSubsystem, Log, TEXT("Do CDO Call for Helper: %s"), *CDOHelper->GetName());
	}
	UE_LOG(ContentCDOHelperSubsystem, Error, TEXT("UKBFLContentCDOHelperSubsystem::OnTimerCallback %d"),
		   mCategoriesToCall.Num());
}

void UKBFLContentCDOHelperSubsystem::MoveRecipesFromBuilding(TSoftClassPtr<> From, TSoftClassPtr<> To,
															 TArray<TSubclassOf<UFGItemCategory>> IgnoreCategory,
															 TArray<TSubclassOf<UFGRecipe>> IgnoreRecipe)
{
	TSubclassOf<UObject> SubFrom = nullptr;
	TSubclassOf<UObject> SubTo = nullptr;

	if (From.IsPending() || To.IsValid())
	{
		SubFrom = From.LoadSynchronous();
	}

	if (To.IsPending() || To.IsValid())
	{
		SubTo = To.LoadSynchronous();
	}

	if (!SubFrom)
		UE_LOG(ContentCDOHelperSubsystem, Error, TEXT("CDO_MoveRecipesFromBuilding -> Cannot load From"));

	if (!SubTo)
		UE_LOG(ContentCDOHelperSubsystem, Error, TEXT("CDO_MoveRecipesFromBuilding -> Cannot load To"));

	if (SubTo && SubFrom)
	{
		UKBFLAssetDataSubsystem* AssetDataSubsystem = UKBFLAssetDataSubsystem::Get(GetWorld());

		TArray<TSubclassOf<UFGRecipe>> Recipes;
		AssetDataSubsystem->GetRecipesOfProducer({SubFrom}, Recipes);

		for (auto Recipe : Recipes)
		{
			if (IsValid(Recipe) && !IgnoreRecipe.Contains(Recipe))
			{
				UFGRecipe* Default = GetAndStoreDefaultObject_Native<UFGRecipe>(Recipe);
				if (IsValid(Default))
				{
					if (!IgnoreCategory.Contains(Default->mOverriddenCategory) ||
						Default->mOverriddenCategory == nullptr)
					{
						Default->mProducedIn.Remove(From);
						Default->mProducedIn.Add(To);
						UE_LOG(ContentCDOHelperSubsystem, Error,
							   TEXT("CDO_MoveRecipesFromBuilding -> Move Recipe %s : %s -> %s"), *Default->GetName(),
							   *From->GetName(), *To->GetName());

						Default->MarkPackageDirty();
					}
				}
			}
		}
	}
}

void UKBFLContentCDOHelperSubsystem::BeginCDOForModule(UModModule* Module, ELifecyclePhase Phase)
{
#if WITH_EDITOR
	return;
#endif
	UE_LOGFMT(ContentCDOHelperSubsystem, Log, "BeginCDOForModule > Was Called - Mod: {0}, Phase: {1}",
			  Module->GetOwnerModReference().ToString(), Module->LifecyclePhaseToString(Phase));

	if (UKismetSystemLibrary::DoesImplementInterface(Module, UKBFLContentCDOHelperInterface::StaticClass()))
	{
		UE_LOGFMT(ContentCDOHelperSubsystem, Log,
				  "BeginCDOForModule > Module {0} implements KBFLContentCDOHelperInterface",
				  Module->GetOwnerModReference().ToString());

		if (!WasCDOForModuleCalled(Module, Phase))
		{
			UE_LOGFMT(ContentCDOHelperSubsystem, Log, "BeginCDOForModule > CDO not yet called for Phase {0} - Mod: {1}",
					  Module->LifecyclePhaseToString(Phase), Module->GetOwnerModReference().ToString());

			bool HasPhase;
			FKBFLCDOInformation Info =
				IKBFLContentCDOHelperInterface::Execute_GetCDOInformationFromPhase(Module, Phase, HasPhase);

			UE_LOGFMT(ContentCDOHelperSubsystem, Log,
					  "BeginCDOForModule > GetCDOInformationFromPhase returned HasPhase: {0} - Mod: {1}", HasPhase,
					  Module->GetOwnerModReference().ToString());

			if (HasPhase)
			{
				UE_LOGFMT(ContentCDOHelperSubsystem, Log,
						  "BeginCDOForModule > Starting CDO on Phase: {0} - Mod: {1}, CDOHelperClasses: {2}, "
						  "ItemStackSizeCDO: {3}",
						  Module->LifecyclePhaseToString(Phase), Module->GetOwnerModReference().ToString(),
						  Info.mCDOHelperClasses.Num(), Info.mItemStackSizeCDO.Num());

				DoCDOFromInfo(Info);

				if (!mCDOCalled.Contains(Module))
				{
					mCDOCalled.Add(Module);
					UE_LOGFMT(ContentCDOHelperSubsystem, Log, "BeginCDOForModule > Added Module {0} to mCDOCalled",
							  Module->GetOwnerModReference().ToString());
				}
				mCDOCalled[Module].mCalledPhases.Add(Phase);

				UE_LOGFMT(ContentCDOHelperSubsystem, Log, "BeginCDOForModule > CDO completed for Phase: {0} - Mod: {1}",
						  Module->LifecyclePhaseToString(Phase), Module->GetOwnerModReference().ToString());
			}
			else
			{
				UE_LOGFMT(ContentCDOHelperSubsystem, Log,
						  "BeginCDOForModule > Skip phase (no CDO info) - Mod: {0}, Phase: {1}",
						  Module->GetOwnerModReference().ToString(), Module->LifecyclePhaseToString(Phase));
			}
		}
		else
		{
			UE_LOGFMT(ContentCDOHelperSubsystem, Log,
					  "BeginCDOForModule > CDO Was Already Called for this phase: {0} - Mod: {1}",
					  Module->LifecyclePhaseToString(Phase), Module->GetOwnerModReference().ToString());
		}
	}
	else
	{
		UE_LOGFMT(ContentCDOHelperSubsystem, Log,
				  "BeginCDOForModule > Module {0} does NOT implement KBFLContentCDOHelperInterface, skipping",
				  Module->GetOwnerModReference().ToString());
	}
}

bool UKBFLContentCDOHelperSubsystem::WasCDOForModuleCalled(UModModule* Module, ELifecyclePhase Phase) const
{
	if (Module)
	{
		if (mCDOCalled.Contains(Module))
		{
			return mCDOCalled[Module].mCalledPhases.Contains(Phase);
		}
	}
	return false;
}

void UKBFLContentCDOHelperSubsystem::ResetCDOCallFromModule(UModModule* Module)
{
	if (Module)
	{
		if (mCDOCalled.Contains(Module))
		{
			mCDOCalled[Module].mCalledPhases.Empty();
		}
	}
}

void UKBFLContentCDOHelperSubsystem::DoCDOFromInfo(FKBFLCDOInformation Info)
{
#if WITH_EDITOR
	return;
#endif
	UE_LOGFMT(ContentCDOHelperSubsystem, Log,
			  "DoCDOFromInfo > Starting with {0} CDOHelperClasses and {1} ItemStackSizeCDO entries",
			  Info.mCDOHelperClasses.Num(), Info.mItemStackSizeCDO.Num());

	Info.mCDOHelperClasses.Sort(
		[&](const TSubclassOf<UKBFL_CDOHelperClass_Base> A, const TSubclassOf<UKBFL_CDOHelperClass_Base> B)
		{ return A.GetDefaultObject()->mCallOrder > B.GetDefaultObject()->mCallOrder; });

	UE_LOGFMT(ContentCDOHelperSubsystem, Log, "DoCDOFromInfo > CDOHelperClasses sorted by mCallOrder");

	int32 HelperIndex = 0;
	for (TSubclassOf<UKBFL_CDOHelperClass_Base> CDOHelper : Info.mCDOHelperClasses)
	{
		if (IsValid(CDOHelper))
		{
			UE_LOGFMT(ContentCDOHelperSubsystem, Log, "DoCDOFromInfo > Processing CDOHelper [{0}/{1}]: {2}",
					  HelperIndex + 1, Info.mCDOHelperClasses.Num(), CDOHelper->GetName());
			CallCDOHelper(CDOHelper);
		}
		else
		{
			UE_LOGFMT(ContentCDOHelperSubsystem, Warning, "DoCDOFromInfo > CDOHelper [{0}/{1}] is invalid, skipping",
					  HelperIndex + 1, Info.mCDOHelperClasses.Num());
		}
		HelperIndex++;
	}

	UE_LOGFMT(ContentCDOHelperSubsystem, Log,
			  "DoCDOFromInfo > Finished processing {0} CDOHelperClasses, now processing ItemStackSizeCDO",
			  Info.mCDOHelperClasses.Num());

	int32 StackSizeIndex = 0;
	for (auto CDO : Info.mItemStackSizeCDO)
	{
		UE_LOGFMT(ContentCDOHelperSubsystem, Log,
				  "DoCDOFromInfo > Processing StackSize entry [{0}/{1}]: StackSize={2}, Items={3}", StackSizeIndex + 1,
				  Info.mItemStackSizeCDO.Num(), static_cast<int32>(CDO.Key), CDO.Value.mItems.Num());

		int32 ItemIndex = 0;
		for (auto Item : CDO.Value.mItems)
		{
			if (Item)
			{
				UE_LOGFMT(ContentCDOHelperSubsystem, Log, "DoCDOFromInfo > Setting StackSize for Item [{0}/{1}]: {2}",
						  ItemIndex + 1, CDO.Value.mItems.Num(), Item->GetName());
				DoSetNewStackSize(Item, CDO.Key);
			}
			else
			{
				UE_LOGFMT(ContentCDOHelperSubsystem, Warning, "DoCDOFromInfo > Item [{0}/{1}] is null, skipping",
						  ItemIndex + 1, CDO.Value.mItems.Num());
			}
			ItemIndex++;
		}
		StackSizeIndex++;
	}

	UE_LOGFMT(ContentCDOHelperSubsystem, Log, "DoCDOFromInfo > Completed processing all CDO info");
}

void UKBFLContentCDOHelperSubsystem::DoSetNewStackSize(TSubclassOf<UFGItemDescriptor> Item, EStackSize StackSize)
{
	if (IsValid(Item))
	{
		if (UFGItemDescriptor* Default = GetAndStoreDefaultObject_Native<UFGItemDescriptor>(Item))
		{
			UE_LOG(ContentCDOHelperSubsystem, Log, TEXT("ContentCDOHelperSubsystem > DoSetNewStackSize for item %s"),
				   *Item->GetName());
			Default->mStackSize = StackSize;
			Default->mCachedStackSize = UFGItemDescriptor::GetStackSize(Item);
			Default->MarkPackageDirty();
		}
	}
}

void UKBFLContentCDOHelperSubsystem::CallCDOHelper(TSubclassOf<UKBFL_CDOHelperClass_Base> CDOHelperClass,
												   bool IgnoreCallCheck)
{
#if WITH_EDITOR
	return;
#endif
	if (IsValid(CDOHelperClass))
	{
		if (UKBFL_CDOHelperClass_Base* CDOHelper =
				GetAndStoreDefaultObject_Native<UKBFL_CDOHelperClass_Base>(CDOHelperClass))
		{
			if (CDOHelper->ExecuteAllowed() || IgnoreCallCheck)
			{
				CDOHelper->mSubsystem = this;

				if (UKBFL_CDOHelperClass_CategoryOrder* CategoryOrderHelper =
						Cast<UKBFL_CDOHelperClass_CategoryOrder>(CDOHelper))
				{
					mCategoriesToCall.Add(CategoryOrderHelper);
					UE_LOG(ContentCDOHelperSubsystem, Log, TEXT("Defer call for: %s"), *CDOHelper->GetName());
					return;
				}

				CDOHelper->ModifyValues();
				CDOHelper->DoCDO();
				CDOHelper->ExecuteBlueprintCDO();
				UE_LOG(ContentCDOHelperSubsystem, Log, TEXT("Do CDO Call for Helper: %s"), *CDOHelper->GetName());
			}
		}
	}
}

UObject* UKBFLContentCDOHelperSubsystem::GetAndStoreDefaultObject(UClass* Class)
{
	if (IsValid(Class))
	{
		mCalledClasses.AddUnique(Class);
		mCalledObjects.AddUnique(Class->GetDefaultObject());
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

void UKBFLContentCDOHelperSubsystem::StoreClass(UClass* Class) { mCalledClasses.AddUnique(Class); }

void UKBFLContentCDOHelperSubsystem::StoreObject(UObject* Object) { mCalledObjects.AddUnique(Object); }

void UKBFLContentCDOHelperSubsystem::RemoveClass(UClass* Class) { mCalledClasses.Remove(Class); }

void UKBFLContentCDOHelperSubsystem::RemoveObject(UObject* Object) { mCalledObjects.Remove(Object); }

TArray<UModModule*> UKBFLContentCDOHelperSubsystem::GetCalledModules() const
{
	TArray<UModModule*> Modules;
	mCDOCalled.GenerateKeyArray(Modules);
	return Modules;
}
