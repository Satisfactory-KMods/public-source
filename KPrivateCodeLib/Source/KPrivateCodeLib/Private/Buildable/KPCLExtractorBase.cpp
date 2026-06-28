// ILikeBanas

#include "Buildable/KPCLExtractorBase.h"

#include "AbstractInstanceManager.h"
#include "Async/TaskGraphInterfaces.h"
#include "Components/KPCLBetterIndicator.h"
#include "FGCrate.h"
#include "FGFactoryConnectionComponent.h"
#include "FGPowerConnectionComponent.h"
#include "FGPowerInfoComponent.h"
#include "KPrivateCodeLibModule.h"
#include "Net/UnrealNetwork.h"
#include "Replication/KPCLDefaultRCO.h"
#include "Resources/FGPowerShardDescriptor.h"
#include "Structures/KPCLFunctionalStructure.h"
#include "Subsystems/KBFLAssetDataSubsystem.h"

AKPCLExtractorBase::AKPCLExtractorBase() : mFGPowerConnection(nullptr), mCustomProductionStateIndicator(nullptr)
{
	bReplicates = true;
	mFactoryTickFunction.bCanEverTick = true;
	PrimaryActorTick.bCanEverTick = true;
}

#if WITH_EDITOR
void AKPCLExtractorBase::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(AKPCLExtractorBase, mPowerOptions))
	{
		mPowerConsumption = mPowerOptions.GetMaxPowerConsume();
	}
}

void AKPCLExtractorBase::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(AKPCLExtractorBase, mPowerOptions))
	{
		mPowerConsumption = mPowerOptions.GetMaxPowerConsume();
	}
}
#endif

bool AKPCLExtractorBase::Overclocking_IsConsumer_Implementation() { return false; }

void AKPCLExtractorBase::UI_ApplyRelevantItems_Implementation(TArray<TSubclassOf<UFGItemDescriptor>>& OutSlots)
{
	for (TSubclassOf<UFGItemDescriptor> RelevantItem : mRelevantItems)
	{
		if (IsValid(RelevantItem))
		{
			OutSlots.AddUnique(RelevantItem);
		}
	}

	UKBFLAssetDataSubsystem* AssetSubsystem = UKBFLAssetDataSubsystem::Get(this);
	if (IsValid(AssetSubsystem))
	{
		TArray<TSubclassOf<UFGItemDescriptor>> Items;
		AssetSubsystem->GetItemsOfChilds({UFGPowerShardDescriptor::StaticClass()}, Items, true);
		for (TSubclassOf<UFGItemDescriptor> Item : Items)
		{
			TSubclassOf<UFGPowerShardDescriptor> PowerShard = TSubclassOf<UFGPowerShardDescriptor>(Item);
			if (IsValid(PowerShard) &&
				UFGPowerShardDescriptor::GetPowerShardType(PowerShard) == EPowerShardType::PST_Overclock)
			{
				OutSlots.AddUnique(PowerShard);
			}
		}
	}
}

UFGInventoryComponent* AKPCLExtractorBase::Overclocking_GetInventory_Implementation()
{
	return GetPotentialInventory();
}

bool AKPCLExtractorBase::Overclocking_ShouldUse_Implementation() { return bEnableCustomOverclocking; }

bool AKPCLExtractorBase::Overclocking_UseInventory_Implementation(int32& UnlockedSlots)
{
	UnlockedSlots = 0;
	if (IsValid(GetPotentialInventory()))
	{
		TArray<FInventoryStack> Stacks;
		GetPotentialInventory()->GetInventoryStacks(Stacks);
		for (const FInventoryStack& Stack : Stacks)
		{
			if (!Stack.HasItems())
			{
				continue;
			}

			if (TSubclassOf<UFGPowerShardDescriptor> PowerShard =
					TSubclassOf<UFGPowerShardDescriptor>(Stack.Item.GetItemClass()))
			{
				EPowerShardType Type = UFGPowerShardDescriptor::GetPowerShardType(PowerShard);
				if (Type == EPowerShardType::PST_Overclock)
				{
					UnlockedSlots += Stack.NumItems;
				}
			}
		}

		return true;
	}

	return false;
}

AFGBuildableFactory* AKPCLExtractorBase::Overclocking_GetFactory_Implementation()
{
	return Cast<AFGBuildableFactory>(this);
}

void AKPCLExtractorBase::Overclocking_GetCostSlots_Implementation(TArray<FItemAmount>& OutSlots)
{
	if (IsValid(GetPotentialInventory()))
	{
		TArray<FInventoryStack> Stacks;
		GetPotentialInventory()->GetInventoryStacks(Stacks);
		for (const FInventoryStack& Stack : Stacks)
		{
			if (!Stack.HasItems())
			{
				continue;
			}

			if (TSubclassOf<UFGPowerShardDescriptor> PowerShard =
					TSubclassOf<UFGPowerShardDescriptor>(Stack.Item.GetItemClass()))
			{
				EPowerShardType Type = UFGPowerShardDescriptor::GetPowerShardType(PowerShard);
				if (Type == EPowerShardType::PST_Overclock)
				{
					int32 Count = Stack.NumItems;
					while (Count > 0)
					{
						OutSlots.Add(FItemAmount(Stack.Item.GetItemClass(), 1));
						Count--;
					}
				}
			}
		}
	}
}

bool AKPCLExtractorBase::CanUseFactoryClipboard_Implementation()
{
	int32 UnlockedSlots = 0;
	return bEnableCustomOverclocking && Execute_Overclocking_UseInventory(this, UnlockedSlots);
}

UFGFactoryClipboardSettings* AKPCLExtractorBase::CopySettings_Implementation()
{
	UKPCLBaseClipboardSettings* Settings = NewObject<UKPCLBaseClipboardSettings>();
	WriteClipboardSettings(Settings);
	return Settings;
}

bool AKPCLExtractorBase::PasteSettings_Implementation(UFGFactoryClipboardSettings* factoryClipboard,
													  class AFGPlayerController* player)
{
	if (UKPCLBaseClipboardSettings* Settings = Cast<UKPCLBaseClipboardSettings>(factoryClipboard))
	{
		ApplyClipboardSettings(Settings, player);
		return true;
	}
	return Super::PasteSettings_Implementation(factoryClipboard, player);
}

