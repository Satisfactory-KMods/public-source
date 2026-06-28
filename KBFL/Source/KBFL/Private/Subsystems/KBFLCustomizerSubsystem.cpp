#include "Subsystems/KBFLCustomizerSubsystem.h"

#include "BFL/KBFL_Player.h"
#include "FGBuildableSubsystem.h"
#include "FGFactoryColoringTypes.h"
#include "FGGameMode.h"
#include "FGGameState.h"
#include "KBFLLogging.h"
#include "Kismet/GameplayStatics.h"
#include "Module/WorldModuleManager.h"
#include "Subsystem/SubsystemActorManager.h"
#include "Subsystems/KBFLAssetDataSubsystem.h"
#include "Subsystems/KBFLContentCDOHelperSubsystem.h"


void UKBFLCustomizerSubsystem::Tick(float DeltaTime)
{

	if (bInitialized && !bDefaultGathered)
	{
		bInitialized = false;
		mGameMode = Cast<AFGGameMode>(GetWorld()->GetAuthGameMode());
		if (!bInitialized && !(mGameMode != nullptr && mGameMode->IsMainMenuGameMode()))
		{
			if (!bDefaultGathered)
			{
				UE_LOGFMT(CustomizerSubsystem, Log, "CustomizerSubsystem > RegisterCollectionsInSubsystem");
				bDefaultGathered = GatherDefaultCollections();
			}

			UE_LOGFMT(CustomizerSubsystem, Log, "CustomizerSubsystem > GatherInterfaces");
			GatherProviders();

			bInitialized = true;
		}
	}
}

TStatId UKBFLCustomizerSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UKBFLCustomizerSubsystem, STATGROUP_Tickables);
}

void UKBFLCustomizerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Collection.InitializeDependency(USubsystemActorManager::StaticClass());
	UE_LOGFMT(CustomizerSubsystem, Log, "Initialize Subsystem");


	Super::Initialize(Collection);
}

void UKBFLCustomizerSubsystem::Deinitialize()
{
	if (mDefaultSwatchCollection)
	{
		for (auto SwatchMap : mSwatchIDMap)
		{
			UFGFactoryCustomizationCollection* Default = mDefaultSwatchCollection.GetDefaultObject();
			if (Default)
			{
				if (Default->mCustomizations.Contains(SwatchMap.Value))
				{
					Default->mCustomizations.Remove(SwatchMap.Value);
				}
			}
		}
	}

	mCustomizationProviders.Empty();
	mSwatchIDMap.Empty();
	bInitialized = false;
	bDefaultGathered = false;

	Super::Deinitialize();
}

void UKBFLCustomizerSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	AFGGameMode* GameMode = Cast<AFGGameMode>(GetWorld()->GetAuthGameMode());
	if (!bInitialized && !(GameMode != nullptr && GameMode->IsMainMenuGameMode()))
	{
		if (!bDefaultGathered)
		{
			UE_LOGFMT(CustomizerSubsystem, Log, "CustomizerSubsystem > RegisterCollectionsInSubsystem");
			GatherDefaultCollections();
		}

		UE_LOGFMT(CustomizerSubsystem, Log, "CustomizerSubsystem > GatherProviders");
		bInitialized = GatherProviders();
	}
}

