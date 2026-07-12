#include "Components/KPCLColoredStaticMesh.h"

#include "FGBuildableSubsystem.h"

UKPCLColoredStaticMesh::UKPCLColoredStaticMesh() { mNumCustomDataFloats = 20 + mCustomExtraData.Num(); }

bool UKPCLColoredStaticMesh::ShouldSave_Implementation() const { return mShouldSave; }

void UKPCLColoredStaticMesh::BeginPlay()
{
	mLastWorldTransform = GetComponentTransform();
	mNumCustomDataFloats = 20 + mCustomExtraData.Num();

	Super::BeginPlay();

	if (mStartWithDirtyState)
	{
		// sometime can happen that we have the instance in the next frame.
		if (!mInstanceHandle.IsInstanced() && !mBlockInstancing)
		{
			GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UKPCLColoredStaticMesh::ApplyNewData);
			return;
		}

		ApplyNewData();
	}
}

void UKPCLColoredStaticMesh::ApplyNewData()
{
	if (!mBlockInstancing)
	{
		if (mInstanceHandle.IsInstanced())
		{
			UFGColoredInstanceManager* Manager = AFGBuildableSubsystem::Get(this)->GetColoredInstanceManager(this);
			if (Manager)
			{
				mInstanceHandle.CustomData.SetNum(mNumCustomDataFloats + mCustomExtraData.Num());

				for (TTuple<int32, float> OverwriteMap : mFGOverwriteMap)
				{
					mInstanceHandle.SetCustomDataById(OverwriteMap.Key, OverwriteMap.Value);
				}

				for (int i = 0; i < mCustomExtraData.Num(); ++i)
				{
					mInstanceHandle.SetCustomDataById(20 + i, mCustomExtraData[i]);
				}

				Manager->SetUserDefinedData(mCustomExtraData, mInstanceHandle);
				Manager->UpdateColorForInstanceFromDataArray(mInstanceHandle);
			}
		}
	}
	else if (mBlockInstancing)
	{
		for (TTuple<int32, float> OverwriteMap : mFGOverwriteMap)
		{
			SetDefaultCustomPrimitiveDataFloat(OverwriteMap.Key, OverwriteMap.Value);
			SetCustomPrimitiveDataFloat(OverwriteMap.Key, OverwriteMap.Value);
		}
		// Set Primitive Data for not Instanced Indicators (Should not used because > Performance)
		for (int i = 0; i < mCustomExtraData.Num(); ++i)
		{
			SetDefaultCustomPrimitiveDataFloat(20 + i, mCustomExtraData[i]);
			SetCustomPrimitiveDataFloat(20 + i, mCustomExtraData[i]);
		}
	}
}

bool UKPCLColoredStaticMesh::CheckIndex(FKPCLColorData ColorData, bool IsFG)
{
	int32 FirstIndex = IsFG ? 0 : 20;
	if ((FirstIndex + ColorData.mColorIndex) < mNumCustomDataFloats &&
		mCustomExtraData.IsValidIndex(ColorData.mColorIndex))
	{
		if (IsFG)
		{
			if (mFGOverwriteMap.Contains(ColorData.mColorIndex))
			{
				return mFGOverwriteMap[ColorData.mColorIndex] != ColorData.mIndexData;
			}
			return true;
		}
		return mCustomExtraData[ColorData.mColorIndex] != ColorData.mIndexData;
	}
	if (FirstIndex + ColorData.mColorIndex < mNumCustomDataFloats)
		UE_LOG(LogTemp, Error, TEXT("Try set an invalid index %d for owner %s"), FirstIndex + ColorData.mColorIndex,
			   *GetOwner()->GetClass()->GetName())
	return false;
}

void UKPCLColoredStaticMesh::UpdateWorldTransform(FTransform Transform)
{
	if (IsInGameThread())
	{
		if (this)
		{
			if (!mBlockInstancing && IsValid(this))
			{
				if (mInstanceHandle.IsInstanced() && AFGBuildableSubsystem::Get(this))
				{
					if (UFGColoredInstanceManager* Manager =
							AFGBuildableSubsystem::Get(this)->GetColoredInstanceManager(this))
					{
						Manager->UpdateTransformForInstance(Transform, mInstanceHandle.GetHandleID());
						Manager->UpdateColorForInstanceFromDataArray(mInstanceHandle);
						mLastWorldTransform = Transform;
						ApplyNewData();
					}
				}
			}
		}
	}
	else
	{
		AsyncTask(ENamedThreads::GameThread,
				  [&, Transform]()
				  {
					  if (this)
					  {
						  if (!mBlockInstancing && IsValid(this))
						  {
							  if (mInstanceHandle.IsInstanced() && AFGBuildableSubsystem::Get(this))
							  {
								  if (UFGColoredInstanceManager* Manager =
										  AFGBuildableSubsystem::Get(this)->GetColoredInstanceManager(this))
								  {
									  Manager->UpdateTransformForInstance(Transform, mInstanceHandle.GetHandleID());
									  Manager->UpdateColorForInstanceFromDataArray(mInstanceHandle);
									  mLastWorldTransform = Transform;
									  ApplyNewData();
								  }
							  }
						  }
					  }
				  });
	}
}