void AKPCLExtractorBase::WriteClipboardSettings(UKPCLBaseClipboardSettings* ClipboardSettings)
{
	int32 UnlockedSlots = 0;
	if (Execute_Overclocking_UseInventory(this, UnlockedSlots))
	{
		if (IsValid(GetPotentialInventory()))
		{
			TArray<FInventoryStack> Stacks;
			GetPotentialInventory()->GetInventoryStacks(Stacks);
			for (const FInventoryStack& Stack : Stacks)
			{
				if (!Stack.HasItems())
				{
					continue;
				}

				if (TSubclassOf<UFGPowerShardDescriptor> PowerShard =
						TSubclassOf<UFGPowerShardDescriptor>(Stack.Item.GetItemClass()))
				{
					EPowerShardType Type = UFGPowerShardDescriptor::GetPowerShardType(PowerShard);
					if (Type == EPowerShardType::PST_Overclock)
					{
						ClipboardSettings->mShards.Add(FItemAmount(PowerShard, 1));
					}
				}
			}
		}
	}
	ClipboardSettings->bIsPaused = IsProductionPaused();
	ClipboardSettings->mTargetPotential = GetPendingPotential();
}

void AKPCLExtractorBase::ApplyClipboardSettings(UKPCLBaseClipboardSettings* ClipboardSettings,
												AFGPlayerController* Player)
{
	int32 UnlockedSlots = 0;
	if (Execute_Overclocking_UseInventory(this, UnlockedSlots) && IsValid(GetPotentialInventory()) && IsValid(Player))
	{
		if (AFGCharacterPlayer* Char = Cast<AFGCharacterPlayer>(Player->GetCharacter()))
		{
			if (IsValid(Char->GetInventory()))
			{
				for (FItemAmount ItemAmount : ClipboardSettings->mShards)
				{
					if (Char->GetInventory()->HasItems(ItemAmount.ItemClass, ItemAmount.Amount))
					{
						int32 Added =
							GetPotentialInventory()->AddStack(FInventoryStack(ItemAmount.Amount, ItemAmount.ItemClass));
						if (Added > 0)
						{
							Char->GetInventory()->Remove(ItemAmount.ItemClass, ItemAmount.Amount);
						}
					}
				}
			}
		}
	}

	SetIsProductionPaused(ClipboardSettings->bIsPaused);
	SetPendingPotential(ClipboardSettings->mTargetPotential);
}

void AKPCLExtractorBase::Overclocking_GetInfo_Implementation(FKPCLOverclockingProductionInfo& OutProductionInfo)
{
	OutProductionInfo.mDefaultProductionTime = mProductionHandle.mProductionTime;
}

bool AKPCLExtractorBase::ShouldSave_Implementation() const { return true; }

void AKPCLExtractorBase::InitInventories()
{
	if (!HasAuthority())
	{
		return;
	}

	mCachedInventorys.Empty();
	TArray<UFGInventoryComponent*> Components;
	GetComponents(Components);
	for (UFGInventoryComponent* Component : Components)
	{
		if (!IsValid(Component))
		{
			continue;
		}
		FName ComponentName = FName(Component->GetName());
		if (mCachedInventorys.Contains(ComponentName))
		{
			UE_LOG(LogKPCL, Error, TEXT("InitInventories, with duplicate inventory %s in actor %s! > SKIP!"),
				   *ComponentName.ToString(), *GetName());
			continue;
		}
		mCachedInventorys.Add(ComponentName, Component);
	}

	if (IsValid(GetInventory()))
	{
		InitInputInventory();
		GetInventory()->SetReplicationRelevancyOwner(this);
	}

	if (IsValid(GetOutputInventory()))
	{
		InitOutputInventory();
		GetOutputInventory()->SetReplicationRelevancyOwner(this);
	}

	if (IsValid(GetBoosterInventory()))
	{
		InitBoosterInventory();
		GetBoosterInventory()->SetReplicationRelevancyOwner(this);
	}

	SetBelts();
}

void AKPCLExtractorBase::SetBelts()
{
	TArray<UFGFactoryConnectionComponent*> BeltsToSet = GetAllConv();
	if (BeltsToSet.Num() > 0)
	{
		for (UFGFactoryConnectionComponent* BeltConnection : BeltsToSet)
		{
			if (BeltConnection->GetDirection() != EFactoryConnectionDirection::FCD_OUTPUT && IsValid(GetInventory()))
			{
				BeltConnection->SetInventory(GetInventory());
			}
			else if (IsValid(GetOutputInventory()) &&
					 BeltConnection->GetDirection() == EFactoryConnectionDirection::FCD_OUTPUT)
			{
				BeltConnection->SetInventory(GetOutputInventory());
			}
		}
	}

	/*TArray<UFGPipeConnectionFactory*> PipesToSet = GetAllPipes();
	if (PipesToSet.Num() > 0)
	{
		for (UFGPipeConnectionFactory* PipeConnection : PipesToSet)
		{
			if (PipeConnection->GetPipeConnectionType() != EPipeConnectionType::PCT_CONSUMER && IsValid(GetInventory()))
			{
				PipeConnection->SetInventory(GetInventory());
			} else if(IsValid(GetOutputInventory()) && PipeConnection->GetPipeConnectionType() ==
	EPipeConnectionType::PCT_CONSUMER)
			{
				PipeConnection->SetInventory(GetOutputInventory());
			}
		}
	}*/
}

void AKPCLExtractorBase::HandleIndicator()
{
	if (!GetHasPower())
	{
		SetCurrentProductionState(ENewProductionState::NoPower);
		return;
	}
	if (IsProductionPaused())
	{
		SetCurrentProductionState(ENewProductionState::Paused);
		return;
	}
	if (IsProducing())
	{
		SetCurrentProductionState(ENewProductionState::Producing);
		return;
	}
	SetCurrentProductionState(ENewProductionState::Idle);
}

