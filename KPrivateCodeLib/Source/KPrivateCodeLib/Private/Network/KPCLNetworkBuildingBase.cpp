// Copyright Coffee Stain Studios. All Rights Reserved.

#include "Network/KPCLNetworkBuildingBase.h"

#include "FGActorRepresentationManager.h"
#include "FGResourceSinkSubsystem.h"
#include "Interfaces/KPCLNetworkDataInterface.h"
#include "KPrivateCodeLibModule.h"
#include "Net/UnrealNetwork.h"
#include "Network/Buildings/KPCLNetworkCore.h"
#include "Network/Buildings/KPCLNetworkTower.h"
#include "Network/KPCLNetwork.h"
#include "Network/KPCLNetworkAsyncHelpers.h"
#include "Network/KPCLNetworkConnectionComponent.h"
#include "Network/KPCLNetworkInfoComponent.h"
#include "Subsystem/KPCLFaxitSubsystem.h"
#include "Subsystem/KPCLUnlockSubsystem.h"
#include "Wwise/WwiseRetriggerableAsyncTask.h"

bool AKPCLNetworkBuildingBase::HasCore_Implementation() const { return HasFaxitCore(); }

AKPCLNetworkCore* AKPCLNetworkBuildingBase::GetCore_Implementation() { return GetFaxitCore(); }

UKPCLNetwork* AKPCLNetworkBuildingBase::GetNetwork_Implementation() const { return GetFaxitCableNetwork(); }

FNetworkUIData AKPCLNetworkBuildingBase::GetUIDData_Implementation() const
{
	FNetworkUIData Data = mNetworkUIData.Copy();
	Data.mIsWireless = !bHasCableBoost;
	return Data;
}

FKPCLFaxitNetwork AKPCLNetworkBuildingBase::GetNetworkData_Implementation() const
{
	FKPCLFaxitNetwork Outer;
	if (!IsValid(mFaxitSubsystem))
	{
		return Outer;
	}

	mFaxitSubsystem->GetNetwork(this, Outer);
	return Outer;
}

TArray<EKPCLNetworkError> AKPCLNetworkBuildingBase::GetNetworkErrors_Implementation() const
{
	return GetNetworkErrorCodes();
}

bool AKPCLNetworkBuildingBase::HasCoreInNetwork_Implementation() const
{
	if (UKPCLNetwork* Network = Execute_GetNetwork(this))
	{
		return IsValid(Network->GetCore()) && Network->CoreStateIsOk();
	}

	return false;
}

void AKPCLNetworkBuildingBase::SetHasCableBoost(bool NewValue)
{
	bHasCableBoost = NewValue;
	mPropertyReplicator.MarkPropertyDirty(FName("bHasCableBoost"));
}

void AKPCLNetworkBuildingBase::SetNetworkIdDirect(const FString& NewId)
{
	mNetworkId = NewId;
	mPropertyReplicator.MarkPropertyDirty(FName("mNetworkId"));
}

void AKPCLNetworkBuildingBase::SetConnectedNetworkIdDirect(const FString& NewId)
{
	mConnectedNetworkId = NewId;
	mPropertyReplicator.MarkPropertyDirty(FName("mConnectedNetworkId"));
}

void AKPCLNetworkBuildingBase::SetDistanceToNetworkCore(float NewDist)
{
	mDistanceToNetworkCore = NewDist;
	mPropertyReplicator.MarkPropertyDirty(FName("mDistanceToNetworkCore"));
}

void AKPCLNetworkBuildingBase::SetNetworkCore(AKPCLNetworkCore* Core, FKPCLFaxitNetwork* Network)
{
	if (IsValid(Core))
	{
		const FString CoreNetworkId = Core->GetNetworkId();
		SetNetworkIdDirect(CoreNetworkId);
		SetConnectedNetworkIdDirect(CoreNetworkId);
		if (Network)
		{
			SetDistanceToNetworkCore(
				Network->GetNearstDistanceToAccessPoint(GetActorLocation(), IsValid(Cast<AKPCLNetworkTower>(this))));
		}
	}
	else
	{
		SetNetworkIdDirect(FString());
		SetConnectedNetworkIdDirect(FString());
		SetDistanceToNetworkCore(0.f);
	}

	UpdateRepresentation();
}

