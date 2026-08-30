#include "Subsystems/KBFLSwatchReplicationSubsystem.h"

#include "BFL/KBFL_Player.h"
#include "BFL/KBFL_Util.h"
#include "Buildables/FGBuildable.h"
#include "EngineUtils.h"
#include "FGBuildableSubsystem.h"
#include "FGGameState.h"
#include "FGLightweightBuildableSubsystem.h"
#include "KBFLLogging.h"
#include "Kismet/GameplayStatics.h"
#include "Logging/StructuredLog.h"
#include "Net/UnrealNetwork.h"
#include "Subsystem/SubsystemActorManager.h"
#include "Subsystems/KBFLAssetDataSubsystem.h"
#include "Subsystems/KBFLCustomizerSubsystem.h"

TSoftClassPtr<UFGFactoryCustomizationDescriptor_Swatch> FKBFLReplicatedColorSlot::GetSoftClass() const
{
	if (IsValid(mCachedClass))
	{
		return TSoftClassPtr<UFGFactoryCustomizationDescriptor_Swatch>(mCachedClass);
	}
	return TSoftClassPtr<UFGFactoryCustomizationDescriptor_Swatch>(FSoftObjectPath(mPath));
}
TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch> FKBFLReplicatedColorSlot::LoadClass()
{
	if (IsValid(mCachedClass))
	{
		return mCachedClass;
	}

	TSoftClassPtr<UFGFactoryCustomizationDescriptor_Swatch> SoftClass = GetSoftClass();
	if (!SoftClass.IsValid())
	{
		UE_LOG(CustomizerSubsystem, Warning, TEXT("Failed to load Swatch Class from path: %s"), *mPath);
		return nullptr;
	}
	mCachedClass = SoftClass.LoadSynchronous();
	return mCachedClass;
}
UKBFLColorRCO* UKBFLColorRCO::Get(UObject* WorldContext)
{
	AFGPlayerController* PlayerController = UKBFL_Player::GetFGController(WorldContext);
	return IsValid(PlayerController) ? PlayerController->GetRemoteCallObjectOfClass<UKBFLColorRCO>() : nullptr;
}

void UKBFLColorRCO::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UKBFLColorRCO, bTest);
}

AKBFLSwatchReplicationSubsystem::AKBFLSwatchReplicationSubsystem()
{
	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnOnServer_Replicate;
	bReplicates = true;
}

TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch>
AKBFLSwatchReplicationSubsystem::GetCachedSwatchClass(const FKBFLReplicatedColorSlot& ColorSlot)
{
	return ColorSlot.GetCachedClass();
}

AKBFLSwatchReplicationSubsystem* AKBFLSwatchReplicationSubsystem::Get(UObject* WorldContext)
{
	return Cast<AKBFLSwatchReplicationSubsystem>(UKBFL_Util::GetSubsystem(WorldContext, StaticClass()));
}

void AKBFLSwatchReplicationSubsystem::BeginPlay()
{
	Super::BeginPlay();

	UE_LOGFMT(CustomizerSubsystem, Log, "KBFLSwatchReplicationSubsystem::BeginPlay - HasAuthority: {0}",
			  HasAuthority() ? TEXT("true") : TEXT("false"));

	if (HasAuthority() && !bSwatchesPatched)
	{
		TryToPatchSwatches();
	}

	if (!HasAuthority() && mReplicatedColorSlots.Num() > 0 && !bColorSlotsApplied)
	{
		OnRep_ColorSlots();
	}
}

void AKBFLSwatchReplicationSubsystem::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AFGBuildableSubsystem* BuildableSubsystem = AFGBuildableSubsystem::Get(this))
	{
		BuildableSubsystem->mBuildableAddedDelegate.RemoveDynamic(this,
																  &AKBFLSwatchReplicationSubsystem::OnBuildableAdded);
	}

	Super::EndPlay(EndPlayReason);
}

void AKBFLSwatchReplicationSubsystem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AKBFLSwatchReplicationSubsystem, mReplicatedColorSlots);
}
int32 AKBFLSwatchReplicationSubsystem::GetSwatchIdSafe(
	TSubclassOf<class UFGFactoryCustomizationDescriptor_Swatch> SwatchDesc)
{
	if (!IsValid(SwatchDesc))
	{
		return INDEX_NONE;
	}
	return SwatchDesc->GetDefaultObject<UFGFactoryCustomizationDescriptor_Swatch>()->ID;
}