void AKPCLExtractorBase::ApplyNewProductionState(ENewProductionState NewState)
{
	mLastState = NewState;
	SetCurrentProductionState(NewState);

	if (mCustomProductionStateIndicator)
	{
		// Factory_Tick runs on a worker thread. UObject method calls must happen on the game
		// thread to avoid GC/visibility races. Self-marshal when called off the game thread
		// (Phase 2 fix). When already on the game thread (OnRep, BeginPlay, etc.) call directly.
		if (IsInGameThread())
		{
			mCustomProductionStateIndicator->SetState(NewState);
		}
		else
		{
			TObjectPtr<UKPCLBetterIndicator> Indicator = mCustomProductionStateIndicator;
			FFunctionGraphTask::CreateAndDispatchWhenReady(
				[Indicator, NewState]()
				{
					if (IsValid(Indicator))
					{
						Indicator->SetState(NewState);
					}
				},
				TStatId(), nullptr, ENamedThreads::GameThread);
		}
	}

	bOneStateWasNotApplied = false;
	for (const int32 CustomIndicatorHandleIndex : mCustomIndicatorHandleIndexes)
	{
		if (mInstanceHandles.IsValidIndex(CustomIndicatorHandleIndex))
		{
			if (!mInstanceHandles[CustomIndicatorHandleIndex]->IsInstanced())
			{
				bOneStateWasNotApplied = true;
				continue;
			}
			AIO_UpdateCustomFloat(mDefaultIndex, mIntensity, CustomIndicatorHandleIndex, true);
			AIO_UpdateCustomFloatAsColor(mDefaultIndex + 1, mStateColors[static_cast<uint8>(NewState)],
										 CustomIndicatorHandleIndex, true);
			AIO_UpdateCustomFloat(mDefaultIndex + 4, mPulsStates.Contains(NewState), CustomIndicatorHandleIndex, true);
		}
	}
}

UFGFactoryConnectionComponent* AKPCLExtractorBase::GetConv(int Index, ECKPCLDirection Direction) const
{
	if ((Direction == KPCLInput || Direction == KPCLAny) &&
		mConnectionMap[EKPCLConnectionType::ConvIn].IsValidIndex(Index))
	{
		return Cast<UFGFactoryConnectionComponent>(mConnectionMap[EKPCLConnectionType::ConvIn][Index]);
	}

	if ((Direction == KPCLOutput || Direction == KPCLAny) &&
		mConnectionMap[EKPCLConnectionType::ConvOut].IsValidIndex(Index))
	{
		return Cast<UFGFactoryConnectionComponent>(mConnectionMap[EKPCLConnectionType::ConvOut][Index]);
	}

	return nullptr;
}

TArray<UFGFactoryConnectionComponent*> AKPCLExtractorBase::GetAllConv(ECKPCLDirection Direction) const
{
	TArray<UFGFactoryConnectionComponent*> Return;
	TArray<UFGConnectionComponent*> All;

	if (Direction == KPCLInput || Direction == KPCLAny)
	{
		All.Append(mConnectionMap[EKPCLConnectionType::ConvIn]);
	}
	if (Direction == KPCLOutput || Direction == KPCLAny)
	{
		All.Append(mConnectionMap[EKPCLConnectionType::ConvOut]);
	}

	for (UFGConnectionComponent* Conv : All)
	{
		if (IsValid(Conv))
		{
			Return.AddUnique(Cast<UFGFactoryConnectionComponent>(Conv));
		}
	}

	return Return;
}

UFGPipeConnectionFactory* AKPCLExtractorBase::GetPipe(int Index, ECKPCLDirection Direction) const
{
	if ((Direction == KPCLInput || Direction == KPCLAny) &&
		mConnectionMap[EKPCLConnectionType::PipeIn].IsValidIndex(Index))
	{
		return Cast<UFGPipeConnectionFactory>(mConnectionMap[EKPCLConnectionType::PipeIn][Index]);
	}

	if ((Direction == KPCLOutput || Direction == KPCLAny) &&
		mConnectionMap[EKPCLConnectionType::PipeOut].IsValidIndex(Index))
	{
		return Cast<UFGPipeConnectionFactory>(mConnectionMap[EKPCLConnectionType::PipeOut][Index]);
	}

	return nullptr;
}

TArray<UFGPipeConnectionFactory*> AKPCLExtractorBase::GetAllPipes(ECKPCLDirection Direction) const
{
	TArray<UFGPipeConnectionFactory*> Return;
	TArray<UFGConnectionComponent*> All;

	if (Direction == KPCLInput || Direction == KPCLAny)
	{
		All.Append(mConnectionMap[EKPCLConnectionType::PipeIn]);
	}
	if (Direction == KPCLOutput || Direction == KPCLAny)
	{
		All.Append(mConnectionMap[EKPCLConnectionType::PipeOut]);
	}

	for (UFGConnectionComponent* Conv : All)
	{
		if (IsValid(Conv))
		{
			Return.AddUnique(Cast<UFGPipeConnectionFactory>(Conv));
		}
	}

	return Return;
}

void AKPCLExtractorBase::BeginPlay()
{
	Super::BeginPlay();
	mAssetSubsystem = UKAPIDataAssetSubsystem::GetChecked(GetWorld());

	InitComponents();

	if (HasAuthority())
	{
		InitInventories();
		HandlePowerInit();
		SetBelts();
		SetPendingPotential(GetPendingPotential());
	}

	if (!bDeferBeginPlay)
	{
		ReadyForVisuelUpdate();
	}

	InitAudioConfig();
}

void AKPCLExtractorBase::OnBuildEffectFinished()
{
	Super::OnBuildEffectFinished();
	ReadyForVisuelUpdate();
}

void AKPCLExtractorBase::OnBuildEffectActorFinished()
{
	Super::OnBuildEffectActorFinished();
	ReadyForVisuelUpdate();
}

void AKPCLExtractorBase::ReadyForVisuelUpdate()
{
	InitMeshOverwriteInformation();
	ApplyNewProductionState(GetCurrentProductionState());
}

void AKPCLExtractorBase::InitAudioConfig()
{
	UConfigPropertyFloat* Property = mAudioConfig.GetPropertyAsType(GetWorld());
	if (IsValid(Property))
	{
		TArray<UAudioComponent*> AudioComponents;
		GetComponents(AudioComponents);

		if (AudioComponents.Num() > 0)
		{
			for (UAudioComponent* AudioComponent : AudioComponents)
			{
				mAudioComponents.Add(FKPCLAudioComponent(AudioComponent));
			}
			Property->OnPropertyValueChanged.AddUniqueDynamic(this, &AKPCLExtractorBase::OnAudioConfigChanged_Native);
			OnAudioConfigChanged_Native();
		}
	}
}