bool AKPCLNetworkBuildingBase::Factory_HasFaxitCableConnection() const
{
	UKPCLNetwork* Network = GetFaxitCableNetwork();
	return IsValid(Network) && Network->NetworkHasCore();
}

void AKPCLNetworkBuildingBase::HandlePower(float dt)
{
	mPowerOptions.bHasPower = HasPower();
	mPowerOptions.StructureTick(dt, IsProducing());

	// Fixed: GetPowerInfoExplicit() / GetNetworkInfoComponent() can return null on
	// buildings whose components have not been created by a subclass BP.
	UFGPowerInfoComponent* PowerInfo = GetPowerInfoExplicit();
	if (IsValid(PowerInfo))
	{
		PowerInfo->SetTargetConsumption(IsProducing() ? GetRealPowerConsume() : 0.1f);
		PowerInfo->SetMaximumTargetConsumption(GetRealPowerConsume());
	}

	UKPCLNetworkInfoComponent* NetInfo = GetNetworkInfoComponent();
	if (IsValid(NetInfo))
	{
		NetInfo->SetTargetConsumption(IsProducing() ? mPowerOptions.mNormalPowerConsume + GetPowerConsumeOnDistance()
													: 0.1f);
		NetInfo->SetMaximumTargetConsumption(GetRealPowerConsume());
		NetInfo->SetBaseProduction(1000000000.f);
	}
}

void AKPCLNetworkBuildingBase::UpdateDistanceToCore()
{
	if (bHasCableBoost)
	{
		SetDistanceToNetworkCore(0.f);
		return;
	}

	AKPCLNetworkCore* Core = GetFaxitCore();
	if (IsValid(Core))
	{
		SetDistanceToNetworkCore(Core->GetDistanceFrom(this));
	}
}

bool AKPCLNetworkBuildingBase::HasCableBoost() const { return bHasCableBoost; }

bool AKPCLNetworkBuildingBase::HasFaxitCore() const { return IsValid(GetFaxitCoreConst()); }

const AKPCLNetworkCore* AKPCLNetworkBuildingBase::GetFaxitCoreConst() const
{
	FKPCLFaxitNetwork Network = GetFaxitNetwork();
	return Network.mCore;
}

AKPCLNetworkCore* AKPCLNetworkBuildingBase::GetFaxitCore()
{
	FKPCLFaxitNetwork Network = GetFaxitNetwork();
	return Network.mCore;
}

UKPCLNetwork* AKPCLNetworkBuildingBase::GetFaxitCableNetwork() const
{
	if (GetNetworkInfoComponent() && GetNetworkInfoComponent()->IsConnected())
	{
		return Cast<UKPCLNetwork>(GetNetworkInfoComponent()->GetPowerCircuit());
	}
	return nullptr;
}

FKPCLFaxitNetwork AKPCLNetworkBuildingBase::GetFaxitNetwork() const
{
	if (!mFaxitSubsystem)
	{
		return FKPCLFaxitNetwork();
	}
	bool Success;
	return mFaxitSubsystem->GetByNetworkId(GetNetworkId(), Success);
}

FString AKPCLNetworkBuildingBase::GetNetworkId() const
{
	UKPCLNetwork* Network = GetFaxitCableNetwork();
	if (IsValid(Network) && Network->NetworkHasCore())
	{
		return Network->GetNetworkId();
	}
	return mNetworkId;
}

bool AKPCLNetworkBuildingBase::GetCableNetworkHasToManyCores() const
{
	UKPCLNetwork* Network = GetFaxitCableNetwork();
	if (IsValid(Network))
	{
		return Network->NetworkHasCoreToMuchCores();
	}
	return false;
}

