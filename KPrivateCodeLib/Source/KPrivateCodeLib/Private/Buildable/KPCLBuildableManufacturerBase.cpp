// Copyright Coffee Stain Studios. All Rights Reserved.


#include "Buildable/KPCLBuildableManufacturerBase.h"

#include "AbstractInstanceManager.h"
#include "Buildable/KPCLProducerBase.h"
#include "Net/UnrealNetwork.h"


// Sets default values
AKPCLBuildableManufacturerBase::AKPCLBuildableManufacturerBase()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

void AKPCLBuildableManufacturerBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Production
	//DOREPLIFETIME( AKPCLBuildableManufacturerBase, mProductionHandle );
	//DOREPLIFETIME( AKPCLBuildableManufacturerBase, mPowerOptions );
	//DOREPLIFETIME( AKPCLBuildableManufacturerBase, mCurrentState );
	DOREPLIFETIME(ThisClass, mMeshOverwriteInformations);
	//DOREPLIFETIME( AKPCLBuildableManufacturerBase, mInventoryDatasSaved );
}

bool AKPCLBuildableManufacturerBase::ShouldSave_Implementation() const
{
	return Super::ShouldSave_Implementation();
}

void AKPCLBuildableManufacturerBase::BeginPlay()
{
	Super::BeginPlay();

	if (!bDeferBeginPlay)
	{
		ReadyForVisuelUpdate();
	}

	InitAudioConfig();
}

void AKPCLBuildableManufacturerBase::InitAudioConfig()
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
			Property->OnPropertyValueChanged.AddUniqueDynamic(
				this, &AKPCLBuildableManufacturerBase::OnAudioConfigChanged_Native);
			OnAudioConfigChanged_Native();
		}
	}
}

void AKPCLBuildableManufacturerBase::OnAudioConfigChanged_Native()
{
	for (FKPCLAudioComponent AudioComponent : mAudioComponents)
	{
		AudioComponent.SetVolumePercent(mAudioConfig.GetValue(GetWorld()));
	}
	OnAudioConfigChanged();
}

void AKPCLBuildableManufacturerBase::OnBuildEffectFinished()
{
	Super::OnBuildEffectFinished();
	ReadyForVisuelUpdate();
}

void AKPCLBuildableManufacturerBase::OnBuildEffectActorFinished()
{
	Super::OnBuildEffectActorFinished();
	ReadyForVisuelUpdate();
}

void AKPCLBuildableManufacturerBase::ReadyForVisuelUpdate()
{
}

void AKPCLBuildableManufacturerBase::InitMeshOverwriteInformation()
{
	if (!DoesContainLightweightInstances_Native())
	{
		return;
	}

	if (mInstanceHandles.Num() <= 0)
	{
		GetWorldTimerManager().SetTimerForNextTick(this, &AKPCLBuildableManufacturerBase::InitMeshOverwriteInformation);
		return;
	}

	if (!mInstanceHandles[0]->IsInstanced())
	{
		GetWorldTimerManager().SetTimerForNextTick(this, &AKPCLBuildableManufacturerBase::InitMeshOverwriteInformation);
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
}

void AKPCLBuildableManufacturerBase::ReApplyColorForIndex(int32 Idx, const FFactoryCustomizationData& customizationData)
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

void AKPCLBuildableManufacturerBase::ApplyCustomizationData_Native(const FFactoryCustomizationData& customizationData)
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

void AKPCLBuildableManufacturerBase::SetCustomizationData_Native(const FFactoryCustomizationData& customizationData,
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

void AKPCLBuildableManufacturerBase::ApplyMeshOverwriteInformation(int32 Idx)
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

void AKPCLBuildableManufacturerBase::ApplyMeshInformation(FKPCLMeshOverwriteInformation Information)
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

bool AKPCLBuildableManufacturerBase::ShouldOverwriteIndexHandle(int32 Idx, FKPCLMeshOverwriteInformation& Information)
{
	return false;
}

bool AKPCLBuildableManufacturerBase::AIO_OverwriteInstanceData(UStaticMesh* Mesh, int32 Idx)
{
	if (!IsInGameThread())
	{
		FFunctionGraphTask::CreateAndDispatchWhenReady([ &, Mesh, Idx ]()
		{
			AIO_OverwriteInstanceData(Mesh, Idx);
		}, GET_STATID(STAT_TaskGraph_OtherTasks), nullptr, ENamedThreads::GameThread);
		return true;
	}

	if (Idx > INDEX_NONE && IsValid(Mesh))
	{
		TArray<FInstanceData> Datas = mInstanceDataCDO->GetInstanceData();
		if (Datas.IsValidIndex(Idx))
		{
			return AIO_OverwriteInstanceData_Transform(Mesh, Datas[Idx].RelativeTransform, Idx);
		}
	}
	return false;
}

bool AKPCLBuildableManufacturerBase::AIO_OverwriteInstanceData_Transform(
	UStaticMesh* Mesh, FTransform NewRelativTransform, int32 Idx)
{
	if (!IsInGameThread())
	{
		FFunctionGraphTask::CreateAndDispatchWhenReady([ &, Mesh, NewRelativTransform, Idx ]()
		{
			AIO_OverwriteInstanceData_Transform(Mesh, NewRelativTransform, Idx);
		}, GET_STATID(STAT_TaskGraph_OtherTasks), nullptr, ENamedThreads::GameThread);
		return true;
	}

	if (Idx > INDEX_NONE && IsValid(Mesh))
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
			FInstanceOwnerHandlePtr NewInstanceHandle;
			mInstanceHandles[Idx] = NewInstanceHandle;
			Manager->SetInstanced(this, GetActorTransform(), Data, mInstanceHandles[Idx]);
			mCachedTransforms.Add(Idx, NewRelativTransform * GetActorTransform());

			ApplyCustomizationData_Native(mCustomizationData);
			SetCustomizationData_Native(mCustomizationData, false);

			return true;
		}
	}

	return false;
}