void AKPCLExtractorBase::OnAudioConfigChanged_Native()
{
	for (FKPCLAudioComponent AudioComponent : mAudioComponents)
	{
		AudioComponent.SetVolumePercent(mAudioConfig.GetValue(GetWorld()));
	}
	OnAudioConfigChanged();
}

void AKPCLExtractorBase::StartIsLookedAtForDismantle_Implementation(AFGCharacterPlayer* byCharacter)
{
	UpdateInstancesForOutline();
	Super::StartIsLookedAtForDismantle_Implementation(byCharacter);
}

void AKPCLExtractorBase::StartIsLookedAt_Implementation(AFGCharacterPlayer* byCharacter, const FUseState& state)
{
	UpdateInstancesForOutline();
	Super::StartIsLookedAt_Implementation(byCharacter, state);
}

void AKPCLExtractorBase::StartIsAimedAtForColor_Implementation(AFGCharacterPlayer* byCharacter, bool isValid)
{
	UpdateInstancesForOutline();
	Super::StartIsAimedAtForColor_Implementation(byCharacter, isValid);
}

void AKPCLExtractorBase::StartIsLookedAtForConnection(AFGCharacterPlayer* byCharacter,
													  UFGCircuitConnectionComponent* overlappingConnection)
{
	UpdateInstancesForOutline();
	Super::StartIsLookedAtForConnection(byCharacter, overlappingConnection);
}

void AKPCLExtractorBase::UpdateInstancesForOutline() const
{
	TArray<UKPCLColoredStaticMesh*> ColoredStaticMeshes;
	GetComponents<UKPCLColoredStaticMesh>(ColoredStaticMeshes);
	for (UKPCLColoredStaticMesh* ColoredStaticMesh : ColoredStaticMeshes)
	{
		if (ColoredStaticMesh)
		{
			ColoredStaticMesh->ApplyTransformToComponent();
		}
	}
}

void AKPCLExtractorBase::InitMeshOverwriteInformation()
{
	if (!DoesContainLightweightInstances_Native())
	{
		return;
	}

	if (mInstanceHandles.Num() <= 0)
	{
		GetWorldTimerManager().SetTimerForNextTick(this, &AKPCLExtractorBase::InitMeshOverwriteInformation);
		return;
	}

	if (!mInstanceHandles[0]->IsInstanced())
	{
		GetWorldTimerManager().SetTimerForNextTick(this, &AKPCLExtractorBase::InitMeshOverwriteInformation);
		return;
	}

	for (int32 Idx = 0; Idx < mInstanceHandles.Num(); ++Idx)
	{
		ApplyMeshOverwriteInformation(Idx);

		FKPCLMeshOverwriteInformation Information;
		if (ShouldOverwriteIndexHandle(Idx, Information))
		{
			ApplyMeshInformation(Information);
		}
	}
	ApplyNewProductionState(GetCurrentProductionState());
}

void AKPCLExtractorBase::ReApplyColorForIndex(int32 Idx, const FFactoryCustomizationData& customizationData)
{
	if (!mInstanceHandles.IsValidIndex(Idx) || !DoesContainLightweightInstances_Native() || !IsValid(mInstanceDataCDO))
	{
		return;
	}

	if (mInstanceHandles[Idx]->IsInstanced())
	{
		if (!mInstanceDataCDO->GetInstanceData().IsValidIndex(Idx))
		{
			return;
		}
		int32 NewNum = FMath::Clamp(mInstanceDataCDO->GetInstanceData()[Idx].NumCustomDataFloats, 0, 100);
		TArray<float> Datas = customizationData.Data;

		Datas.SetNum(NewNum);
		if (mCachedCustomData.Contains(Idx))
		{
			for (TTuple<int, float> Result : mCachedCustomData[Idx])
			{
				if (!Datas.IsValidIndex(Result.Key))
				{
					continue;
				}
				Datas[Result.Key] = Result.Value;
			}
		}
		AAbstractInstanceManager::SetCustomPrimitiveDataOnHandle(mInstanceHandles[Idx], Datas, true);
	}
}

void AKPCLExtractorBase::ApplyCustomizationData_Native(const FFactoryCustomizationData& customizationData)
{
	Super::ApplyCustomizationData_Native(customizationData);

	if (DoesContainLightweightInstances_Native())
	{
		for (int32 Idx = 0; Idx < mInstanceHandles.Num(); ++Idx)
		{
			ReApplyColorForIndex(Idx, customizationData);
		}
	}
}

void AKPCLExtractorBase::SetCustomizationData_Native(const FFactoryCustomizationData& customizationData,
													 bool skipCombine)
{
	Super::SetCustomizationData_Native(customizationData, skipCombine);

	if (DoesContainLightweightInstances_Native())
	{
		for (int32 Idx = 0; Idx < mInstanceHandles.Num(); ++Idx)
		{
			ReApplyColorForIndex(Idx, customizationData);
		}
	}
}

void AKPCLExtractorBase::ApplyMeshOverwriteInformation(int32 Idx)
{
	for (FKPCLMeshOverwriteInformation MeshOverwriteInformation : mDefaultMeshOverwriteInformations)
	{
		if (MeshOverwriteInformation.mOverwriteHandleIndex == Idx)
		{
			ApplyMeshInformation(MeshOverwriteInformation);
			return;
		}
	}

	for (FKPCLMeshOverwriteInformation MeshOverwriteInformation : mMeshOverwriteInformations)
	{
		if (MeshOverwriteInformation.mOverwriteHandleIndex == Idx)
		{
			ApplyMeshInformation(MeshOverwriteInformation);
			return;
		}
	}
}

void AKPCLExtractorBase::ApplyMeshInformation(FKPCLMeshOverwriteInformation Information)
{
	if (!Information.mUseCustomTransform)
	{
		AIO_OverwriteInstanceData(Information.mOverwriteMesh, Information.mOverwriteHandleIndex);
	}
	else
	{
		AIO_OverwriteInstanceData_Transform(Information.mOverwriteMesh, Information.mCustomTransform,
											Information.mOverwriteHandleIndex);
	}
}

bool AKPCLExtractorBase::ShouldOverwriteIndexHandle(int32 Idx, FKPCLMeshOverwriteInformation& Information)
{
	return false;
}