void AKBFLSwatchReplicationSubsystem::PostLoadGame_Implementation(int32 saveVersion, int32 gameVersion)
{
	if (HasAuthority())
	{
		UE_LOGFMT(CustomizerSubsystem, Log,
				  "KBFLSwatchReplicationSubsystem::PostLoadGame - Loaded {0} saved color slots",
				  mReplicatedColorSlots.Num());

		bSwatchesPatched = false;
		bColorSlotsApplied = false;
		if (UWorld* World = GetWorld())
		{
			TWeakObjectPtr<AKBFLSwatchReplicationSubsystem> WeakThis(this);
			World->GetTimerManager().SetTimerForNextTick(
				[WeakThis]()
				{
					if (WeakThis.IsValid() && WeakThis->HasAuthority())
					{
						WeakThis->TryToPatchSwatches();
					}
				});
		}
	}
}

void AKBFLSwatchReplicationSubsystem::OnRep_ColorSlots()
{
	UE_LOGFMT(CustomizerSubsystem, Log,
			  "KBFLSwatchReplicationSubsystem::OnRep_ColorSlots - Received {0} color slots on client",
			  mReplicatedColorSlots.Num());

	if (UWorld* World = GetWorld())
	{
		if (World->HasBegunPlay())
		{
			ApplyColorSlotsToWorld();
		}
		else
		{
			{
				TWeakObjectPtr<AKBFLSwatchReplicationSubsystem> WeakThis(this);
				World->GetTimerManager().SetTimerForNextTick(
					[WeakThis]()
					{
						if (WeakThis.IsValid())
						{
							WeakThis->ApplyColorSlotsToWorld();
						}
					});
			}
		}
	}
}