TArray<EKPCLNetworkError> AKPCLNetworkBuildingBase::GetNetworkErrorCodes() const
{
	TArray<EKPCLNetworkError> ErrorCodes;
	if (!IsValid(mFaxitSubsystem))
	{
		return ErrorCodes;
	}

	bool Success;
	FKPCLFaxitNetwork Network = mFaxitSubsystem->GetByNetworkId(GetNetworkId(), Success);

	if (!Success)
	{
		ErrorCodes.Add(EKPCLNetworkError::NoNetwork);
	}

	if (mFaxitSubsystem->HasToManyNetworks())
	{
		ErrorCodes.Add(EKPCLNetworkError::GlobalNetworkToManyAddresses);
	}

	if (GetCableNetworkHasToManyCores())
	{
		ErrorCodes.Add(EKPCLNetworkError::NetworkToManyAddresses);
	}

	if (IsValid(Network.mCore))
	{
		if (!Network.mCore->HasDrives())
		{
			ErrorCodes.Add(EKPCLNetworkError::NoDrivesAvailable);
		}

		if (Network.mCore->IsOverloaded())
		{
			ErrorCodes.Add(EKPCLNetworkError::NetworkOverloaded);
		}

		if (!Network.mCore->GetHasPower())
		{
			ErrorCodes.Add(EKPCLNetworkError::NoPower);
		}
	}

	return ErrorCodes;
}

void AKPCLNetworkBuildingBase::TryToConnectToNearstCore()
{
	if (!HasAuthority())
	{
		return;
	}

	// Fixed: was dereferencing mFaxitSubsystem without null check.
	if (!IsValid(mFaxitSubsystem))
	{
		UE_LOG(LogFaxit, Warning,
			   TEXT("AKPCLNetworkBuildingBase::TryToConnectToNearstCore - mFaxitSubsystem is null for %s"), *GetName());
		return;
	}

	FKPCLFaxitNetwork* NetworkRef = mFaxitSubsystem->GetNetworkRef(GetNetworkId());
	if (!NetworkRef)
	{
		FKPCLFaxitNetwork Network;
		mFaxitSubsystem->GetNearstNetwork(this, Network);
		if (Network.mIsValid)
		{
			mFaxitSubsystem->AddBuildingToCore(this, Network.mCore);
		}
		else
		{
			UE_LOG(LogFaxit, Warning,
				   TEXT("AKPCLNetworkBuildingBase::TryToConnectToNearstCore - Failed to find nearest network core for "
						"building %s, Is there no Network or are all on limit?"),
				   *GetName());
		}
	}
}

float AKPCLNetworkBuildingBase::GetRealPowerConsume() const { return GetPowerConsume(); }

void AKPCLNetworkBuildingBase::OnNetworkDestoryed_Internal() {}

void AKPCLNetworkBuildingBase::OnNetworkAdded_Internal(AKPCLNetworkCore* Core) {}

float FKPCLNetworkDistanceModifier::GetValue(float Distance) const
{
	if (Distance - mSafeDistance <= 0.f)
	{
		return mDefaultValue;
	}

	if (bNegative)
	{
		return FMath::Max(mDefaultValue - ((Distance - mSafeDistance) / mEveryMeter) * mValue, mMaxValue);
	}

	int32 Counter = FMath::CeilToInt32((Distance - mSafeDistance) / mEveryMeter);
	return FMath::Min(mDefaultValue + (Counter * mValue), mMaxValue);
}

float FKPCLNetworkDistanceModifier::GetValue(AActor* ActorA, AActor* ActorB) const
{
	if (!IsValid(ActorA) || !IsValid(ActorB))
	{
		return mDefaultValue;
	}

	FVector LocationA = ActorA->GetActorLocation();
	FVector LocationB = ActorB->GetActorLocation();
	float Distance = FVector::Distance(LocationA, LocationB);
	return GetValue(Distance);
}

AKPCLNetworkBuildingBase::AKPCLNetworkBuildingBase()
{
	PrimaryActorTick.bCanEverTick = false;
	mPowerInfo = CreateDefaultSubobject<UKPCLNetworkInfoComponent>(TEXT("NetworkConnection"));
	mInteractionRegisterPlayerWithCircuit = true;
	mMinimumStoppedTime = 1.f;
}

bool AKPCLNetworkBuildingBase::Overclocking_ShouldUse_Implementation() { return false; }

UFGFactoryClipboardSettings* AKPCLNetworkBuildingBase::CopySettings_Implementation()
{
	UKPCLFaxitBasicClipboardSettings* Settings = NewObject<UKPCLFaxitBasicClipboardSettings>();
	Settings->mNetworkId = mNetworkId;
	Settings->bIsPaused = IsProductionPaused();
	Settings->mConnectedNetworkId = mConnectedNetworkId;
	return Settings;
}