bool AKPCLExtractorBase::AIO_OverwriteInstanceData(UStaticMesh* Mesh, int32 Idx)
{
	if (!IsInGameThread())
	{
		FFunctionGraphTask::CreateAndDispatchWhenReady([&, Mesh, Idx]() { AIO_OverwriteInstanceData(Mesh, Idx); },
													   GET_STATID(STAT_TaskGraph_OtherTasks), nullptr,
													   ENamedThreads::GameThread);
		return true;
	}

	if (Idx > INDEX_NONE && IsValid(Mesh) && IsValid(mInstanceDataCDO))
	{
		TArray<FInstanceData> Datas = mInstanceDataCDO->GetInstanceData();
		if (Datas.IsValidIndex(Idx))
		{
			return AIO_OverwriteInstanceData_Transform(Mesh, Datas[Idx].RelativeTransform, Idx);
		}
	}
	return false;
}

bool AKPCLExtractorBase::AIO_OverwriteInstanceData_Transform(UStaticMesh* Mesh, FTransform NewRelativTransform,
															 int32 Idx)
{
	if (!IsInGameThread())
	{
		FFunctionGraphTask::CreateAndDispatchWhenReady(
			[&, Mesh, NewRelativTransform, Idx]()
			{ AIO_OverwriteInstanceData_Transform(Mesh, NewRelativTransform, Idx); },
			GET_STATID(STAT_TaskGraph_OtherTasks), nullptr, ENamedThreads::GameThread);
		return true;
	}

	if (Idx > INDEX_NONE && IsValid(Mesh) && IsValid(mInstanceDataCDO))
	{
		AAbstractInstanceManager* Manager = AAbstractInstanceManager::GetInstanceManager(GetWorld());
		TArray<FInstanceData> Datas = mInstanceDataCDO->GetInstanceData();
		if (Datas.IsValidIndex(Idx) && mInstanceHandles.IsValidIndex(Idx) && IsValid(Manager))
		{
			FInstanceData Data = Datas[Idx];
			Data.RelativeTransform = NewRelativTransform;
			Data.StaticMesh = Mesh;

			if (mInstanceHandles[Idx]->IsInstanced())
			{
				Manager->RemoveInstance(mInstanceHandles[Idx]);
			}

			FInstanceOwnerHandlePtr newHandle;
			mInstanceHandles[Idx] = newHandle;
			Manager->SetInstanced(this, GetActorTransform(), Data, mInstanceHandles[Idx]);
			mCachedTransforms.Add(Idx, NewRelativTransform * GetActorTransform());

			ApplyCustomizationData_Native(mCustomizationData);
			SetCustomizationData_Native(mCustomizationData, false);

			return true;
		}
	}

	return false;
}

bool AKPCLExtractorBase::AIO_UpdateCustomFloat(int32 FloatIndex, float Data, int32 InstanceIdx, bool MarkDirty)
{
	if (!IsInGameThread())
	{
		FFunctionGraphTask::CreateAndDispatchWhenReady(
			[&, FloatIndex, Data, InstanceIdx, MarkDirty]()
			{ AIO_UpdateCustomFloat(FloatIndex, Data, InstanceIdx, MarkDirty); }, GET_STATID(STAT_TaskGraph_OtherTasks),
			nullptr, ENamedThreads::GameThread);
		return true;
	}

	if (mInstanceHandles.IsValidIndex(InstanceIdx))
	{
		if (mInstanceHandles[InstanceIdx]->IsInstanced())
		{
			// UE_LOG( LogKPCL, Error, TEXT("mInstanceHandles[ %d ]->SetPrimitiveDataByID( %f, %d, %d ); IsValid(%d),
			// Component(%d), Owner(%d)"), InstanceIdx, Data, FloatIndex, MarkDirty, mInstanceHandles[ InstanceIdx
			// ]->IsValid(), IsValid( mInstanceHandles[ InstanceIdx ]->GetInstanceComponent() ), mInstanceHandles[
			// InstanceIdx ]->GetOwner() == this )
			mInstanceHandles[InstanceIdx]->SetPrimitiveDataByID(Data /** float that we want to set */,
																FloatIndex /** Index where we want to set */, true);
			if (mCachedCustomData.Contains(InstanceIdx))
			{
				mCachedCustomData[InstanceIdx].Add(FloatIndex, Data);
			}
			else
			{
				TMap<int32, float> Map;
				Map.Add(FloatIndex, Data);
				mCachedCustomData.Add(InstanceIdx, Map);
			}
			return true;
		}
	}

	return false;
}

bool AKPCLExtractorBase::AIO_UpdateCustomFloatAsColor(int32 StartFloatIndex, FLinearColor Data, int32 InstanceIdx,
													  bool MarkDirty)
{
	if (!IsInGameThread())
	{
		FFunctionGraphTask::CreateAndDispatchWhenReady(
			[&, StartFloatIndex, Data, InstanceIdx, MarkDirty]()
			{ AIO_UpdateCustomFloatAsColor(StartFloatIndex, Data, InstanceIdx, MarkDirty); },
			GET_STATID(STAT_TaskGraph_OtherTasks), nullptr, ENamedThreads::GameThread);
		return true;
	}

	AIO_UpdateCustomFloat(StartFloatIndex, Data.R, InstanceIdx, MarkDirty);
	AIO_UpdateCustomFloat(StartFloatIndex + 1, Data.G, InstanceIdx, MarkDirty);
	return AIO_UpdateCustomFloat(StartFloatIndex + 2, Data.B, InstanceIdx, MarkDirty);
}

bool AKPCLExtractorBase::AIO_SetInstanceHidden(int32 InstanceIdx, bool IsHidden)
{
	if (!IsInGameThread())
	{
		FFunctionGraphTask::CreateAndDispatchWhenReady(
			[&, InstanceIdx, IsHidden]() { AIO_SetInstanceHidden(InstanceIdx, IsHidden); },
			GET_STATID(STAT_TaskGraph_OtherTasks), nullptr, ENamedThreads::GameThread);
		return true;
	}

	if (mInstanceHandles.IsValidIndex(InstanceIdx))
	{
		if (mInstanceHandles[InstanceIdx]->IsInstanced())
		{
			FTransform T;
			if (mCachedTransforms.Contains(InstanceIdx))
			{
				T = mCachedTransforms[InstanceIdx];
			}
			else if (IsValid(mInstanceDataCDO) && mInstanceDataCDO->GetInstanceData().IsValidIndex(InstanceIdx))
			{
				T = mInstanceDataCDO->GetInstanceData()[InstanceIdx].RelativeTransform * GetActorTransform();
			}
			else
			{
				T = GetActorTransform();
			}
			T.SetScale3D(!IsHidden ? T.GetScale3D() : FVector(0.001f));
			T.SetLocation(!IsHidden ? T.GetLocation() : FVector(0.001f));

			mInstanceHandles[InstanceIdx]->SetLocalTransform(T);
			return true;
		}
	}

	return false;
}