void AKBFLSwatchReplicationSubsystem::TryToPatchSwatches()
{
#if WITH_EDITOR
	return;
#endif

	if (bSwatchesPatched)
	{
		return;
	}

	if (!HasAuthority())
	{
		UE_LOGFMT(CustomizerSubsystem, Warning, "TryToPatchSwatches called on client - ignoring");
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World) || !IsValid(World->GetGameInstance()))
	{
		UE_LOGFMT(CustomizerSubsystem, Warning,
				  "TryToPatchSwatches: World or GameInstance not ready, retrying next tick");
		{
			TWeakObjectPtr<AKBFLSwatchReplicationSubsystem> WeakThis(this);
			World->GetTimerManager().SetTimerForNextTick(
				[WeakThis]()
				{
					if (WeakThis.IsValid())
					{
						WeakThis->TryToPatchSwatches();
					}
				});
		}
		return;
	}

	UKBFLAssetDataSubsystem* AssetDataSubsystem = World->GetGameInstance()->GetSubsystem<UKBFLAssetDataSubsystem>();
	if (!AssetDataSubsystem)
	{
		UE_LOGFMT(CustomizerSubsystem, Warning,
				  "TryToPatchSwatches: AssetDataSubsystem not available, retrying next tick");
		{
			TWeakObjectPtr<AKBFLSwatchReplicationSubsystem> WeakThis(this);
			World->GetTimerManager().SetTimerForNextTick(
				[WeakThis]()
				{
					if (WeakThis.IsValid())
					{
						WeakThis->TryToPatchSwatches();
					}
				});
		}
		return;
	}

	TArray<TSubclassOf<UObject>> OutObjects;
	AssetDataSubsystem->GetObjectsOfChilds({UFGFactoryCustomizationDescriptor_Swatch::StaticClass()}, OutObjects);
	AssetDataSubsystem->GetObjectsOfChilds({UFGFactoryCustomizationDescriptor_Swatch::StaticClass()}, OutObjects, true);

	TSet<TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch>> UniqueSwatches;
	TSet<int32> UniversalReservedIDs;
	for (TSubclassOf<UObject> OutObject : OutObjects)
	{
		TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch> AsSwatch{OutObject};
		if (!IsValid(AsSwatch))
		{
			continue;
		}
		if (AsSwatch->GetPathName().Contains(TEXT("UniversalSwatchSlots")))
		{
			const int32 Id = AsSwatch.GetDefaultObject()->ID;
			if (Id >= 0 && Id <= 254)
			{
				UniversalReservedIDs.Add(Id);
			}
			continue;
		}
		UniqueSwatches.Add(AsSwatch);
	}

	TArray<TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch>> SortedSwatches = UniqueSwatches.Array();
	SortedSwatches.Sort(
		[](const TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch>& A,
		   const TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch>& B)
		{
			FString PathA = A ? A->GetPathName() : FString();
			FString PathB = B ? B->GetPathName() : FString();
			return PathA < PathB;
		});

	UE_LOGFMT(CustomizerSubsystem, Log, "TryToPatchSwatches: Found {0} unique swatches", SortedSwatches.Num());

	AFGGameState* FGGameState = Cast<AFGGameState>(UGameplayStatics::GetGameState(this));
	AFGBuildableSubsystem* Subsystem = AFGBuildableSubsystem::Get(this);

	if (!Subsystem || !FGGameState)
	{
		UE_LOGFMT(CustomizerSubsystem, Warning,
				  "TryToPatchSwatches: BuildableSubsystem or GameState not available, retrying next tick");
		{
			TWeakObjectPtr<AKBFLSwatchReplicationSubsystem> WeakThis(this);
			World->GetTimerManager().SetTimerForNextTick(
				[WeakThis]()
				{
					if (WeakThis.IsValid())
					{
						WeakThis->TryToPatchSwatches();
					}
				});
		}
		return;
	}

	int32 MaxBaseSwatchId = INDEX_NONE;
	for (TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch> AsSwatch : SortedSwatches)
	{
		if (IsBaseGameSwatch(AsSwatch))
		{
			const int32 BaseId = AsSwatch.GetDefaultObject()->ID;
			if (BaseId != 255 && BaseId != INDEX_NONE)
			{
				MaxBaseSwatchId = FMath::Max(MaxBaseSwatchId, BaseId);
			}
		}
	}
	if (MaxBaseSwatchId != INDEX_NONE && Subsystem->mColorSlots_Data.Num() <= MaxBaseSwatchId)
	{
		UE_LOGFMT(CustomizerSubsystem, Warning,
				  "TryToPatchSwatches: base color palette not ready ({0} <= {1}), retrying next tick",
				  Subsystem->mColorSlots_Data.Num(), MaxBaseSwatchId);
		{
			TWeakObjectPtr<AKBFLSwatchReplicationSubsystem> WeakThis(this);
			World->GetTimerManager().SetTimerForNextTick(
				[WeakThis]()
				{
					if (WeakThis.IsValid())
					{
						WeakThis->TryToPatchSwatches();
					}
				});
		}
		return;
	}

	TSet<int32> ReservedIDs;
	ReservedIDs.Append(UniversalReservedIDs);
	for (const FKBFLReplicatedColorSlot& Slot : mReplicatedColorSlots)
	{
		if (Slot.mSlotIndex != 255)
		{
			ReservedIDs.Add(Slot.mSlotIndex);
		}
	}
	for (TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch> AsSwatch : SortedSwatches)
	{
		if (IsBaseGameSwatch(AsSwatch))
		{
			int32 BaseID = AsSwatch.GetDefaultObject()->ID;
			ReservedIDs.Add(BaseID);
		}
	}

	DeduplicateSavedSlots(ReservedIDs);

	for (TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch> AsSwatch : SortedSwatches)
	{
		if (!IsBaseGameSwatch(AsSwatch))
		{
			PatchSwatchFromSave(AsSwatch, ReservedIDs);
		}
	}

	for (TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch> AsSwatch : SortedSwatches)
	{
		if (!IsBaseGameSwatch(AsSwatch))
		{
			PatchSwatchAssignNew(AsSwatch, ReservedIDs);
		}
	}

	TArray<FKBFLReplicatedColorSlot> NewSwatchSlots;

	for (TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch> AsSwatch : SortedSwatches)
	{
		UFGFactoryCustomizationDescriptor_Swatch* SwatchDefaults =
			AsSwatch->GetDefaultObject<UFGFactoryCustomizationDescriptor_Swatch>();
		if (!SwatchDefaults)
		{
			continue;
		}

		int32 ColourIndex = SwatchDefaults->ID;
		bool bIsBase = IsBaseGameSwatch(AsSwatch);

		if (!bIsBase && ColourIndex > 254)
		{
			UE_LOGFMT(CustomizerSubsystem, Warning, "Skipping swatch {0} - no valid color slot (ID {1})",
					  AsSwatch->GetName(), ColourIndex);
			continue;
		}

		SwatchDefaults->mMenuPriority = static_cast<float>(ColourIndex);

		FKBFLReplicatedColorSlot* ExistingSlot = nullptr;
		for (FKBFLReplicatedColorSlot& Slot : mReplicatedColorSlots)
		{
			if (Slot.mPath == AsSwatch->GetPathName())
			{
				ExistingSlot = &Slot;
				break;
			}
		}

		if (ExistingSlot)
		{
			ExistingSlot->mCachedClass = AsSwatch;

		}

		if (!bIsBase)
		{
			bool bIsPaintFinish = SwatchDefaults->IsA<UFGFactoryCustomizationDescriptor_PaintFinish>();
			bool bSlotAlreadyExists = Subsystem->mColorSlots_Data.IsValidIndex(ColourIndex);

			FFactoryCustomizationColorSlot NewColourSlot;
			NewColourSlot.PaintFinish = TSubclassOf<UFGFactoryCustomizationDescriptor_PaintFinish>(AsSwatch);

			if (bIsPaintFinish)
			{

				UFGFactoryCustomizationDescriptor_PaintFinish* PaintFinish =
					Cast<UFGFactoryCustomizationDescriptor_PaintFinish>(SwatchDefaults);
				NewColourSlot = FFactoryCustomizationColorSlot(PaintFinish->mForcedColor, PaintFinish->mForcedColor);
				NewColourSlot.PaintFinish = TSubclassOf<UFGFactoryCustomizationDescriptor_PaintFinish>(AsSwatch);
			}
			else if (ExistingSlot || bSlotAlreadyExists)
			{

				if (Subsystem->mColorSlots_Data.IsValidIndex(ColourIndex))
				{
					NewColourSlot = Subsystem->mColorSlots_Data[ColourIndex];
				}
			}
			else if (UKBFLactoryCustomizationDescriptor_Swatch* SwatchInterface =
						 Cast<UKBFLactoryCustomizationDescriptor_Swatch>(SwatchDefaults))
			{

				NewColourSlot = SwatchInterface->mDefaultColorSlot;
			}
			else
			{

				const uint32 Seed = GetTypeHash(AsSwatch->GetPathName());
				FRandomStream Rng(static_cast<int32>(Seed));
				auto RandChannel = [&]() -> float { return Rng.FRandRange(0.1f, 0.9f); };

				FLinearColor PrimaryColor(RandChannel(), RandChannel(), RandChannel(), 1.0f);
				FLinearColor SecondaryColor(FMath::Fmod(PrimaryColor.R + 0.5f, 1.0f),
											FMath::Fmod(PrimaryColor.G + 0.5f, 1.0f),
											FMath::Fmod(PrimaryColor.B + 0.5f, 1.0f), 1.0f);

				NewColourSlot.PrimaryColor = PrimaryColor;
				NewColourSlot.SecondaryColor = SecondaryColor;

				UE_LOGFMT(CustomizerSubsystem, Log, "Generated stable color for new swatch slot {0} (Index: {1})",
						  AsSwatch->GetName(), ColourIndex);
			}

			if (!bSlotAlreadyExists)
			{
				for (int32 i = Subsystem->mColorSlots_Data.Num(); i <= ColourIndex; ++i)
				{
					FFactoryCustomizationColorSlot SlotToAdd =
						(i == ColourIndex) ? NewColourSlot : FFactoryCustomizationColorSlot();

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
					Subsystem->mColorSlotsAreDirty = true;

					UE_LOGFMT(CustomizerSubsystem, Log, "New Colour slot added: {0} {1}", i, AsSwatch->GetName());
				}
			}

			if (!FGGameState->mBuildingColorSlots_Data.IsValidIndex(ColourIndex))
			{
				FGGameState->mBuildingColorSlots_Data.SetNum(ColourIndex + 1, EAllowShrinking::No);
				FGGameState->mBuildingColorSlots_Data[ColourIndex] = Subsystem->mColorSlots_Data[ColourIndex];
				FGGameState->SetupColorSlots_Data(Subsystem->mColorSlots_Data);
			}

			if (bIsPaintFinish)
			{
				UFGFactoryCustomizationDescriptor_PaintFinish* PaintFinish =
					Cast<UFGFactoryCustomizationDescriptor_PaintFinish>(AsSwatch->GetDefaultObject());
				FFactoryCustomizationColorSlot NewColourSlotFinished =
					FFactoryCustomizationColorSlot(PaintFinish->mForcedColor, PaintFinish->mForcedColor);
				NewColourSlotFinished.PaintFinish =
					TSubclassOf<UFGFactoryCustomizationDescriptor_PaintFinish>(AsSwatch);

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

				NewColourSlot = NewColourSlotFinished;
			}
		}

		if (!ExistingSlot)
		{

			FKBFLReplicatedColorSlot NewSlot;
			NewSlot.mSlotIndex = static_cast<uint8>(ColourIndex);
			NewSlot.mPath = AsSwatch->GetPathName();
			NewSlot.mModName = GetModNameFromPath(AsSwatch->GetPathName());
			NewSlot.bIsBaseGame = bIsBase;
			NewSlot.mCachedClass = AsSwatch;

			NewSwatchSlots.Add(NewSlot);
		}
	}

	NewSwatchSlots.Sort([](const FKBFLReplicatedColorSlot& A, const FKBFLReplicatedColorSlot& B)
						{ return A.mSlotIndex < B.mSlotIndex; });

	for (FKBFLReplicatedColorSlot& NewSlot : NewSwatchSlots)
	{
		if (!NewSlot.bIsBaseGame && IsValid(NewSlot.mCachedClass))
		{
			NewSlot.mCachedClass->GetDefaultObject<UFGFactoryCustomizationDescriptor_Swatch>()->mMenuPriority =
				static_cast<float>(NewSlot.mSlotIndex);
		}

		mReplicatedColorSlots.Add(NewSlot);

		UE_LOGFMT(CustomizerSubsystem, Log, "Added replicated slot: {0} (Index: {1}, BaseGame: {2})",
				  NewSlot.mCachedClass ? NewSlot.mCachedClass->GetName() : NewSlot.mPath, NewSlot.mSlotIndex,
				  NewSlot.bIsBaseGame ? TEXT("true") : TEXT("false"));
	}

	OnRep_ColorSlots();

	bSwatchesPatched = true;

	UE_LOGFMT(CustomizerSubsystem, Log,
			  "KBFLSwatchReplicationSubsystem: TryToPatchSwatches complete - {0} color slots stored for replication",
			  mReplicatedColorSlots.Num());
}