void UKPCLColoredStaticMesh::ApplyTransformToComponent()
{
	if (IsInGameThread())
	{
		SetWorldTransform(mLastWorldTransform);
	}
	else
	{
		TWeakObjectPtr<UKPCLColoredStaticMesh> WeakThis(this);
		FTransform CapturedTransform = mLastWorldTransform;
		AsyncTask(ENamedThreads::GameThread,
				  [WeakThis, CapturedTransform]()
				  {
					  if (WeakThis.IsValid())
					  {
						  WeakThis->SetWorldTransform(CapturedTransform);
					  }
				  });
	}
}

void UKPCLColoredStaticMesh::UpdateStaticMesh(UKPCLColoredStaticMesh* Proxy, AActor* Owner, UStaticMesh* Mesh)
{
	if (Proxy && Owner && Mesh)
	{
		if (IsInGameThread())
		{
			if (Mesh != Proxy->GetStaticMesh())
			{
				UKPCLColoredStaticMesh* NewProxy =
					NewObject<UKPCLColoredStaticMesh>(Owner, NAME_None, RF_NoFlags, Proxy);
				NewProxy->SetRelativeLocation(Proxy->GetRelativeLocation());
				NewProxy->mShouldSave = Proxy->mShouldSave;
				NewProxy->mStartWithDirtyState = true;
				NewProxy->mCustomExtraData = Proxy->mCustomExtraData;
				NewProxy->SetStaticMesh(Mesh);
				NewProxy->SetupAttachment(Owner->GetRootComponent());
				Proxy->DestroyComponent();
				NewProxy->RegisterComponent();
				NewProxy->ApplyNewData();
			}
		}
		else
		{
			AsyncTask(ENamedThreads::GameThread,
					  [Proxy, Owner, Mesh]()
					  {
						  UKPCLColoredStaticMesh* NewProxy =
							  NewObject<UKPCLColoredStaticMesh>(Owner, NAME_None, RF_NoFlags, Proxy);
						  NewProxy->SetRelativeLocation(Proxy->GetRelativeLocation());
						  NewProxy->mShouldSave = Proxy->mShouldSave;
						  NewProxy->mStartWithDirtyState = true;
						  NewProxy->mCustomExtraData = Proxy->mCustomExtraData;
						  NewProxy->SetStaticMesh(Mesh);
						  NewProxy->SetupAttachment(Owner->GetRootComponent());
						  Proxy->DestroyComponent();
						  NewProxy->RegisterComponent();
						  NewProxy->ApplyNewData();
					  });
		}
	}
}

void UKPCLColoredStaticMesh::ApplyNewColorDatas(TArray<FKPCLColorData> ColorData, bool MarkStateDirty)
{
	// Now we look should we instance this component so if yes is Innstanced? if not we skip.
	if (!mBlockInstancing && !mInstanceHandle.IsInstanced())
	{
		return;
	}

	if (ColorData.Num() > 0)
	{
		bool NeedDirty = false;
		for (const auto Data : ColorData)
		{
			if (CheckIndex(Data))
			{
				NeedDirty = true;
				mCustomExtraData[Data.mColorIndex] = Data.mIndexData;
			}
		}

		if (MarkStateDirty && NeedDirty)
		{
			if (IsInGameThread())
			{
				if (IsValid(this))
				{
					ApplyNewData();
				}
			}
			else
			{
				TWeakObjectPtr<UKPCLColoredStaticMesh> WeakThis(this);
				AsyncTask(ENamedThreads::GameThread,
						  [WeakThis]()
						  {
							  if (WeakThis.IsValid())
							  {
								  WeakThis->ApplyNewData();
							  }
						  });
			}
		}
	}
}

void UKPCLColoredStaticMesh::ApplyNewColorData(FKPCLColorData ColorData, bool MarkStateDirty)
{
	ApplyNewColorDatas({ColorData}, MarkStateDirty);
}