bool UKBFLCustomizerSubsystem::RegisterSwatchesInSubsystem(
	const TMap<TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch>, FKBFLSwatchInformation>& SwatchMap)
{
	AFGBuildableSubsystem* Subsystem = AFGBuildableSubsystem::Get(this);
	UKBFLContentCDOHelperSubsystem* CDOHelperSubsystem = UKBFLContentCDOHelperSubsystem::Get(this);
	AFGGameState* FGGameState = Cast<AFGGameState>(UGameplayStatics::GetGameState(this));

	if (Subsystem && FGGameState && CDOHelperSubsystem)
	{
		// --- Conflict Resolver ---
		// Reassign any swatch whose CDO ID is in SF's reserved range (0-27) or collides with an already-registered
		// swatch. Modifies the CDO in place so the loop below always sees a safe, unique ID.
		{
			TSet<int32> TakenSlots;
			for (const auto& Pair : mSwatchIDMap)
			{
				TakenSlots.Add(Pair.Key);
			}

			for (const auto& Map : SwatchMap)
			{
				TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch> SwatchClass = Map.Key;
				if (!IsValid(SwatchClass))
				{
					continue;
				}
				UFGFactoryCustomizationDescriptor_Swatch* SwatchCDO = SwatchClass.GetDefaultObject();
				if (!IsValid(SwatchCDO))
				{
					continue;
				}

				const int32 OriginalID = SwatchCDO->ID;
				if (OriginalID <= 27 || TakenSlots.Contains(OriginalID))
				{
					int32 NewID = FMath::Max(28, OriginalID + 1);
					while (TakenSlots.Contains(NewID) || NewID == 255)
					{
						++NewID;
					}
					const FString ConflictReason = (OriginalID <= 27) ? TEXT("reserved by SF") : TEXT("duplicate");
					UE_LOGFMT(CustomizerSubsystem, Warning,
							  "Swatch ID conflict resolved: {0} requested slot {1} ({2}) - auto-reassigned to {3}.",
							  SwatchClass->GetName(), OriginalID, ConflictReason, NewID);
					SwatchCDO->ID = NewID;
				}

				TakenSlots.Add(SwatchCDO->ID);
			}
		}
		// --- End Conflict Resolver ---

		for (const TPair<TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch>, FKBFLSwatchInformation>& Map :
			 SwatchMap)
		{
			TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch> SwatchClass = Map.Key;
			FKBFLSwatchInformation Swatch = Map.Value;
			if (!IsValid(SwatchClass))
			{
				continue;
			}

			UFGFactoryCustomizationDescriptor_Swatch* SwatchDefauls = SwatchClass.GetDefaultObject();
			if (!IsValid(SwatchDefauls))
			{
				continue;
			}

			int32 ColourIndex = SwatchDefauls->ID;
			SwatchDefauls->mMenuPriority = static_cast<float>(ColourIndex);
			if (!mSwatchIDMap.Find(ColourIndex))
			{
				if (mDefaultSwatchCollection)
				{
					UFGFactoryCustomizationCollection* Default =
						CDOHelperSubsystem->GetAndStoreDefaultObject_Native<UFGFactoryCustomizationCollection>(
							mDefaultSwatchCollection);
					if (IsValid(Default))
					{
						Default->mCustomizations.AddUnique(SwatchClass);
					}
				}

				if (!Subsystem->mColorSlots_Data.IsValidIndex(ColourIndex))
				{
					UE_LOGFMT(CustomizerSubsystem, Log, "Try to add new color Slot at index, {0} - {1}", ColourIndex,
							  Subsystem->mColorSlots_Data.Num());
					for (int32 i = Subsystem->mColorSlots_Data.Num(); i <= ColourIndex; ++i)
					{
						// Defaults
						FFactoryCustomizationColorSlot NewColourSlot =
							FFactoryCustomizationColorSlot(Swatch.mPrimaryColour, Swatch.mSecondaryColour);
						NewColourSlot.PaintFinish =
							TSubclassOf<UFGFactoryCustomizationDescriptor_PaintFinish>(SwatchClass);

						// Add to Array
						Subsystem->mColorSlots_Data.Add(NewColourSlot);
						FGGameState->SetupColorSlots_Data(Subsystem->mColorSlots_Data);
						FGGameState->Server_SetBuildingColorDataForSlot(i, NewColourSlot);

						FTimerDelegate TimerDel;
						FTimerHandle TimerHandle;
						TimerDel.BindUFunction(Subsystem, FName("SetColorSlot_Data"), i, NewColourSlot);
						GetWorld()->GetTimerManager().SetTimer(TimerHandle, TimerDel, 2.0f, false);
						TimerDel.BindUFunction(FGGameState, FName("Server_SetBuildingColorDataForSlot"), i,
											   NewColourSlot);
						GetWorld()->GetTimerManager().SetTimer(TimerHandle, TimerDel, 2.0f, false);

						Subsystem->SetColorSlot_Data(i, NewColourSlot);

						// Mark Slots as Dirty
						Subsystem->mColorSlotsAreDirty = true;

						UE_LOGFMT(CustomizerSubsystem, Log, "New Colour slot added: {0} :: {1}", i,
								  SwatchClass->GetName());
					}
				}

				// Add to Array
				if (!FGGameState->mBuildingColorSlots_Data.IsValidIndex(ColourIndex))
				{
					FGGameState->mBuildingColorSlots_Data.SetNum(ColourIndex + 1, EAllowShrinking::No);
					FGGameState->mBuildingColorSlots_Data[ColourIndex] = Subsystem->mColorSlots_Data[ColourIndex];
					FGGameState->SetupColorSlots_Data(Subsystem->mColorSlots_Data);
					UE_LOGFMT(CustomizerSubsystem, Log, "write color again to gamestate: {0} / {1}",
							  FGGameState->mBuildingColorSlots_Data.Num(), Subsystem->mColorSlots_Data.Num());
				}

				mSwatchIDMap.Add(ColourIndex, SwatchClass);
				FGGameState->SetupColorSlots_Data(Subsystem->mColorSlots_Data);
				Subsystem->mColorSlotsAreDirty = true;
				UE_LOGFMT(CustomizerSubsystem, Log, "Swatch found and success: {0} > {1} ({2}/{3})", ColourIndex,
						  SwatchClass->GetName(), FGGameState->mBuildingColorSlots_Data.Num(),
						  Subsystem->mColorSlots_Data.Num());
			}
			else
			{
				UE_LOGFMT(CustomizerSubsystem, Warning,
						  "Duplicate Swatch ID (post-resolve): {0} | {1} >< {2} | {3} - skipping.",
						  SwatchClass->GetName(), SwatchClass.GetDefaultObject()->ID,
						  mSwatchIDMap[ColourIndex]->GetName(), ColourIndex);
				continue;
			}

			// Safety check - should never fire after the resolver above
			if (!(ColourIndex > 27))
			{
				UE_LOGFMT(CustomizerSubsystem, Warning,
						  "Swatch {0} still has reserved SF slot {1} after conflict resolution - skipping.",
						  SwatchClass->GetName(), ColourIndex);
				continue;
			}
		}

		TArray<FFactoryCustomizationColorSlot> ColorSlots = Subsystem->mColorSlots_Data;
		UE_LOGFMT(CustomizerSubsystem, Error, "Slots: {0}", ColorSlots.Num());
		FGGameState->SetupColorSlots_Data(ColorSlots);
		// FGGameState->Init();
		Subsystem->mColorSlotsAreDirty = true;
		for (const TPair<TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch>, FKBFLSwatchInformation>& Map :
			 SwatchMap)
		{
			TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch> SwatchClass = Map.Key;
			FKBFLSwatchInformation Swatch = Map.Value;
			if (!IsValid(SwatchClass))
			{
				continue;
			}

			UFGFactoryCustomizationDescriptor_Swatch* SwatchDefauls = SwatchClass.GetDefaultObject();
			if (!IsValid(SwatchDefauls))
			{
				continue;
			}

			int32 ColourIndex = SwatchDefauls->ID;
			SwatchDefauls->mMenuPriority = static_cast<float>(ColourIndex);
			if (!FGGameState->mBuildingColorSlots_Data.IsValidIndex(ColourIndex))
			{
				FGGameState->mBuildingColorSlots_Data.SetNum(ColourIndex + 1, EAllowShrinking::No);
				FGGameState->mBuildingColorSlots_Data[ColourIndex] = Subsystem->mColorSlots_Data[ColourIndex];
				FGGameState->SetupColorSlots_Data(Subsystem->mColorSlots_Data);
				UE_LOGFMT(CustomizerSubsystem, Log, "write color again to gamestate: {0} / {1}",
						  FGGameState->mBuildingColorSlots_Data.Num(), Subsystem->mColorSlots_Data.Num());
			}
		}

		return true;
	}

	UE_LOGFMT(CustomizerSubsystem, Error, "Cannot load AFGBuildableSubsystem for Swatches");
	return false;
}