void AKBFLSwatchReplicationSubsystem::DeduplicateSavedSlots(TSet<int32>& ReservedIDs)
{
	TSet<int32> ClaimedIndices;
	for (FKBFLReplicatedColorSlot& Slot : mReplicatedColorSlots)
	{
		if (Slot.mSlotIndex == 255 || Slot.bIsBaseGame)
		{
			continue;
		}

		if (!ClaimedIndices.Contains(Slot.mSlotIndex))
		{
			ClaimedIndices.Add(Slot.mSlotIndex);
			continue;
		}

		constexpr int32 SafeMin = 28;
		constexpr int32 SafeMax = 254;
		int32 NewIndex = SafeMin;
		while (NewIndex <= SafeMax && (ReservedIDs.Contains(NewIndex) || ClaimedIndices.Contains(NewIndex)))
		{
			++NewIndex;
		}
		if (NewIndex > SafeMax)
		{
			UE_LOGFMT(CustomizerSubsystem, Warning,
					  "DeduplicateSavedSlots: no free slot to resolve duplicate index {0} for {1} - leaving as-is",
					  Slot.mSlotIndex, Slot.mPath);
			continue;
		}

		UE_LOGFMT(CustomizerSubsystem, Warning,
				  "DeduplicateSavedSlots: {0} was saved with duplicate slot index {1} - reassigning to {2}", Slot.mPath,
				  Slot.mSlotIndex, NewIndex);

		Slot.mSlotIndex = static_cast<uint8>(NewIndex);
		ReservedIDs.Add(NewIndex);
		ClaimedIndices.Add(NewIndex);
	}
}