bool AKPCLExtractorBase::AIO_SetInstanceWorldTransform(int32 InstanceIdx, FTransform Transform)
{
	if (!IsInGameThread())
	{
		FFunctionGraphTask::CreateAndDispatchWhenReady(
			[&, InstanceIdx, Transform]() { AIO_SetInstanceWorldTransform(InstanceIdx, Transform); },
			GET_STATID(STAT_TaskGraph_OtherTasks), nullptr, ENamedThreads::GameThread);
		return true;
	}

	if (mInstanceHandles.IsValidIndex(InstanceIdx))
	{
		if (mInstanceHandles[InstanceIdx]->IsInstanced())
		{
			mInstanceHandles[InstanceIdx]->SetLocalTransform(Transform);
			mCachedTransforms.Add(InstanceIdx, Transform);
			return true;
		}
	}

	return false;
}

void AKPCLExtractorBase::Factory_Tick(float dt)
{
	Super::Factory_Tick(dt);

	if (bCustomFactoryTickPreCustomLogic)
	{
		if (HasAuthority())
		{
			Factory_TickAuthOnly(dt);
		}
		else
		{
			Factory_TickClientOnly(dt);
		}
	}

	if (HasAuthority())
	{
		float Pending = GetPendingPotential();
		float Limit = GetCurrentMaxPotential();
		float MinLimit = GetCurrentMinPotential();
		if (Pending > Limit)
		{
			FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(
				FSimpleDelegateGraphTask::FDelegate::CreateLambda([&, Limit] { SetPendingPotential(Limit); }),
				TStatId(), nullptr, ENamedThreads::GameThread);
		}
		else if (Pending < MinLimit)
		{
			FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(
				FSimpleDelegateGraphTask::FDelegate::CreateLambda([&, MinLimit] { SetPendingPotential(MinLimit); }),
				TStatId(), nullptr, ENamedThreads::GameThread);
		}

		float Current = GetCurrentPotential();
		if (Current > Limit)
		{
			FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(
				FSimpleDelegateGraphTask::FDelegate::CreateLambda([&, Limit] { SetCurrentPotential(Limit); }),
				TStatId(), nullptr, ENamedThreads::GameThread);
		}
		else if (Current < MinLimit)
		{
			FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(
				FSimpleDelegateGraphTask::FDelegate::CreateLambda([&, MinLimit] { SetCurrentPotential(MinLimit); }),
				TStatId(), nullptr, ENamedThreads::GameThread);
		}

		BeltPipeGrab(dt);
		HandleUiTick(dt);

		mProductionHandle.TickHandle(dt, IsProducing(),
									 [&]()
									 {
										 EndProductionTime();
										 onProducingFinal();
									 });
		mPropertyReplicator.MarkPropertyDirty(FName("mProductionHandle"));

		mPowerOptions.mCurrentPotential = mProductionHandle.mCurrentPotential;

		if (!mPowerOptions.bWasInit)
		{
			HandlePowerInit();
		}

		if (GetPowerInfo() && mPowerOptions.bWasInit)
		{
			HandlePower(dt);
		}
		mPropertyReplicator.MarkPropertyDirty(FName("mPowerOptions"));
	}

	if (mCustomProductionStateIndicator || mCustomIndicatorHandleIndexes.Num() > 0)
	{
		if (HasAuthority())
		{
			HandleIndicator();
		}
		if (GetCurrentCompProductionState() != GetCurrentProductionState() || bOneStateWasNotApplied)
		{
			ApplyNewProductionState(GetCurrentProductionState());
		}
	}

	if (!bCustomFactoryTickPreCustomLogic)
	{
		if (HasAuthority())
		{
			Factory_TickAuthOnly(dt);
		}
		else
		{
			Factory_TickClientOnly(dt);
		}
	}
}

void AKPCLExtractorBase::Factory_TickAuthOnly(float dt) {}

void AKPCLExtractorBase::Factory_TickClientOnly(float dt) {}

bool AKPCLExtractorBase::CanProduce_Implementation() const
{
	if (IsPlayingBuildEffect())
	{
		return false;
	}

	return !IsProductionPaused();
}

void AKPCLExtractorBase::EndProductionTime() { SetCurrentPotential(GetPendingPotential()); }

void AKPCLExtractorBase::BeltPipeGrab(float dt) {}

void AKPCLExtractorBase::HandlePower(float dt)
{
	mPowerOptions.bHasPower = HasPower();
	mPowerOptions.StructureTick(dt, IsProducing());
	GetPowerInfo()->SetTargetConsumption(mPowerOptions.mIsProducer ? FMath::Max(mPowerOptions.mForcePowerConsume, 0.1f)
																   : mPowerOptions.GetPowerConsume());
	GetPowerInfo()->SetMaximumTargetConsumption(mPowerOptions.mIsProducer
													? FMath::Max(mPowerOptions.mForcePowerConsume, 0.1f)
													: mPowerOptions.GetMaxPowerConsume());
	GetPowerInfo()->SetBaseProduction(!mPowerOptions.mIsProducer ||
											  (!mPowerOptions.mIsProducer && mPowerOptions.mIsDynamicProducer)
										  ? 0.0f
										  : mPowerOptions.GetMaxPowerConsume());
	GetPowerInfo()->SetDynamicProductionCapacity(
		!mPowerOptions.mIsProducer || !mPowerOptions.mIsDynamicProducer ? 0.0f : mPowerOptions.GetMaxPowerConsume());
	GetPowerInfo()->SetFullBlast((!mPowerOptions.mIsProducer || !mPowerOptions.mIsDynamicProducer));
}

void AKPCLExtractorBase::HandlePowerInit()
{
	mPowerOptions.Init();
	mPropertyReplicator.MarkPropertyDirty(FName("mPowerOptions"));
}

void AKPCLExtractorBase::HandleUiTick(float dt) {}