bool AKPCLNetworkBuildingBase::PasteSettings_Implementation(UFGFactoryClipboardSettings* factoryClipboard,
															class AFGPlayerController* player)
{
	if (UKPCLFaxitBasicClipboardSettings* Settings = Cast<UKPCLFaxitBasicClipboardSettings>(factoryClipboard))
	{
		// Fixed: was dereferencing mFaxitSubsystem without null check.
		if (IsValid(mFaxitSubsystem))
		{
			bool Success;
			FKPCLFaxitNetwork Network = mFaxitSubsystem->GetByNetworkId(Settings->mNetworkId, Success);
			if (Success)
			{
				mFaxitSubsystem->AddBuildingToCore(this, Network.mCore);
			}
		}

		SetIsProductionPaused(Settings->bIsPaused);
		return true;
	}

	return false;
}

void AKPCLNetworkBuildingBase::UI_ApplyRelevantItems_Implementation(TArray<TSubclassOf<UFGItemDescriptor>>& OutSlots) {}

bool AKPCLNetworkBuildingBase::CanUseFactoryClipboard_Implementation() { return true; }

bool AKPCLNetworkBuildingBase::AddAsRepresentation()
{
	if (!bRepresentationEnabled)
	{
		return false;
	}
	if (AFGActorRepresentationManager* Manager = AFGActorRepresentationManager::Get(GetWorld()))
	{
		Manager->CreateAndAddNewRepresentation(this);
		return true;
	}
	return false;
}

bool AKPCLNetworkBuildingBase::UpdateRepresentation()
{
	if (!bRepresentationEnabled)
	{
		return false;
	}
	if (AFGActorRepresentationManager* Manager = AFGActorRepresentationManager::Get(GetWorld()))
	{
		Manager->UpdateRepresentationOfActor(this);
		return true;
	}
	return false;
}

bool AKPCLNetworkBuildingBase::RemoveAsRepresentation()
{
	if (!bRepresentationEnabled)
	{
		return false;
	}
	if (AFGActorRepresentationManager* Manager = AFGActorRepresentationManager::Get(GetWorld()))
	{
		Manager->RemoveRepresentationOfActor(this);
		return true;
	}
	return false;
}

bool AKPCLNetworkBuildingBase::IsActorStatic() { return true; }

FVector AKPCLNetworkBuildingBase::GetRealActorLocation() { return GetActorLocation(); }

FRotator AKPCLNetworkBuildingBase::GetRealActorRotation() { return GetActorRotation(); }

class UTexture2D* AKPCLNetworkBuildingBase::GetActorRepresentationTexture() { return mRepresentationIcon; }

FText AKPCLNetworkBuildingBase::GetActorRepresentationText()
{
	// Fixed: was calling mFaxitSubsystem without null check.
	if (!IsValid(mFaxitSubsystem))
	{
		return FText::FromString(TEXT("No Network"));
	}

	bool bSuccess = false;
	FKPCLFaxitNetworkInfo NetworkInfo = mFaxitSubsystem->GetNetworkByIdWithInfo(GetNetworkId(), bSuccess);
	if (!bSuccess)
	{
		return FText::FromString(TEXT("No Network"));
	}
	return FText::FromString(NetworkInfo.mRelatedNetwork.mNetworkName);
}

void AKPCLNetworkBuildingBase::SetActorRepresentationText(const FText& newText) {}

FLinearColor AKPCLNetworkBuildingBase::GetActorRepresentationColor() { return mRepresentationColor; }

void AKPCLNetworkBuildingBase::SetActorRepresentationColor(FLinearColor newColor) { mRepresentationColor = newColor; }

ERepresentationType AKPCLNetworkBuildingBase::GetActorRepresentationType() { return ERepresentationType::RT_Default; }

bool AKPCLNetworkBuildingBase::GetActorShouldShowInCompass() { return bRepresentationEnabled && bShouldShowInCompass; }

bool AKPCLNetworkBuildingBase::GetActorShouldShowOnMap() { return bRepresentationEnabled && bShouldShowOnMap; }

EFogOfWarRevealType AKPCLNetworkBuildingBase::GetActorFogOfWarRevealType() { return EFogOfWarRevealType::FOWRT_Static; }

float AKPCLNetworkBuildingBase::GetActorFogOfWarRevealRadius() { return mFogOfWarRevealRadius; }