void AKBFLSwatchReplicationSubsystem::PatchSwatchFromSave(TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch> Swatch,
														  TSet<int32>& ReservedIDs)
{
	UFGFactoryCustomizationDescriptor_Swatch* SwatchDefault = Swatch.GetDefaultObject();

	if (IsBaseGameSwatch(Swatch))
	{
		return;
	}

	for (FKBFLReplicatedColorSlot& Slot : mReplicatedColorSlots)
	{
		TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch> SlotClass = Slot.LoadClass();
		if (SlotClass == Swatch)
		{
			SwatchDefault->ID = Slot.mSlotIndex;
			Slot.mCachedClass = Swatch;
			ReservedIDs.Add(Slot.mSlotIndex);
			UE_LOGFMT(CustomizerSubsystem, Log, "Swatch ID found in save for {0} > {1}", Swatch->GetName(),
					  SwatchDefault->ID);
			return;
		}
	}

	if (SwatchDefault->ID != 0 && IsSwatchUsed(Swatch) && !IsColorIdDuplicate(Swatch))
	{
		ReservedIDs.Add(SwatchDefault->ID);
		UE_LOGFMT(CustomizerSubsystem, Log, "Swatch {0} is used in world but not in save - keeping current ID {1}",
				  Swatch->GetName(), SwatchDefault->ID);
	}

}

void AKBFLSwatchReplicationSubsystem::PatchSwatchAssignNew(TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch> Swatch,
														   TSet<int32>& ReservedIDs)
{
	UFGFactoryCustomizationDescriptor_Swatch* SwatchDefault = Swatch.GetDefaultObject();

	if (IsBaseGameSwatch(Swatch))
	{
		return;
	}

	if (ReservedIDs.Contains(SwatchDefault->ID))
	{

		bool bOwnsID = false;
		for (const FKBFLReplicatedColorSlot& Slot : mReplicatedColorSlots)
		{
			TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch> SlotClass = Slot.GetCachedClass();
			if (SlotClass == Swatch && Slot.mSlotIndex == SwatchDefault->ID)
			{
				bOwnsID = true;
				break;
			}
		}

		if (!bOwnsID && SwatchDefault->ID != 0 && IsSwatchUsed(Swatch))
		{
			bOwnsID = true;
		}
		if (bOwnsID)
		{
			return;
		}
	}

	constexpr int32 SafeMin = 28;
	constexpr int32 SafeMax = 254;
	constexpr int32 SafeCount = SafeMax - SafeMin + 1;

	int32 NewID = SafeMin;
	while (NewID <= SafeMax && ReservedIDs.Contains(NewID))
	{
		NewID++;
	}

	if (NewID > SafeMax)
	{

		NewID = SafeMin + static_cast<int32>(GetTypeHash(Swatch->GetPathName()) % SafeCount);
		UE_LOGFMT(CustomizerSubsystem, Warning, "Color slots full - Swatch {0} reuses slot {1}", Swatch->GetName(),
				  NewID);
	}

	SwatchDefault->ID = NewID;
	ReservedIDs.Add(NewID);
	UE_LOGFMT(CustomizerSubsystem, Log, "Assigned new Swatch ID for {0} > {1}", Swatch->GetName(), NewID);
}