void AKPCLExtractorBase::InitComponents()
{
	mCustomProductionStateIndicator = FindComponentByClass<UKPCLBetterIndicator>();

	mConnectionMap.Add(EKPCLConnectionType::ConvIn, {});
	mConnectionMap.Add(EKPCLConnectionType::ConvOut, {});
	mConnectionMap.Add(EKPCLConnectionType::PipeIn, {});
	mConnectionMap.Add(EKPCLConnectionType::PipeOut, {});

	TArray<UFGPowerConnectionComponent*> Infos;
	GetComponents<UFGPowerConnectionComponent>(Infos);
	for (UFGPowerConnectionComponent* Info : Infos)
	{
		if (UFGPowerConnectionComponent* AsPower = ExactCast<UFGPowerConnectionComponent>(Info))
		{
			mFGPowerConnection = AsPower;
		}
	}

	TArray<UActorComponent*> Components;
	GetComponents(Components);

	for (UActorComponent* Component : Components)
	{
		// Belts
		if (UFGFactoryConnectionComponent* ConveyorConnection = Cast<UFGFactoryConnectionComponent>(Component))
		{
			if (ConveyorConnection->GetDirection() == EFactoryConnectionDirection::FCD_INPUT)
			{
				mConnectionMap[EKPCLConnectionType::ConvIn].Add(ConveyorConnection);
			}
			else if (ConveyorConnection->GetDirection() == EFactoryConnectionDirection::FCD_OUTPUT)
			{
				mConnectionMap[EKPCLConnectionType::ConvOut].Add(ConveyorConnection);
			}
		}

		// Pipes
		if (UFGPipeConnectionFactory* PipeConnection = Cast<UFGPipeConnectionFactory>(Component))
		{
			if (PipeConnection->GetPipeConnectionType() == EPipeConnectionType::PCT_CONSUMER)
			{
				mConnectionMap[EKPCLConnectionType::PipeIn].Add(PipeConnection);
			}
			else if (PipeConnection->GetPipeConnectionType() == EPipeConnectionType::PCT_PRODUCER)
			{
				mConnectionMap[EKPCLConnectionType::PipeOut].Add(PipeConnection);
			}

			// make sure that this building use AdditionalPressure!!!
			if (!PipeConnection->mApplyAdditionalPressure)
			{
				PipeConnection->mApplyAdditionalPressure = true;
			}
		}
	}
}

float AKPCLExtractorBase::GetMaxPowerConsume() const { return mPowerOptions.GetMaxPowerConsume(); }

float AKPCLExtractorBase::GetPowerConsume() const { return mPowerOptions.GetPowerConsume(); }

bool AKPCLExtractorBase::GetHasPower() const { return mPowerOptions.bHasPower; }

UFGPowerConnectionComponent* AKPCLExtractorBase::GetPowerConnection() const { return mFGPowerConnection; }

float AKPCLExtractorBase::GetCurrentPowerMultiplier() const { return mPowerOptions.mPowerMultiplier; }

void AKPCLExtractorBase::SetPowerMultiplier(float NewMultiplier)
{
	mPowerOptions.mPowerMultiplier = NewMultiplier;
	mPropertyReplicator.MarkPropertyDirty(FName("mPowerOptions"));
}

float AKPCLExtractorBase::GetDefaultProductionCycleTime() const { return mProductionHandle.mProductionTime; }

float AKPCLExtractorBase::GetProductionProgress() const
{
	return mProductionHandle.mProductionTime > 0.f ? mProductionHandle.mCurrentTime / mProductionHandle.mProductionTime
												   : 0.f;
}

void AKPCLExtractorBase::SetPendingPotential(float NewPendingPotential)
{
	NewPendingPotential = FMath::Clamp(NewPendingPotential, GetCurrentMinPotential(), GetCurrentMaxPotential());
	Super::SetPendingPotential(NewPendingPotential);
	mProductionHandle.mPendingPotential = NewPendingPotential;
	mPropertyReplicator.MarkPropertyDirty(FName("mProductionHandle"));
}

float AKPCLExtractorBase::GetCurrentMaxPotential() const
{
	float Result = 1.f;
	if (IsValid(GetPotentialInventory()))
	{
		TArray<FInventoryStack> Stacks;
		GetPotentialInventory()->GetInventoryStacks(Stacks);
		for (const FInventoryStack& Stack : Stacks)
		{
			if (!Stack.HasItems())
			{
				continue;
			}

			if (TSubclassOf<UFGPowerShardDescriptor> PowerShard =
					TSubclassOf<UFGPowerShardDescriptor>(Stack.Item.GetItemClass()))
			{
				EPowerShardType Type = UFGPowerShardDescriptor::GetPowerShardType(PowerShard);
				if (Type == EPowerShardType::PST_Overclock)
				{
					Result += UFGPowerShardDescriptor::GetBoostValue(PowerShard) * Stack.NumItems;
				}
			}
		}
	}
	return Result;
}

void AKPCLExtractorBase::OnBoosterItemAdded(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved,
											UFGInventoryComponent* sourceInventory)
{
}

void AKPCLExtractorBase::OnBoosterItemRemoved(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved,
											  UFGInventoryComponent* sourceInventory)
{
}

float AKPCLExtractorBase::CalcProductionCycleTimeForPotential(float potential) const
{
	return potential != 0.f ? mProductionHandle.mProductionTime / potential : 0.f;
}

float AKPCLExtractorBase::GetProducingPowerConsumptionBase() const { return mPowerOptions.mNormalPowerConsume; }

void AKPCLExtractorBase::Overclocking_GetProductionResults_Implementation(
	TArray<FKPCLOverclockingProductionResults>& OutIngredients, TArray<FKPCLOverclockingProductionResults>& OutProducts)
{
	OutIngredients.Empty();
	OutProducts.Empty();
}

void AKPCLExtractorBase::Factory_CollectInput_Implementation()
{
	Super::Factory_CollectInput_Implementation();

	if (HasAuthority())
	{
		CollectBelts();
	}
}

void AKPCLExtractorBase::Factory_PullPipeInput_Implementation(float dt)
{
	Super::Factory_PullPipeInput_Implementation(dt);

	if (HasAuthority())
	{
		CollectAndPushPipes(dt, false);
	}
}

void AKPCLExtractorBase::Factory_PushPipeOutput_Implementation(float dt)
{
	Super::Factory_PushPipeOutput_Implementation(dt);

	if (HasAuthority())
	{
		CollectAndPushPipes(dt, true);
	}
}