bool UKBFLCustomizerSubsystem::RegisterSwatchGroups(
	TMap<TSubclassOf<UFGSwatchGroup>, TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch>> Map)
{
	for (TTuple<TSubclassOf<UFGSwatchGroup>, TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch>> Group : Map)
	{
		if (Group.Key && Group.Value)
		{
			if (!SetDefaultToSwatchGroup(Group.Key, Group.Value))
			{
				return false;
			}
		}
	}
	return true;
}

void UKBFLCustomizerSubsystem::CDOMaterials(TArray<FKBFLMaterialDescriptorInformation> CDOInformation)
{
	UKBFLContentCDOHelperSubsystem* CDOHelperSubsystem = UKBFLContentCDOHelperSubsystem::Get(this);
	if (CDOInformation.Num() > 0 && CDOHelperSubsystem)
	{
		for (FKBFLMaterialDescriptorInformation Information : CDOInformation)
		{
			if (Information.mApplyThisInformationTo.Num() > 0)
			{
				for (auto Descriptor : Information.mApplyThisInformationTo)
				{
					if (Descriptor)
					{
						if (UFGFactoryCustomizationDescriptor_Material* Default =
								CDOHelperSubsystem
									->GetAndStoreDefaultObject_Native<UFGFactoryCustomizationDescriptor_Material>(
										Descriptor))
						{
							if (Information.mBuildableMap.Num() > 0)
							{
								Default->mBuildableMap.Append(Information.mBuildableMap);
							}
							if (Information.mValidBuildables.Num() > 0)
							{
								Default->mValidBuildables.Append(Information.mValidBuildables);
							}
						}
					}
				}
			}
		}
	}
}
void UKBFLCustomizerSubsystem::BeginForProvider(UKBFLCustomizationProvider* Provider)
{
	if (!IsValid(Provider))
	{
		return;
	}

	if (!bDefaultGathered)
	{
		bDefaultGathered = GatherDefaultCollections();
	}

	UE_LOGFMT(CustomizerSubsystem, Log, "RegisterCollection {0} in Subsystem", Provider->GetFullName());
	RegisterSwatchesInSubsystem(Provider->mSwatchInformation);

	if (bDefaultGathered)
	{
		RegisterSwatchGroups(Provider->mSwatchGroups);
		CDOMaterials(Provider->mMaterialInformation);
	}
	else
	{
		const FTimerDelegate RespawnDelegate =
			FTimerDelegate::CreateUObject(this, &UKBFLCustomizerSubsystem::BeginForProvider, Provider);
		GetWorld()->GetTimerManager().SetTimerForNextTick(RespawnDelegate);
		UE_LOGFMT(CustomizerSubsystem, Log, "Gather was invalid Retry next tick!");
	}
}