void AKBFLSwatchReplicationSubsystem::ApplyColorSlotsToWorld()
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		UE_LOGFMT(CustomizerSubsystem, Warning,
				  "KBFLSwatchReplicationSubsystem::ApplyColorSlotsToWorld - World is invalid");
		return;
	}

	AFGBuildableSubsystem* BuildableSubsystem = AFGBuildableSubsystem::Get(this);
	AFGLightweightBuildableSubsystem* BuildableLightSubsystem = AFGLightweightBuildableSubsystem::Get(this);
	AFGGameState* GameState = Cast<AFGGameState>(UGameplayStatics::GetGameState(this));

	if (!BuildableSubsystem || !GameState)
	{
		UE_LOGFMT(CustomizerSubsystem, Warning,
				  "KBFLSwatchReplicationSubsystem::ApplyColorSlotsToWorld - BuildableSubsystem or GameState not "
				  "available, retrying next tick");

		{
			TWeakObjectPtr<AKBFLSwatchReplicationSubsystem> WeakThis(this);
			World->GetTimerManager().SetTimerForNextTick(
				[WeakThis]()
				{
					if (WeakThis.IsValid())
					{
						WeakThis->ApplyColorSlotsToWorld();
					}
				});
		}
		return;
	}

	for (FKBFLReplicatedColorSlot& Slot : mReplicatedColorSlots)
	{
		const int32 SlotIndex = Slot.mSlotIndex;
		if (SlotIndex == 255)
		{
			continue;
		}

		if (Slot.bIsBaseGame)
		{
			continue;
		}

		if (TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch> SwatchClass = Slot.LoadClass())
		{
			if (UFGFactoryCustomizationDescriptor_Swatch* SwatchCDO = SwatchClass.GetDefaultObject())
			{
				SwatchCDO->ID = SlotIndex;
				SwatchCDO->mMenuPriority = static_cast<float>(SlotIndex);
			}
		}

		FFactoryCustomizationColorSlot ColourToUse;
		if (BuildableSubsystem->mColorSlots_Data.IsValidIndex(SlotIndex))
		{
			ColourToUse = BuildableSubsystem->mColorSlots_Data[SlotIndex];
		}
		else if (GameState->mBuildingColorSlots_Data.IsValidIndex(SlotIndex))
		{
			ColourToUse = GameState->mBuildingColorSlots_Data[SlotIndex];
		}
		else
		{

			continue;
		}

		if (!BuildableSubsystem->mColorSlots_Data.IsValidIndex(SlotIndex))
		{

			const int32 OldNum = BuildableSubsystem->mColorSlots_Data.Num();
			BuildableSubsystem->mColorSlots_Data.SetNum(SlotIndex + 1, EAllowShrinking::No);
			for (int32 i = OldNum; i < SlotIndex; ++i)
			{
				if (GameState->mBuildingColorSlots_Data.IsValidIndex(i))
				{
					BuildableSubsystem->mColorSlots_Data[i] = GameState->mBuildingColorSlots_Data[i];
				}
			}
		}
		BuildableSubsystem->mColorSlots_Data[SlotIndex] = ColourToUse;
		BuildableSubsystem->SetColorSlot_Data(SlotIndex, ColourToUse);
		BuildableSubsystem->mColorSlotsAreDirty = true;
		BuildableSubsystem->mDirtyColorSlots.AddUnique(SlotIndex);
		BuildableSubsystem->mOnColorIndexChanged.Broadcast(SlotIndex);

		if (HasAuthority())
		{
			if (!GameState->mBuildingColorSlots_Data.IsValidIndex(SlotIndex))
			{
				GameState->mBuildingColorSlots_Data.SetNum(SlotIndex + 1, EAllowShrinking::No);
			}
			GameState->mBuildingColorSlots_Data[SlotIndex] = ColourToUse;
		}
	}

	if (HasAuthority())
	{
		GameState->SetupColorSlots_Data(BuildableSubsystem->mColorSlots_Data);
	}
	BuildableSubsystem->mColorSlotsAreDirty = true;

	bColorSlotsApplied = true;

	BuildableSubsystem->mBuildableAddedDelegate.AddUniqueDynamic(this,
																 &AKBFLSwatchReplicationSubsystem::OnBuildableAdded);

	RefreshPaintFinishCustomizations();
	SchedulePaintFinishCustomizationRefreshes();

	UE_LOGFMT(CustomizerSubsystem, Log,
			  "KBFLSwatchReplicationSubsystem: Applied {0} color slots to world (Authority: {1})",
			  mReplicatedColorSlots.Num(), HasAuthority() ? TEXT("true") : TEXT("false"));

	if (IsValid(BuildableLightSubsystem))
	{
		for (int32 I = 0; I < GameState->mBuildingColorSlots_Data.Num(); ++I)
		{
			BuildableLightSubsystem->SetColorSlotIndexDataDirty(I);
		}
	}
}