ECompassViewDistance AKPCLNetworkBuildingBase::GetActorCompassViewDistance() { return mCompassViewDistance; }

void AKPCLNetworkBuildingBase::SetActorCompassViewDistance(ECompassViewDistance compassViewDistance)
{
	mCompassViewDistance = compassViewDistance;
}

UMaterialInterface* AKPCLNetworkBuildingBase::GetActorRepresentationCompassMaterial()
{
	return mRepresentationMaterial;
}

FPlayerInfoHandle AKPCLNetworkBuildingBase::GetLastEditedBy() const
{
	// No idea what do I need that for
	return FPlayerInfoHandle();
}

void AKPCLNetworkBuildingBase::SetActorLastEditedByHandle(const FPlayerInfoHandle& playerInfoHandle) {}

void AKPCLNetworkBuildingBase::BeginPlay()
{
	Super::BeginPlay();

	mFaxitSubsystem = AKPCLFaxitSubsystem::Get(GetWorld());

	TArray<UFGPowerInfoComponent*> Infos;
	GetComponents<UFGPowerInfoComponent>(Infos);
	for (UFGPowerInfoComponent* Info : Infos)
	{
		if (UFGPowerInfoComponent* AsPower = ExactCast<UFGPowerInfoComponent>(Info))
		{
			mPowerInfo = AsPower;
		}

		if (UKPCLNetworkInfoComponent* AsNetwork = ExactCast<UKPCLNetworkInfoComponent>(Info))
		{
			mNetworkInfoComponent = AsNetwork;
		}
	}
	mNetworkConnection = FindComponentByClass<UKPCLNetworkConnectionComponent>();

	if (HasAuthority())
	{
		AddAsRepresentation();
		if (GetNetworkConnectionComponent())
		{
			GetNetworkConnectionComponent()->SetPowerInfo(GetNetworkInfoComponent());
			if (bBindNetworkComponent)
			{
				GetNetworkConnectionComponent()->OnConnectionChanged.AddUObject(
					this, &AKPCLNetworkBuildingBase::OnCircuitChanged);
			}
		}

		if (IsValid(mFaxitSubsystem))
		{
			FKPCLFaxitNetwork* Network = mFaxitSubsystem->GetNetworkRef(GetNetworkId());
			if (!Network || !Network->mIsValid)
			{
				SetNetworkIdDirect(FString());
			}
			else if (!IsCore() && Network && IsValid(Network->mCore))
			{
				if (!mFaxitSubsystem->AddBuildingToCore(this, Network->mCore))
				{
					SetNetworkIdDirect(FString());
				}
			}
		}

		AKPCLUnlockSubsystem* UnlockSubsystem = AKPCLUnlockSubsystem::Get(GetWorld());
		check(UnlockSubsystem);
	}
}

void AKPCLNetworkBuildingBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (HasAuthority())
	{
		RemoveAsRepresentation();

		if (IsValid(mFaxitSubsystem))
		{
			mFaxitSubsystem->DestroyNetworkBuilding(this);
		}
	}
}

void AKPCLNetworkBuildingBase::Factory_Tick(float dt)
{
	Super::Factory_Tick(dt);

	if (HasAuthority())
	{
		const bool NewCableBoost = Factory_HasFaxitCableConnection();
		if (NewCableBoost != bHasCableBoost)
		{
			SetHasCableBoost(NewCableBoost);
		}

		FString NewNetworkId = GetNetworkId();
		if (!IsCore() && mConnectedNetworkId != NewNetworkId)
		{
			bool Success;
			FKPCLFaxitNetwork Network = mFaxitSubsystem->GetByNetworkId(NewNetworkId, Success);
			if (Success)
			{
				// Fixed: was capturing raw `this` and `Network` by value inside an AsyncTask without
				// guarding against UAF. Now uses RunOnGameThreadIfValid with a TWeakObjectPtr.
				RunOnGameThreadIfValid(this,
									   [Network](AKPCLNetworkBuildingBase* Self)
									   {
										   if (IsValid(Self->mFaxitSubsystem))
										   {
											   Self->mFaxitSubsystem->AddBuildingToCore(Self, Network.mCore);
										   }
									   });
			}
		}

		if (mDistanceChecker.Tick(dt))
		{
			UpdateDistanceToCore();
		}
	}
}