bool UKBFLCustomizerSubsystem::GatherDefaultCollections()
{
	if (AFGBuildableSubsystem* Subsystem = AFGBuildableSubsystem::Get(this))
	{
		for (auto CustomizationClass : Subsystem->mCustomizationCollectionClasses)
		{
			if (CustomizationClass.GetDefaultObject()->mCustomizationClass->IsChildOf(
					UFGFactoryCustomizationDescriptor_Material::StaticClass()))
			{
				mDefaultMaterialCollection = CustomizationClass;
			}
			if (CustomizationClass.GetDefaultObject()->mCustomizationClass->IsChildOf(
					UFGFactoryCustomizationDescriptor_Pattern::StaticClass()))
			{
				mDefaultPatternCollection = CustomizationClass;
			}
			if (CustomizationClass.GetDefaultObject()->mCustomizationClass->IsChildOf(
					UFGFactoryCustomizationDescriptor_Swatch::StaticClass()))
			{
				mDefaultSwatchCollection = CustomizationClass;
			}
			if (CustomizationClass.GetDefaultObject()->mCustomizationClass->IsChildOf(
					UFGFactoryCustomizationDescriptor_Skin::StaticClass()))
			{
				mDefaultSkinCollection = CustomizationClass;
			}
		}

		return true;
	}

	UE_LOGFMT(CustomizerSubsystem, Error, "Cannot load AFGBuildableSubsystem for Collections");
	return false;
}

bool UKBFLCustomizerSubsystem::GatherProviders()
{
#if !WITH_EDITOR
	mCustomizationProviders.Empty();
	if (!IsValid(GetWorld()) || !IsValid(GetWorld()->GetGameInstance()))
		return false;

	UKBFLAssetDataSubsystem* AssetDataSubsystem = UKBFLAssetDataSubsystem::Get(GetWorld());

	// Use temporary raw pointer set for compatibility
	AssetDataSubsystem->FindAllDataAssetsOfClass(mCustomizationProviders);

	for (TObjectPtr<UKBFLCustomizationProvider> CustomizationProvider : mCustomizationProviders)
	{
		BeginForProvider(CustomizationProvider);
	}
	return true;
#endif
	return false;
}

bool UKBFLCustomizerSubsystem::SetDefaultToSwatchGroup(TSubclassOf<UFGSwatchGroup> SwatchGroup,
													   TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch> Swatch)
{
	UE_LOGFMT(CustomizerSubsystem, Log, "SetDefaultToSwatchGroup");
	AFGBuildableSubsystem* Subsystem = AFGBuildableSubsystem::Get(this);
	AFGGameState* GameState = Cast<AFGGameState>(UGameplayStatics::GetGameState(this));

	if (Subsystem && GameState && Swatch && SwatchGroup)
	{
		FSwatchGroupData NewGroup;
		NewGroup.Swatch = Swatch;
		NewGroup.SwatchGroup = SwatchGroup;

		bool Contain = false;
		for (auto Group : GameState->mSwatchGroupDatum)
		{
			if (Group.SwatchGroup == SwatchGroup)
			{
				Contain = true;
			}
		}

		if (!Contain)
		{
			GameState->mSwatchGroupDatum.Add(NewGroup);
			GameState->SetDefaultSwatchForBuildableGroup(SwatchGroup, Swatch);

			if (AFGPlayerController* PlayerController = UKBFL_Player::GetFGController(this))
			{
				PlayerController->SetDefaultSwatchForBuildableGroup(SwatchGroup, Swatch);
			}

			Subsystem->mColorSlotsAreDirty = true;
			UE_LOGFMT(CustomizerSubsystem, Log, "Swatch Group found and success added: {0} > {1}",
					  SwatchGroup->GetName(), Swatch->GetName());
		}

		UE_LOGFMT(CustomizerSubsystem, Log, "Swatch Group found: {0} > {1}", SwatchGroup->GetName(), Swatch->GetName());
		return true;
	}

	UE_LOGFMT(CustomizerSubsystem, Log, "Subsystem && GameState && Swatch && SwatchGroup something is invalid!");
	return false;
}
TMap<int32, TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch>> UKBFLCustomizerSubsystem::GetSwatchMap() const
{
	return mSwatchIDMap;
}