bool AKBFLSwatchReplicationSubsystem::ShouldRefreshPaintFinishCustomization(
	const FFactoryCustomizationData& CustomizationData)
{
	return IsValid(CustomizationData.SwatchDesc) &&
		CustomizationData.SwatchDesc->IsChildOf(UFGFactoryCustomizationDescriptor_PaintFinish::StaticClass()) &&
		!IsBaseGameSwatch(CustomizationData.SwatchDesc);
}

void AKBFLSwatchReplicationSubsystem::RefreshPaintFinishCustomizations()
{
	AFGBuildableSubsystem* BuildableSubsystem = AFGBuildableSubsystem::Get(this);
	AFGLightweightBuildableSubsystem* LightweightSubsystem = AFGLightweightBuildableSubsystem::Get(this);
	AFGGameState* GameState = Cast<AFGGameState>(UGameplayStatics::GetGameState(this));
	if (!IsValid(BuildableSubsystem) || !IsValid(GameState))
	{
		return;
	}

	int32 RefreshedBuildables = 0;
	for (AFGBuildable* Buildable : BuildableSubsystem->GetAllBuildablesRef())
	{
		if (!IsValid(Buildable))
		{
			continue;
		}

		RefreshedBuildables += RefreshPaintFinishCustomization(Buildable) ? 1 : 0;
	}

	int32 RefreshedLightweightBuildables = 0;
	if (IsValid(LightweightSubsystem))
	{
		struct FPendingLightweightCustomization
		{
			TSubclassOf<AFGBuildable> BuildableClass;
			int32 Index = INDEX_NONE;
			FFactoryCustomizationData CustomizationData;
		};

		TArray<FPendingLightweightCustomization> PendingCustomizations;
		for (const TPair<TSubclassOf<AFGBuildable>, TArray<FRuntimeBuildableInstanceData>>& BuildableInstances :
			 LightweightSubsystem->GetAllLightweightBuildableInstances())
		{
			for (int32 Index = 0; Index < BuildableInstances.Value.Num(); ++Index)
			{
				const FRuntimeBuildableInstanceData& InstanceData = BuildableInstances.Value[Index];
				if (!InstanceData.IsValidOnLoad() ||
					!ShouldRefreshPaintFinishCustomization(InstanceData.CustomizationData))
				{
					continue;
				}

				FPendingLightweightCustomization& Pending = PendingCustomizations.AddDefaulted_GetRef();
				Pending.BuildableClass = BuildableInstances.Key;
				Pending.Index = Index;
				Pending.CustomizationData = InstanceData.CustomizationData;
				Pending.CustomizationData.Data.Reset();
				Pending.CustomizationData.ColorSlot = INDEX_NONE;
				Pending.CustomizationData.Initialize(GameState);
			}
		}

		for (const FPendingLightweightCustomization& Pending : PendingCustomizations)
		{
			LightweightSubsystem->SetCustomizationDataOnInstance(Pending.BuildableClass, Pending.CustomizationData,
																 Pending.Index);
			++RefreshedLightweightBuildables;
		}
	}

	UE_LOGFMT(CustomizerSubsystem, Log,
			  "Refreshed mod paint-finish customization caches for {0} buildables and {1} lightweight buildables",
			  RefreshedBuildables, RefreshedLightweightBuildables);
}

void AKBFLSwatchReplicationSubsystem::SchedulePaintFinishCustomizationRefreshes()
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	constexpr float RefreshDelays[] = {0.25f, 1.0f, 3.0f, 8.0f};
	TWeakObjectPtr<AKBFLSwatchReplicationSubsystem> WeakThis(this);
	for (const float Delay : RefreshDelays)
	{
		FTimerHandle TimerHandle;
		FTimerDelegate RefreshDelegate;
		RefreshDelegate.BindLambda(
			[WeakThis]()
			{
				if (WeakThis.IsValid() && WeakThis->bColorSlotsApplied)
				{
					WeakThis->RefreshPaintFinishCustomizations();
				}
			});
		World->GetTimerManager().SetTimer(TimerHandle, RefreshDelegate, Delay, false);
	}
}