void AKPCLExtractorBase::CollectBelts() {}

void AKPCLExtractorBase::CollectAndPushPipes(float dt, bool IsPush) {}

void AKPCLExtractorBase::ResetProduction()
{
	mProductionHandle.Reset();
	mPropertyReplicator.MarkPropertyDirty(FName("mProductionHandle"));
}

void AKPCLExtractorBase::SetProductionTime(float NewTime, bool ShouldResetProduction)
{
	mProductionHandle.SetNewTime(NewTime, ShouldResetProduction);
	mPropertyReplicator.MarkPropertyDirty(FName("mProductionHandle"));
}

void AKPCLExtractorBase::SetPowerOption(FPowerOptions NewPowerOption)
{
	mPowerOptions.OverWritePowerOptions(NewPowerOption);
	mPropertyReplicator.MarkPropertyDirty(FName("mPowerOptions"));
}

FPowerOptions AKPCLExtractorBase::GetPowerOption() const { return mPowerOptions; }

FPowerOptions& AKPCLExtractorBase::GetPowerOptionRef() { return mPowerOptions; }

void AKPCLExtractorBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AKPCLExtractorBase, mMeshOverwriteInformations);
	DOREPLIFETIME(AKPCLExtractorBase, mCurrentState);
}

void AKPCLExtractorBase::GetConditionalReplicatedProps(TArray<FFGCondReplicatedProperty>& outProps) const
{
	Super::GetConditionalReplicatedProps(outProps);

	FG_DOREPCONDITIONAL(ThisClass, mProductionHandle);
	FG_DOREPCONDITIONAL(ThisClass, mPowerOptions);
}

void AKPCLExtractorBase::OnRep_MeshOverwriteInformations() { ReadyForVisuelUpdate(); }

void AKPCLExtractorBase::OnRep_CurrentState() { ApplyNewProductionState(mCurrentState); }

void AKPCLExtractorBase::SetCurrentProductionState(ENewProductionState NewState)
{
	if (mCurrentState == NewState)
	{
		return;
	}
	mCurrentState = NewState;
}

void AKPCLExtractorBase::onProducingFinal_Implementation() {}

float AKPCLExtractorBase::GetProductionTime() const { return mProductionHandle.GetProductionTime(); }

float AKPCLExtractorBase::GetPendingProductionTime() const { return mProductionHandle.GetPendingProductionTime(); }

FFullProductionHandle AKPCLExtractorBase::GetProductionHandle() const { return mProductionHandle; }

void AKPCLExtractorBase::FlushFluids()
{
	if (HasAuthority())
	{
		Server_DoFlush();
	}
	else
	{
		if (UKPCLDefaultRCO* RCO = UKPCLDefaultRCO::GetRCO<UKPCLDefaultRCO>(GetWorld()))
		{
			RCO->Server_FlushFluids(this);
		}
	}
}

float AKPCLExtractorBase::GetProductionCycleTime() const { return mProductionHandle.GetProductionTime(); }

UFGInventoryComponent* AKPCLExtractorBase::GetInventory() const
{
	if (!mCachedInventorys.Contains(FKPCLInventoryStructure::InputName))
	{
		return nullptr;
	}
	return mCachedInventorys.FindChecked(FKPCLInventoryStructure::InputName);
}

void AKPCLExtractorBase::InitInputInventory()
{
	GetInventory()->OnItemAddedDelegate_Native.BindUObject(this, &AKPCLExtractorBase::OnInputItemAdded);
	GetInventory()->OnItemRemovedDelegate_Native.BindUObject(this, &AKPCLExtractorBase::OnInputItemRemoved);

	if (!GetInventory()->mItemFilter.IsBoundToObject(this))
	{
		GetInventory()->mItemFilter.BindUObject(this, &AKPCLExtractorBase::FilterInputInventory);
	}
	if (!GetInventory()->mFormFilter.IsBoundToObject(this))
	{
		GetInventory()->mFormFilter.BindUObject(this, &AKPCLExtractorBase::FormFilterInputInventory);
	}
}

void AKPCLExtractorBase::InitOutputInventory()
{
	GetOutputInventory()->OnItemAddedDelegate_Native.BindUObject(this, &AKPCLExtractorBase::OnOutputItemAdded);
	GetOutputInventory()->OnItemRemovedDelegate_Native.BindUObject(this, &AKPCLExtractorBase::OnOutputItemRemoved);

	if (!GetOutputInventory()->mItemFilter.IsBoundToObject(this))
	{
		GetOutputInventory()->mItemFilter.BindUObject(this, &AKPCLExtractorBase::FilterOutputInventory);
	}
	if (!GetOutputInventory()->mFormFilter.IsBoundToObject(this))
	{
		GetOutputInventory()->mFormFilter.BindUObject(this, &AKPCLExtractorBase::FormFilterOutputInventory);
	}
}

UFGInventoryComponent* AKPCLExtractorBase::GetBoosterInventory() const
{
	if (!mCachedInventorys.Contains(FKPCLInventoryStructure::BoosterName))
	{
		return nullptr;
	}
	return mCachedInventorys.FindChecked(FKPCLInventoryStructure::BoosterName);
}

void AKPCLExtractorBase::InitBoosterInventory()
{
	GetBoosterInventory()->OnItemAddedDelegate_Native.BindUObject(this, &AKPCLExtractorBase::OnBoosterItemAdded);
	GetBoosterInventory()->OnItemRemovedDelegate_Native.BindUObject(this, &AKPCLExtractorBase::OnBoosterItemRemoved);

	if (!GetBoosterInventory()->mItemFilter.IsBoundToObject(this))
	{
		GetBoosterInventory()->mItemFilter.BindUObject(this, &AKPCLExtractorBase::FilterBoosterInventory);
	}
	if (!GetBoosterInventory()->mFormFilter.IsBoundToObject(this))
	{
		GetBoosterInventory()->mFormFilter.BindUObject(this, &AKPCLExtractorBase::FormFilterBoosterInventory);
	}
}

UFGInventoryComponent* AKPCLExtractorBase::GetInventoryFromType(EKPCLInventoryType Type) const
{
	switch (Type)
	{
	case EKPCLInventoryType::Booster:
		return GetBoosterInventory();
	case EKPCLInventoryType::Output:
		return GetOutputInventory();
	default:
		return GetInventory();
	}
}