void UKPCLColoredStaticMesh::ApplyNewLinearColorDatas(TArray<FKPCLLinearColorData> ColorData, bool MarkStateDirty)
{
	if (ColorData.Num() > 0)
	{
		TArray<FKPCLColorData> ColorDatas;

		// Translate FKPCLLinearColorData in FKPCLColorData
		for (FKPCLLinearColorData Data : ColorData)
		{
			ColorDatas.Add(FKPCLColorData(Data.mStartColorIndex, Data.mColor.R));
			ColorDatas.Add(FKPCLColorData(Data.mStartColorIndex + 1, Data.mColor.G));
			ColorDatas.Add(FKPCLColorData(Data.mStartColorIndex + 2, Data.mColor.B));
		}
		ApplyNewColorDatas(ColorDatas, MarkStateDirty);
	}
}

void UKPCLColoredStaticMesh::ApplyNewLinearColorData(FKPCLLinearColorData ColorData, bool MarkStateDirty)
{
	ApplyNewLinearColorDatas({ColorData}, MarkStateDirty);
}

void UKPCLColoredStaticMesh::ApplyNewFGLinearColorDatas(TArray<FKPCLLinearColorData> ColorData, bool MarkStateDirty)
{
	if (ColorData.Num() > 0)
	{
		TArray<FKPCLColorData> ColorDatas;

		// Translate FKPCLLinearColorData in FKPCLColorData
		for (FKPCLLinearColorData Data : ColorData)
		{
			ColorDatas.Add(FKPCLColorData(Data.mStartColorIndex, Data.mColor.R));
			ColorDatas.Add(FKPCLColorData(Data.mStartColorIndex + 1, Data.mColor.G));
			ColorDatas.Add(FKPCLColorData(Data.mStartColorIndex + 2, Data.mColor.B));
		}
		ApplyFGNewColorDatas(ColorDatas, MarkStateDirty);
	}
}

void UKPCLColoredStaticMesh::ApplyNewFGLinearColorData(FKPCLLinearColorData ColorData, bool MarkStateDirty)
{
	ApplyNewFGLinearColorDatas({ColorData}, MarkStateDirty);
}

void UKPCLColoredStaticMesh::ApplyFGNewColorDatas(TArray<FKPCLColorData> ColorData, bool MarkStateDirty)
{
	// Now we look should we instance this component so if yes is Innstanced? if not we skip.
	if (!mBlockInstancing && !mInstanceHandle.IsInstanced())
	{
		return;
	}

	if (ColorData.Num() > 0)
	{
		bool NeedDirty = false;
		for (const auto Data : ColorData)
		{
			if (CheckIndex(Data, true))
			{
				NeedDirty = true;
				mFGOverwriteMap.Add(Data.mColorIndex, Data.mIndexData);
			}
		}

		if (MarkStateDirty && NeedDirty)
		{
			if (IsInGameThread())
			{
				if (IsValid(this))
				{
					ApplyNewData();
				}
			}
			else
			{
				TWeakObjectPtr<UKPCLColoredStaticMesh> WeakThis(this);
				AsyncTask(ENamedThreads::GameThread,
						  [WeakThis]()
						  {
							  if (WeakThis.IsValid())
							  {
								  WeakThis->ApplyNewData();
							  }
						  });
			}
		}
	}
}

void UKPCLColoredStaticMesh::ApplyFgNewColorData(FKPCLColorData ColorData, bool MarkStateDirty)
{
	ApplyFGNewColorDatas({ColorData}, MarkStateDirty);
}

void UKPCLColoredStaticMesh::ApplyFgNewColorToType(FLinearColor Color, EKPCLDefaultColorIndex Type, bool MarkStateDirty)
{
	UE_LOG(LogTemp, Error, TEXT("ApplyFgNewColorToType: Try set an invalid index %d for owner %s"),
		   static_cast<uint8>(Type), *GetOwner()->GetClass()->GetName())
	ApplyNewFGLinearColorData({FKPCLLinearColorData(static_cast<uint8>(Type), Color)}, MarkStateDirty);
}

void UKPCLColoredStaticMesh::RemoveFGIndex(int32 Idx, bool MarkStateDirty)
{
	if (mFGOverwriteMap.Contains(Idx))
	{
		mFGOverwriteMap.Remove(Idx);
		if (MarkStateDirty)
		{
			if (IsInGameThread())
			{
				if (IsValid(this))
				{
					ApplyNewData();
				}
			}
			else
			{
				TWeakObjectPtr<UKPCLColoredStaticMesh> WeakThis(this);
				AsyncTask(ENamedThreads::GameThread,
						  [WeakThis]()
						  {
							  if (WeakThis.IsValid())
							  {
								  WeakThis->ApplyNewData();
							  }
						  });
			}
		}
	}
}

void UKPCLColoredStaticMesh::GetAllFGOverwriteData(TArray<FKPCLColorData>& Datas)
{
	Datas.Empty();
	for (TTuple<int, float> OverwriteMap : mFGOverwriteMap)
	{
		Datas.Add(FKPCLColorData(OverwriteMap.Key, OverwriteMap.Value));
	}
}
