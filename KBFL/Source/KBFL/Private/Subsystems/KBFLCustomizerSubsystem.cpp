#include "Subsystems/KBFLCustomizerSubsystem.h"

#include "BFL/KBFL_Player.h"
#include "Dom/JsonObject.h"
#include "FGBuildableSubsystem.h"
#include "FGFactoryColoringTypes.h"
#include "FGGameMode.h"
#include "FGGameState.h"
#include "FGSaveSession.h"
#include "HAL/PlatformFileManager.h"
#include "KBFLLogging.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Module/WorldModuleManager.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Subsystem/SubsystemActorManager.h"
#include "Subsystems/KBFLAssetDataSubsystem.h"
#include "Subsystems/KBFLContentCDOHelperSubsystem.h"


void UKBFLCustomizerSubsystem::Tick(float DeltaTime)
{
	// Try to patch swatches once World and GameInstance are available
	if (!bSwatchesPatched)
	{
		UWorld* World = GetWorld();
		if (World && World->GetGameInstance())
		{
			UE_LOGFMT(CustomizerSubsystem, Log, "CustomizerSubsystem > World ready, calling TryToPatchSwatches");
			TryToPatchSwatches();
			bSwatchesPatched = true;
		}
	}

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

	LoadSaved();

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
	bSwatchesPatched = false;

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
					FGGameState->mBuildingColorSlots_Data.SetNum(ColourIndex + 1, false);
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
				UE_LOGFMT(CustomizerSubsystem, Fatal, "Duplicate Swatch ID: {0} | {1} >< {2} | {3}",
						  SwatchClass->GetName(), SwatchClass.GetDefaultObject()->ID,
						  mSwatchIDMap[ColourIndex]->GetName(), ColourIndex);
			}

			// Ignore Slots used by CSS (Slot 16 for example is used twice)
			if (!(ColourIndex > 27))
			{
				UE_LOGFMT(CustomizerSubsystem, Fatal, "Please use a Index bigger as 27 (Dont use Slots from SF!)");
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
				FGGameState->mBuildingColorSlots_Data.SetNum(ColourIndex + 1, false);
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

void UKBFLCustomizerSubsystem::SaveDirty()
{
	FString FilePath = GetSwatchSavePath();
	SaveSwatchArrayToFile(mSavedSwatches, FilePath);
}

void UKBFLCustomizerSubsystem::LoadSaved()
{
	FString FilePath = GetSwatchSavePath();
	LoadSwatchArrayFromFile(mSavedSwatches, FilePath);
}

void UKBFLCustomizerSubsystem::TryToPatchSwatches()
{
#if WITH_EDITOR
	return;
#endif

	// World and GameInstance are guaranteed to be valid here (checked in Tick)
	UWorld* World = GetWorld();
	UGameInstance* GameInstance = World->GetGameInstance();

	// Get the AssetDataSubsystem
	UKBFLAssetDataSubsystem* AssetDataSubsystem = GameInstance->GetSubsystem<UKBFLAssetDataSubsystem>();
	if (!AssetDataSubsystem)
	{
		UE_LOGFMT(CustomizerSubsystem, Error,
				  "TryToPatchSwatches: AssetDataSubsystem not available - this should not happen!");
		return;
	}

	TArray<TSubclassOf<UObject>> OutObjects;
	AssetDataSubsystem->GetObjectsOfChilds({UFGFactoryCustomizationDescriptor_Swatch::StaticClass()}, OutObjects);
	AssetDataSubsystem->GetObjectsOfChilds({UFGFactoryCustomizationDescriptor_Swatch::StaticClass()}, OutObjects, true);

	// Sort by path hash to ensure deterministic order for consistent Swatch IDs
	OutObjects.Sort(
		[](const TSubclassOf<UObject>& A, const TSubclassOf<UObject>& B)
		{
			FString PathA = A ? A->GetPathName() : FString();
			FString PathB = B ? B->GetPathName() : FString();
			return PathA < PathB;
		});

	TSet<TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch>> Swatches;
	for (TSubclassOf<UObject> OutObject : OutObjects)
	{
		TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch> AsSwatch{OutObject};
		if (IsValid(AsSwatch))
		{
			Swatches.Add(AsSwatch);
		}
		if (!IsBaseGameSwatch(AsSwatch))
		{
			continue;
		}
		PatchSwatch(AsSwatch);
	}

	AFGGameState* FGGameState = Cast<AFGGameState>(UGameplayStatics::GetGameState(this));
	AFGBuildableSubsystem* Subsystem = AFGBuildableSubsystem::Get(this);

	for (TSubclassOf<UObject> OutObject : OutObjects)
	{
		TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch> AsSwatch{OutObject};
		if (IsBaseGameSwatch(AsSwatch))
		{
			continue;
		}
		PatchSwatch(AsSwatch);

		UFGFactoryCustomizationDescriptor_Swatch* SwatchDefauls =
			AsSwatch->GetDefaultObject<UFGFactoryCustomizationDescriptor_Swatch>();
		if (SwatchDefauls && Subsystem && FGGameState)
		{
			int32 ColourIndex = SwatchDefauls->ID;
			SwatchDefauls->mMenuPriority = static_cast<float>(ColourIndex);

			// Get default color slot
			FFactoryCustomizationColorSlot NewColourSlot = FFactoryCustomizationColorSlot();
			NewColourSlot.PaintFinish = TSubclassOf<UFGFactoryCustomizationDescriptor_PaintFinish>(AsSwatch);

			UE_LOGFMT(CustomizerSubsystem, Log, "Creating NewColourSlot for Swatch: {0} (Index: {1})",
					  AsSwatch->GetName(), ColourIndex);

			if (UKBFLactoryCustomizationDescriptor_Swatch* SwatchInterface =
					Cast<UKBFLactoryCustomizationDescriptor_Swatch>(SwatchDefauls))
			{
				NewColourSlot = SwatchInterface->mDefaultColorSlot;
				UE_LOGFMT(CustomizerSubsystem, Log,
						  "  Using KBFL Swatch ColorSlot - Primary: (R:{0} G:{1} B:{2} A:{3}), Secondary: (R:{4} G:{5} "
						  "B:{6} A:{7})",
						  NewColourSlot.PrimaryColor.R, NewColourSlot.PrimaryColor.G, NewColourSlot.PrimaryColor.B,
						  NewColourSlot.PrimaryColor.A, NewColourSlot.SecondaryColor.R, NewColourSlot.SecondaryColor.G,
						  NewColourSlot.SecondaryColor.B, NewColourSlot.SecondaryColor.A);
			}
			else if (UFGFactoryCustomizationDescriptor_PaintFinish* SwatchFinishedInterface =
						 Cast<UFGFactoryCustomizationDescriptor_PaintFinish>(SwatchDefauls))
			{
				NewColourSlot = FFactoryCustomizationColorSlot(SwatchFinishedInterface->mForcedColor,
															   SwatchFinishedInterface->mForcedColor);
				UE_LOGFMT(CustomizerSubsystem, Log, "  Using PaintFinish ColorSlot - Color: (R:{0} G:{1} B:{2} A:{3})",
						  SwatchFinishedInterface->mForcedColor.R, SwatchFinishedInterface->mForcedColor.G,
						  SwatchFinishedInterface->mForcedColor.B, SwatchFinishedInterface->mForcedColor.A);
			}
			else
			{
				UE_LOGFMT(CustomizerSubsystem, Log, "  Using default ColorSlot (empty)");
			}

			// Check and create missing slots in Subsystem
			if (!Subsystem->mColorSlots_Data.IsValidIndex(ColourIndex))
			{
				UE_LOGFMT(CustomizerSubsystem, Log, "Try to add new color Slot at index, {0} - {1}", ColourIndex,
						  Subsystem->mColorSlots_Data.Num());
				for (int32 i = Subsystem->mColorSlots_Data.Num(); i <= ColourIndex; ++i)
				{
					// Use the NewColourSlot for this specific swatch, or create a default for intermediate slots
					FFactoryCustomizationColorSlot SlotToAdd =
						(i == ColourIndex) ? NewColourSlot : FFactoryCustomizationColorSlot();

					if (i == ColourIndex)
					{
						UE_LOGFMT(CustomizerSubsystem, Log,
								  "  Adding NewColourSlot at index {0} - Primary: (R:{1} G:{2} B:{3} A:{4}), "
								  "Secondary: (R:{5} G:{6} B:{7} A:{8})",
								  i, SlotToAdd.PrimaryColor.R, SlotToAdd.PrimaryColor.G, SlotToAdd.PrimaryColor.B,
								  SlotToAdd.PrimaryColor.A, SlotToAdd.SecondaryColor.R, SlotToAdd.SecondaryColor.G,
								  SlotToAdd.SecondaryColor.B, SlotToAdd.SecondaryColor.A);
					}
					else
					{
						UE_LOGFMT(CustomizerSubsystem, Log, "  Adding default slot at index {0} (intermediate)", i);
					}

					// Add to Array
					Subsystem->mColorSlots_Data.Add(SlotToAdd);
					FGGameState->SetupColorSlots_Data(Subsystem->mColorSlots_Data);
					FGGameState->Server_SetBuildingColorDataForSlot(i, SlotToAdd);

					FTimerDelegate TimerDel;
					FTimerHandle TimerHandle;
					TimerDel.BindUFunction(Subsystem, FName("SetColorSlot_Data"), i, SlotToAdd);
					World->GetTimerManager().SetTimer(TimerHandle, TimerDel, 10.0f, false);
					TimerDel.BindUFunction(FGGameState, FName("Server_SetBuildingColorDataForSlot"), i, SlotToAdd);
					World->GetTimerManager().SetTimer(TimerHandle, TimerDel, 10.0f, false);

					Subsystem->SetColorSlot_Data(i, SlotToAdd);

					// Mark Slots as Dirty
					Subsystem->mColorSlotsAreDirty = true;

					UE_LOGFMT(CustomizerSubsystem, Log, "New Colour slot added: {0} {1}", i, AsSwatch->GetName());
				}
			}


			// Ensure GameState has the slot
			if (!FGGameState->mBuildingColorSlots_Data.IsValidIndex(ColourIndex))
			{
				FGGameState->mBuildingColorSlots_Data.SetNum(ColourIndex + 1, false);
				FGGameState->mBuildingColorSlots_Data[ColourIndex] = Subsystem->mColorSlots_Data[ColourIndex];
				FGGameState->SetupColorSlots_Data(Subsystem->mColorSlots_Data);
				UE_LOGFMT(CustomizerSubsystem, Log, "write color again to gamestate: {0} / {1}",
						  FGGameState->mBuildingColorSlots_Data.Num(), Subsystem->mColorSlots_Data.Num());
			}

			if (UFGFactoryCustomizationDescriptor_PaintFinish* SwatchFinishedInterface =
					Cast<UFGFactoryCustomizationDescriptor_PaintFinish>(AsSwatch->GetDefaultObject()))
			{
				FFactoryCustomizationColorSlot NewColourSlotFinished = FFactoryCustomizationColorSlot(
					SwatchFinishedInterface->mForcedColor, SwatchFinishedInterface->mForcedColor);
				NewColourSlotFinished.PaintFinish =
					TSubclassOf<UFGFactoryCustomizationDescriptor_PaintFinish>(AsSwatch);

				UE_LOGFMT(CustomizerSubsystem, Log,
						  "PaintFinish Override for Index {0} - Color: (R:{1} G:{2} B:{3} A:{4}), PaintFinish: {5}",
						  ColourIndex, SwatchFinishedInterface->mForcedColor.R, SwatchFinishedInterface->mForcedColor.G,
						  SwatchFinishedInterface->mForcedColor.B, SwatchFinishedInterface->mForcedColor.A,
						  AsSwatch->GetName());

				Subsystem->mColorSlots_Data[ColourIndex] = NewColourSlotFinished;
				FGGameState->mBuildingColorSlots_Data[ColourIndex] = NewColourSlotFinished;
				FGGameState->SetupColorSlots_Data(Subsystem->mColorSlots_Data);
				Subsystem->mColorSlotsAreDirty = true;

				FTimerDelegate TimerDel;
				FTimerDelegate TimerDel2;
				FTimerHandle TimerHandle;
				FTimerHandle TimerHandle2;
				TimerDel.BindUFunction(Subsystem, FName("SetColorSlot_Data"), ColourIndex, NewColourSlotFinished);
				World->GetTimerManager().SetTimer(TimerHandle, TimerDel, 10.0f, false);
				TimerDel2.BindUFunction(FGGameState, FName("Server_SetBuildingColorDataForSlot"), ColourIndex,
										NewColourSlotFinished);
				World->GetTimerManager().SetTimer(TimerHandle2, TimerDel2, 10.0f, false);
			}
		}
	}

	Swatches.Sort([](const TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch>& A,
					 const TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch>& B)
				  { return A.GetDefaultObject()->ID < B.GetDefaultObject()->ID; });

	int32 SwatchCounter = 1;
	for (TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch> Swatch : Swatches)
	{
		UFGFactoryCustomizationDescriptor_Swatch* SwatchDefault = Swatch.GetDefaultObject();
		FText SwatchName = UFGItemDescriptor::GetItemName(Swatch);

		// If the SwatchName starts with "Swatch <number>", we will overwrite the name with "Swatch <SwatchCounter>"
		FString SwatchNameString = SwatchName.ToString();
		if (SwatchNameString.StartsWith("Swatch "))
		{
			int32 SpaceIndex;
			if (SwatchNameString.FindChar(' ', SpaceIndex))
			{
				FString AfterSpace = SwatchNameString.Mid(SpaceIndex + 1);
				if (AfterSpace.IsNumeric())
				{
					// Use localizable text format for "Swatch {0}"
					FText LocalizedSwatchName = FText::Format(
						NSLOCTEXT("KBFLCustomizer", "SwatchNameFormat", "Swatch {0}"), FText::AsNumber(SwatchCounter));

					SwatchDefault->mDisplayName = LocalizedSwatchName;
					SwatchDefault->mMenuPriority = static_cast<float>(SwatchCounter);
					SwatchCounter++;
					UE_LOGFMT(CustomizerSubsystem, Log, "Renamed {0} Swatch to: {1}", Swatch->GetName(),
							  LocalizedSwatchName.ToString());
				}
			}
		}
	}

	SaveDirty();
}

void UKBFLCustomizerSubsystem::PatchSwatch(TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch> Swatch)
{
	UFGFactoryCustomizationDescriptor_Swatch* SwatchDefault = Swatch.GetDefaultObject();

	int32 BiggestKnownSwatch = 0;
	for (const FKBFPSwatchSaveInformation& SavedSwatch : mSavedSwatches)
	{
		TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch> SavedSwatchClass = SavedSwatch.LoadClass();
		if (SavedSwatchClass == Swatch)
		{
			if (SavedSwatch.bIsBaseGame)
			{
				return;
			}
			SwatchDefault->ID = SavedSwatch.mSwatchID;
			UE_LOGFMT(CustomizerSubsystem, Log, "Swatch ID found for {0} > {1}", Swatch->GetName(), SwatchDefault->ID);
			return;
		}

		if (IsValid(SavedSwatchClass))
		{
			UFGFactoryCustomizationDescriptor_Swatch* SavedSwatchDefault = SavedSwatchClass.GetDefaultObject();
			if (SavedSwatchDefault->ID != 255)
			{
				BiggestKnownSwatch = FMath::Max(BiggestKnownSwatch, SavedSwatchDefault->ID);
			}
		}
	}

	BiggestKnownSwatch++;
	BiggestKnownSwatch = BiggestKnownSwatch == 255 ? BiggestKnownSwatch + 1 : BiggestKnownSwatch;

	FKBFPSwatchSaveInformation NewSave;
	NewSave.mSwatchID = IsBaseGameSwatch(Swatch) ? SwatchDefault->ID : BiggestKnownSwatch;
	NewSave.mPath = Swatch->GetPathName();
	NewSave.mModName = GetModNameFromPath(Swatch->GetPathName());
	NewSave.bIsBaseGame = IsBaseGameSwatch(Swatch);
	SwatchDefault->ID = NewSave.mSwatchID;
	mSavedSwatches.Add(NewSave);
}

FString UKBFLCustomizerSubsystem::GetModNameFromPath(FString modPath)
{
	return modPath.Split(TEXT("/"), nullptr, &modPath) ? modPath : FString();
}
bool UKBFLCustomizerSubsystem::IsBaseGameSwatch(const TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch>& SwatchInfo)
{
	return SwatchInfo->GetPathName().StartsWith("/Game/");
}

// ========== Custom Save/Load Implementation for FKBFPSwatchSaveInformation ==========

FString UKBFLCustomizerSubsystem::GetSwatchSavePath() const
{
	FString JsonName = TEXT("swatches.json");
	if (IsValid(GetWorld()))
	{
		UFGSaveSession* SaveSession = UFGSaveSession::Get(GetWorld());
		if (IsValid(SaveSession))
		{
			FString Id = SaveSession->GetSaveIdentifier();
			if (!Id.IsEmpty())
			{
				FString BasePath = FPaths::ProjectSavedDir() / TEXT("ModData") / TEXT("KBFL") / Id;
				FPaths::NormalizeDirectoryName(BasePath);
				return BasePath / JsonName;
			}
		}
	}

	FString BasePath = FPaths::ProjectSavedDir() / TEXT("ModData") / TEXT("KBFL");
	FPaths::NormalizeDirectoryName(BasePath);
	return BasePath / TEXT("swatches.json");
}

bool UKBFLCustomizerSubsystem::SaveSwatchArrayToFile(const TArray<FKBFPSwatchSaveInformation>& Swatches,
													 const FString& FilePath)
{
	if (FilePath.IsEmpty())
	{
		UE_LOGFMT(CustomizerSubsystem, Error, "SaveSwatchArrayToFile: FilePath is empty!");
		return false;
	}

	// Ensure directory exists
	FString Directory = FPaths::GetPath(FilePath);
	if (!Directory.IsEmpty())
	{
		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
		if (!PlatformFile.DirectoryExists(*Directory))
		{
			if (!PlatformFile.CreateDirectoryTree(*Directory))
			{
				UE_LOGFMT(CustomizerSubsystem, Error, "SaveSwatchArrayToFile: Failed to create directory: {0}",
						  Directory);
				return false;
			}
		}
	}

	// Build JSON array using FJsonObject
	TArray<TSharedPtr<FJsonValue>> JsonArray;

	for (const FKBFPSwatchSaveInformation& Swatch : Swatches)
	{
		TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();

		JsonObject->SetNumberField(TEXT("mSwatchID"), Swatch.mSwatchID);
		JsonObject->SetStringField(TEXT("mPath"), Swatch.mPath);
		JsonObject->SetStringField(TEXT("mModName"), Swatch.mModName);
		JsonObject->SetBoolField(TEXT("bIsBaseGame"), Swatch.bIsBaseGame);

		JsonArray.Add(MakeShared<FJsonValueObject>(JsonObject));
	}

	// Serialize to string
	FString JsonString;
	TSharedRef<TJsonWriter<>> JsonWriter = TJsonWriterFactory<>::Create(&JsonString);

	if (!FJsonSerializer::Serialize(JsonArray, JsonWriter))
	{
		UE_LOGFMT(CustomizerSubsystem, Error, "SaveSwatchArrayToFile: Failed to serialize JSON");
		return false;
	}

	// Save to file
	bool bSuccess =
		FFileHelper::SaveStringToFile(JsonString, *FilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

	if (bSuccess)
	{
		UE_LOGFMT(CustomizerSubsystem, Log, "SaveSwatchArrayToFile: Successfully saved {0} swatches to {1}",
				  Swatches.Num(), FilePath);
	}
	else
	{
		UE_LOGFMT(CustomizerSubsystem, Error, "SaveSwatchArrayToFile: Failed to save to {0}", FilePath);
	}

	return bSuccess;
}

bool UKBFLCustomizerSubsystem::LoadSwatchArrayFromFile(TArray<FKBFPSwatchSaveInformation>& OutSwatches,
													   const FString& FilePath)
{
	if (FilePath.IsEmpty())
	{
		UE_LOGFMT(CustomizerSubsystem, Warning, "LoadSwatchArrayFromFile: FilePath is empty!");
		return false;
	}

	// Check if file exists
	if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(*FilePath))
	{
		UE_LOGFMT(CustomizerSubsystem, Warning, "LoadSwatchArrayFromFile: File does not exist: {0}", FilePath);
		return false;
	}

	// Load file content
	FString FileContent;
	if (!FFileHelper::LoadFileToString(FileContent, *FilePath))
	{
		UE_LOGFMT(CustomizerSubsystem, Error, "LoadSwatchArrayFromFile: Failed to load file: {0}", FilePath);
		return false;
	}

	// Parse JSON using FJsonSerializer
	TArray<TSharedPtr<FJsonValue>> JsonArray;
	TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(FileContent);

	if (!FJsonSerializer::Deserialize(JsonReader, JsonArray))
	{
		UE_LOGFMT(CustomizerSubsystem, Error, "LoadSwatchArrayFromFile: Failed to parse JSON from: {0}", FilePath);
		return false;
	}

	// Parse each object
	OutSwatches.Empty();

	for (const TSharedPtr<FJsonValue>& JsonValue : JsonArray)
	{
		if (!JsonValue.IsValid() || JsonValue->Type != EJson::Object)
		{
			continue;
		}

		const TSharedPtr<FJsonObject>* JsonObject;
		if (!JsonValue->TryGetObject(JsonObject) || !JsonObject->IsValid())
		{
			continue;
		}

		FKBFPSwatchSaveInformation Swatch;

		// Parse fields
		int32 SwatchID = 0;
		if ((*JsonObject)->TryGetNumberField(TEXT("mSwatchID"), SwatchID))
		{
			Swatch.mSwatchID = SwatchID;
		}

		(*JsonObject)->TryGetStringField(TEXT("mPath"), Swatch.mPath);
		(*JsonObject)->TryGetStringField(TEXT("mModName"), Swatch.mModName);
		(*JsonObject)->TryGetBoolField(TEXT("bIsBaseGame"), Swatch.bIsBaseGame);

		OutSwatches.Add(Swatch);
	}

	UE_LOGFMT(CustomizerSubsystem, Log, "LoadSwatchArrayFromFile: Successfully loaded {0} swatches from {1}",
			  OutSwatches.Num(), FilePath);
	return true;
}