bool AKPCLNetworkBuildingBase::Factory_HasPower() const
{
	if (IsValid(GetFaxitCoreConst()) && GetFaxitCoreConst() != this && !IsCore())
	{
		return GetFaxitCoreConst()->Factory_HasPower();
	}
	return !GetNetworkId().IsEmpty();
}

bool AKPCLNetworkBuildingBase::Factory_IsProducing() const
{
	if (IsValid(GetFaxitCoreConst()) && GetFaxitCoreConst() != this && !IsCore())
	{
		return GetFaxitCoreConst()->IsProducing() && Super::Factory_IsProducing();
	}
	return Super::Factory_IsProducing();
}

void AKPCLNetworkBuildingBase::GetConditionalReplicatedProps(TArray<FFGCondReplicatedProperty>& outProps) const
{
	Super::GetConditionalReplicatedProps(outProps);
	FG_DOREPCONDITIONAL(ThisClass, bHasCableBoost);
	FG_DOREPCONDITIONAL(ThisClass, mNetworkId);
	FG_DOREPCONDITIONAL(ThisClass, mConnectedNetworkId);
	// Fixed: mDistanceToNetworkCore is now FGReplicated and must be registered here
	// so clients receive distance-based power consumption values.
	FG_DOREPCONDITIONAL(ThisClass, mDistanceToNetworkCore);
}

float AKPCLNetworkBuildingBase::GetPowerConsume() const
{
	return Super::GetPowerConsume() + GetPowerConsumeOnDistance();
}

float AKPCLNetworkBuildingBase::GetPowerConsumeOnDistance() const
{
	if (bHasCableBoost)
	{
		return 0.f;
	}
	return mDistancePowerModifier.GetValue(mDistanceToNetworkCore);
}

int32 AKPCLNetworkBuildingBase::SinkItems(FItemAmount Items, int32 MaxToSink)
{
	int32 Removed = 0;
	AFGResourceSinkSubsystem* Sub = GetSinkSub();
	if (IsValid(Sub))
	{
		for (int32 C = 0; C < FMath::Min(Items.Amount, MaxToSink); ++C)
		{
			if (!Sub->AddPoints_ThreadSafe(Items.ItemClass))
			{
				break;
			}
			Removed++;
		}
	}
	return Removed;
}

void AKPCLNetworkBuildingBase::RegisterInteractingPlayer_Implementation(AFGCharacterPlayer* player)
{
	Super::RegisterInteractingPlayer_Implementation(player);

	UKPCLNetwork* Network = Execute_GetNetwork(this);
	if (HasAuthority() && Network)
	{
		Network->RegisterInteractingPlayer(player);

		if (!IsCore())
		{
			if (Network->NetworkHasCore())
			{
				Execute_RegisterInteractingPlayer(Network->GetCore(), player);
			}
		}
	}
}

void AKPCLNetworkBuildingBase::UnregisterInteractingPlayer_Implementation(AFGCharacterPlayer* player)
{
	Super::UnregisterInteractingPlayer_Implementation(player);

	UKPCLNetwork* Network = Execute_GetNetwork(this);
	if (HasAuthority() && Network)
	{
		Network->UnregisterInteractingPlayer(player);

		if (!IsCore())
		{
			if (Network->NetworkHasCore())
			{
				Execute_UnregisterInteractingPlayer(Network->GetCore(), player);
			}
		}
	}
}

bool AKPCLNetworkBuildingBase::CanProduce_Implementation() const
{
	if (IsPlayingBuildEffect() || IsProductionPaused() || GetNetworkId().IsEmpty())
	{
		return false;
	}

	if (IsValid(GetFaxitCoreConst()) && !IsCore())
	{
		return GetFaxitCoreConst()->IsProducing();
	}

	return true;
}

void AKPCLNetworkBuildingBase::OnCircuitChanged(UFGCircuitConnectionComponent* Component)
{
	if (UKPCLNetworkConnectionComponent* NetworkConnection = Cast<UKPCLNetworkConnectionComponent>(Component))
	{
		SetHasCableBoost(IsValid(NetworkConnection->GetCore()));
	}
}

bool AKPCLNetworkBuildingBase::IsCore() const { return false; }