bool AKBFLSwatchReplicationSubsystem::RefreshPaintFinishCustomization(AFGBuildable* Buildable)
{
	if (!IsValid(Buildable))
	{
		return false;
	}

	FFactoryCustomizationData& CustomizationData = Buildable->GetCustomizationData_Native();
	if (!ShouldRefreshPaintFinishCustomization(CustomizationData))
	{
		return false;
	}

	AFGGameState* GameState = Cast<AFGGameState>(UGameplayStatics::GetGameState(this));
	if (!IsValid(GameState))
	{
		return false;
	}

	CustomizationData.Data.Reset();
	CustomizationData.ColorSlot = INDEX_NONE;
	CustomizationData.Initialize(GameState);
	Buildable->ApplyCustomizationData_Native(CustomizationData);
	return true;
}

void AKBFLSwatchReplicationSubsystem::OnBuildableAdded(AFGBuildable* Buildable)
{
	if (!bColorSlotsApplied || !IsValid(Buildable))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	TWeakObjectPtr<AKBFLSwatchReplicationSubsystem> WeakThis(this);
	TWeakObjectPtr<AFGBuildable> WeakBuildable(Buildable);
	FTimerHandle TimerHandle;
	FTimerDelegate RefreshDelegate;
	RefreshDelegate.BindLambda(
		[WeakThis, WeakBuildable]()
		{
			if (WeakThis.IsValid() && WeakBuildable.IsValid())
			{
				WeakThis->RefreshPaintFinishCustomization(WeakBuildable.Get());
			}
		});
	World->GetTimerManager().SetTimer(TimerHandle, RefreshDelegate, 0.25f, false);
}

bool AKBFLSwatchReplicationSubsystem::GetSlotInformation(TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch> InClass,
														 FKBFLReplicatedColorSlot& OutColorSlot) const
{
	if (!IsValid(InClass))
	{
		return false;
	}

	for (const FKBFLReplicatedColorSlot& Slot : mReplicatedColorSlots)
	{
		if (Slot.GetCachedClass() == InClass || Slot.mPath == InClass->GetPathName())
		{
			OutColorSlot = Slot;
			return true;
		}
	}
	return false;
}

bool AKBFLSwatchReplicationSubsystem::IsSwatchUsed(TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch> InClass) const
{
	if (!IsValid(InClass))
	{
		return false;
	}

	int32 SwatchID = InClass.GetDefaultObject()->ID;
	AFGBuildableSubsystem* Subsystem = AFGBuildableSubsystem::Get(GetWorld());
	if (!Subsystem)
	{
		return false;
	}

	for (AFGBuildable* const& Ref : Subsystem->GetAllBuildablesRef())
	{
		if (Ref && Ref->mColorSlot == SwatchID)
		{
			return true;
		}
	}

	AFGLightweightBuildableSubsystem* Sub = AFGLightweightBuildableSubsystem::Get(GetWorld());
	if (Sub)
	{
		for (const TPair<TSubclassOf<AFGBuildable>, TArray<FRuntimeBuildableInstanceData>>& BuildableInstances :
			 Sub->GetAllLightweightBuildableInstances())
		{
			for (const FRuntimeBuildableInstanceData& InstanceData : BuildableInstances.Value)
			{
				if (InstanceData.IsValid())
				{
					int32 ColorSlot = InstanceData.CustomizationData.ColorSlot;
					if (SwatchID == ColorSlot)
					{
						return true;
					}
				}
			}
		}
	}

	return false;
}

bool AKBFLSwatchReplicationSubsystem::IsColorIdDuplicate(
	TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch> InClass) const
{
	int32 ColorId = InClass.GetDefaultObject()->ID;
	for (const FKBFLReplicatedColorSlot& Slot : mReplicatedColorSlots)
	{
		if (Slot.mSlotIndex == ColorId && Slot.GetCachedClass() != InClass)
		{
			return true;
		}
	}

	return false;
}
bool AKBFLSwatchReplicationSubsystem::IsSwatchEditable(
	TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch> InClass) const
{
	if (!IsValid(InClass))
	{
		return false;
	}

	if (InClass->IsChildOf(UFGFactoryCustomizationDescriptor_PaintFinish::StaticClass()))
	{
		return false;
	}

	if (IsBaseGameSwatch(InClass))
	{
		return mEditableSwatchNames.Contains(InClass);
	}

	return true;
}

bool AKBFLSwatchReplicationSubsystem::IsBaseGameSwatch(
	const TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch>& SwatchInfo)
{
	return SwatchInfo->GetPathName().StartsWith("/Game/");
}

FString AKBFLSwatchReplicationSubsystem::GetModNameFromPath(const FString& ModPath)
{
	FString Result;
	return ModPath.Split(TEXT("/"), nullptr, &Result) ? Result : FString();
}