bool AKPCLBuildableManufacturerBase::AIO_UpdateCustomFloat(int32 FloatIndex, float Data, int32 InstanceIdx,
                                                           bool MarkDirty)
{
	if (!IsInGameThread())
	{
		FFunctionGraphTask::CreateAndDispatchWhenReady([ &, FloatIndex, Data, InstanceIdx, MarkDirty ]()
		{
			AIO_UpdateCustomFloat(FloatIndex, Data, InstanceIdx, MarkDirty);
		}, GET_STATID(STAT_TaskGraph_OtherTasks), nullptr, ENamedThreads::GameThread);
		return true;
	}

	if (mInstanceHandles.IsValidIndex(InstanceIdx))
	{
		if (mInstanceHandles[InstanceIdx]->IsInstanced())
		{
			//UE_LOG( LogKPCL, Error, TEXT("mInstanceHandles[ %d ]->SetPrimitiveDataByID( %f, %d, %d ); IsValid(%d), Component(%d), Owner(%d)"), InstanceIdx, Data, FloatIndex, MarkDirty, mInstanceHandles[ InstanceIdx ]->IsValid(), IsValid( mInstanceHandles[ InstanceIdx ]->GetInstanceComponent() ), mInstanceHandles[ InstanceIdx ]->GetOwner() == this )
			mInstanceHandles[InstanceIdx]->SetPrimitiveDataByID(Data/** float that we want to set */,
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

bool AKPCLBuildableManufacturerBase::AIO_UpdateCustomFloatAsColor(int32 StartFloatIndex, FLinearColor Data,
                                                                  int32 InstanceIdx, bool MarkDirty)
{
	if (!IsInGameThread())
	{
		FFunctionGraphTask::CreateAndDispatchWhenReady([ &, StartFloatIndex, Data, InstanceIdx, MarkDirty ]()
		{
			AIO_UpdateCustomFloatAsColor(StartFloatIndex, Data, InstanceIdx, MarkDirty);
		}, GET_STATID(STAT_TaskGraph_OtherTasks), nullptr, ENamedThreads::GameThread);
		return true;
	}

	AIO_UpdateCustomFloat(StartFloatIndex, Data.R, InstanceIdx, MarkDirty);
	AIO_UpdateCustomFloat(StartFloatIndex + 1, Data.G, InstanceIdx, MarkDirty);
	return AIO_UpdateCustomFloat(StartFloatIndex + 2, Data.B, InstanceIdx, MarkDirty);
}

bool AKPCLBuildableManufacturerBase::AIO_SetInstanceHidden(int32 InstanceIdx, bool IsHidden)
{
	if (!IsInGameThread())
	{
		FFunctionGraphTask::CreateAndDispatchWhenReady([ &, InstanceIdx, IsHidden ]()
		{
			AIO_SetInstanceHidden(InstanceIdx, IsHidden);
		}, GET_STATID(STAT_TaskGraph_OtherTasks), nullptr, ENamedThreads::GameThread);
		return true;
	}

	if (mInstanceHandles.IsValidIndex(InstanceIdx))
	{
		if (mInstanceHandles[InstanceIdx]->IsInstanced())
		{
			FTransform T = mCachedTransforms.Contains(InstanceIdx)
				               ? mCachedTransforms[InstanceIdx]
				               : mInstanceDataCDO->GetInstanceData()[InstanceIdx].RelativeTransform *
				               GetActorTransform();
			T.SetScale3D(!IsHidden ? T.GetScale3D() : FVector(0.001f));
			T.SetLocation(!IsHidden ? T.GetLocation() : FVector(0.001f));

			mInstanceHandles[InstanceIdx]->SetLocalTransform(T);
			return true;
		}
	}

	return false;
}

bool AKPCLBuildableManufacturerBase::AIO_SetInstanceWorldTransform(int32 InstanceIdx, FTransform Transform)
{
	if (!IsInGameThread())
	{
		FFunctionGraphTask::CreateAndDispatchWhenReady([ &, InstanceIdx, Transform ]()
		{
			AIO_SetInstanceWorldTransform(InstanceIdx, Transform);
		}, GET_STATID(STAT_TaskGraph_OtherTasks), nullptr, ENamedThreads::GameThread);
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