float AKPCLNetworkBuildingBase::GetCoreDistance() const
{
	if (bHasCableBoost)
	{
		return -1.f;
	}
	return mDistanceToNetworkCore;
}

UKPCLNetworkInfoComponent* AKPCLNetworkBuildingBase::GetNetworkInfoComponent() const
{
	if (mNetworkInfoComponent)
	{
		return mNetworkInfoComponent;
	}

	return FindComponentByClass<UKPCLNetworkInfoComponent>();
}

UKPCLNetworkConnectionComponent* AKPCLNetworkBuildingBase::GetNetworkConnectionComponent() const
{
	if (mNetworkConnection)
	{
		return mNetworkConnection;
	}

	return FindComponentByClass<UKPCLNetworkConnectionComponent>();
}

UFGPowerInfoComponent* AKPCLNetworkBuildingBase::GetPowerInfoExplicit() const
{
	if (ExactCast<UFGPowerInfoComponent>(GetPowerInfo()))
	{
		return GetPowerInfo();
	}

	TArray<UFGPowerInfoComponent*> Infos;
	GetComponents<UFGPowerInfoComponent>(Infos);
	for (UFGPowerInfoComponent* Info : Infos)
	{
		if (UFGPowerInfoComponent* AsExact = ExactCast<UFGPowerInfoComponent>(Info))
		{
			return AsExact;
		}
	}

	return nullptr;
}

UFGPowerConnectionComponent* AKPCLNetworkBuildingBase::GetPowerConnectionExplicit() const
{
	if (ExactCast<UFGPowerConnectionComponent>(GetPowerConnection()))
	{
		return GetPowerConnection();
	}

	TArray<UFGPowerConnectionComponent*> Infos;
	GetComponents<UFGPowerConnectionComponent>(Infos);
	for (UFGPowerConnectionComponent* Info : Infos)
	{
		if (UFGPowerConnectionComponent* AsExact = ExactCast<UFGPowerConnectionComponent>(Info))
		{
			return AsExact;
		}
	}

	return nullptr;
}

void AKPCLNetworkBuildingBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

TArray<FKPCLFaxitNetworkStatData> AKPCLNetworkBuildingBase::GetAndResetStats()
{
	// Fixed: mCurrentStates is written from the factory worker thread (AddDownloadToStats /
	// AddUploadToStats) and read+cleared here on the game thread. Guard with mStatsMutex.
	FScopeLock Lock(&mStatsMutex);
	TArray<FKPCLFaxitNetworkStatData> Stats;
	Stats.Append(mCurrentStates);
	mCurrentStates.Empty();
	return Stats;
}

FKPCLFaxitNetworkStatData* AKPCLNetworkBuildingBase::GetState(TSubclassOf<UFGItemDescriptor> Item)
{
	// Note: callers (AddDownloadToStats / AddUploadToStats) must hold mStatsMutex before calling.
	if (!IsValid(Item))
	{
		return nullptr;
	}

	for (FKPCLFaxitNetworkStatData& Stat : mCurrentStates)
	{
		if (Stat.mItem == Item)
		{
			return &Stat;
		}
	}

	FKPCLFaxitNetworkStatData& NewStat = mCurrentStates.AddDefaulted_GetRef();
	NewStat.mItem = Item;
	return &NewStat;
}

void AKPCLNetworkBuildingBase::AddDownloadToStats(TSubclassOf<UFGItemDescriptor> Item, int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}
	// Fixed: concurrent access with GetAndResetStats (game thread) — guard with mStatsMutex.
	FScopeLock Lock(&mStatsMutex);
	if (FKPCLFaxitNetworkStatData* State = GetState(Item))
	{
		State->mDownload += Amount;
	}
}

void AKPCLNetworkBuildingBase::AddUploadToStats(TSubclassOf<UFGItemDescriptor> Item, int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}
	// Fixed: concurrent access with GetAndResetStats (game thread) — guard with mStatsMutex.
	FScopeLock Lock(&mStatsMutex);
	if (FKPCLFaxitNetworkStatData* State = GetState(Item))
	{
		State->mUpload += Amount;
	}
}

AFGResourceSinkSubsystem* AKPCLNetworkBuildingBase::GetSinkSub() { return AFGResourceSinkSubsystem::Get(GetWorld()); }
