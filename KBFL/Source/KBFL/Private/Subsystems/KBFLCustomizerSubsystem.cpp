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
