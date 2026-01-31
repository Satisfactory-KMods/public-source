// Copyright Coffee Stain Studios. All Rights Reserved.

#include "KBFLGameInstanceModule.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "KBFLLogging.h"
#include "Subsystems/KBFLAssetDataSubsystem.h"
#include "Subsystems/KBFLContentCDOHelperSubsystem.h"
#include "TimerManager.h"


UKBFLGameInstanceModule::UKBFLGameInstanceModule()
{
	bRootModule = false;

	mCDOInformationMap.Add(ELifecyclePhase::CONSTRUCTION, FKBFLCDOInformation());
	mCDOInformationMap.Add(ELifecyclePhase::INITIALIZATION, FKBFLCDOInformation());
	mCDOInformationMap.Add(ELifecyclePhase::POST_INITIALIZATION, FKBFLCDOInformation());
}

void UKBFLGameInstanceModule::FindAllCDOs()
{
	if (!mUseAssetRegistry || !mRegisterCDOs || bScanForCDOsDone)
	{
		bScanForCDOsDone = true;
		return;
	}
	UKBFLAssetDataSubsystem* AssetDataSub = UKBFLAssetDataSubsystem::Get(GetWorld());
	if (!IsValid(AssetDataSub))
	{
		return;
	}
	fgcheck(AssetDataSub);

	constexpr ELifecyclePhase CdoPhase = ELifecyclePhase::CONSTRUCTION;
	FKBFLAssetData Datas = AssetDataSub->GetModRelatedData(this);
	if (!mCDOInformationMap.Find(CdoPhase))
	{
		mCDOInformationMap.Add(CdoPhase, FKBFLCDOInformation());
	}
	FKBFLCDOInformation* CDOInfo = mCDOInformationMap.Find(CdoPhase);
	fgcheck(CDOInfo);

	for (UClass* CDOHelper : Datas.mAllFoundedCDOHelpers)
	{
		if (TSubclassOf<UKBFL_CDOHelperClass_Base> CDOHelperClass = CDOHelper)
		{
			if (mBlacklistedCDOClasses.Contains(CDOHelperClass))
			{
				continue;
			}
			UE_LOG(KBFLGameInstanceModuleLog, Warning, TEXT("Found CDO helper (%s) and add to map"),
				   *CDOHelperClass->GetName());
			CDOInfo->mCDOHelperClasses.AddUnique(CDOHelperClass);
		}
	}
	bScanForCDOsDone = true;
}

FKBFLCDOInformation UKBFLGameInstanceModule::GetCDOInformationFromPhase_Implementation(ELifecyclePhase Phase,
																					   bool& HasPhase)
{
	HasPhase = mCDOInformationMap.Contains(Phase);
	if (HasPhase)
	{
		return mCDOInformationMap[Phase];
	}
	return FKBFLCDOInformation();
}

void UKBFLGameInstanceModule::DispatchLifecycleEvent(ELifecyclePhase Phase)
{
	if (Phase == ELifecyclePhase::CONSTRUCTION)
	{
		ConstructionPhase();
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UKBFLGameInstanceModule::ConstructionPhase_Delayed);
	}

	if (Phase == ELifecyclePhase::INITIALIZATION)
	{
		InitPhase();
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UKBFLGameInstanceModule::InitPhase_Delayed);
	}

	if (Phase == ELifecyclePhase::POST_INITIALIZATION)
	{
		PostInitPhase();
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UKBFLGameInstanceModule::PostInitPhase_Delayed);
	}

	Super::DispatchLifecycleEvent(Phase);
}

bool UKBFLGameInstanceModule::IsOwnerModObject(UObject* Object) const
{
	TArray<FString> DirectoryArray;
	Object->GetPathName().ParseIntoArray(DirectoryArray, TEXT("/"));
	FName ModName = FName(DirectoryArray[0]);
	return ModName == GetOwnerModReference();
}

void UKBFLGameInstanceModule::ConstructionPhase_Delayed()
{
	FindAllCDOs();
	if (UKBFLContentCDOHelperSubsystem* CDOHelperSubsystem =
			GetWorld()->GetGameInstance()->GetSubsystem<UKBFLContentCDOHelperSubsystem>())
	{
		if (!Called.Contains(ELifecyclePhase::CONSTRUCTION))
		{
			Called.Add(ELifecyclePhase::CONSTRUCTION);
			UE_LOG(KBFLGameInstanceModuleLog, Log, TEXT("Begin CDO call for %s as phase CONSTRUCTION"),
				   *GetOwnerModReference().ToString());
			CDOHelperSubsystem->BeginCDOForModule(this, ELifecyclePhase::CONSTRUCTION);
		}
	}
	else
	{
		UE_LOG(KBFLGameInstanceModuleLog, Log,
			   TEXT("WARNING INVALID CDOHelperSubsystem : %s as phase ELifecyclePhase::CONSTRUCTION"),
			   *GetOwnerModReference().ToString());
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UKBFLGameInstanceModule::ConstructionPhase_Delayed);
	}
}

void UKBFLGameInstanceModule::InitPhase_Delayed()
{
	FindAllCDOs();
	if (UKBFLContentCDOHelperSubsystem* CDOHelperSubsystem =
			GetWorld()->GetGameInstance()->GetSubsystem<UKBFLContentCDOHelperSubsystem>())
	{
		if (!Called.Contains(ELifecyclePhase::INITIALIZATION))
		{
			Called.Add(ELifecyclePhase::INITIALIZATION);
			UE_LOG(KBFLGameInstanceModuleLog, Log, TEXT("Begin CDO call for %s as phase INITIALIZATION"),
				   *GetOwnerModReference().ToString());
			CDOHelperSubsystem->BeginCDOForModule(this, ELifecyclePhase::INITIALIZATION);
		}
	}
	else
	{
		UE_LOG(KBFLGameInstanceModuleLog, Log, TEXT("WARNING INVALID CDOHelperSubsystem : %s as phase INITIALIZATION"),
			   *GetOwnerModReference().ToString());
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UKBFLGameInstanceModule::InitPhase_Delayed);
	}
}

void UKBFLGameInstanceModule::PostInitPhase_Delayed()
{
	FindAllCDOs();
	if (UKBFLContentCDOHelperSubsystem* CDOHelperSubsystem =
			GetWorld()->GetGameInstance()->GetSubsystem<UKBFLContentCDOHelperSubsystem>())
	{
		if (!Called.Contains(ELifecyclePhase::POST_INITIALIZATION))
		{
			Called.Add(ELifecyclePhase::POST_INITIALIZATION);
			UE_LOG(KBFLGameInstanceModuleLog, Log, TEXT("Begin CDO call for %s as phase POST_INITIALIZATION"),
				   *GetOwnerModReference().ToString());
			CDOHelperSubsystem->BeginCDOForModule(this, ELifecyclePhase::POST_INITIALIZATION);
		}
	}
	else
	{
		UE_LOG(KBFLGameInstanceModuleLog, Log,
			   TEXT("WARNING INVALID CDOHelperSubsystem : %s as phase POST_INITIALIZATION"),
			   *GetOwnerModReference().ToString());
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UKBFLGameInstanceModule::PostInitPhase_Delayed);
	}
}

void UKBFLGameInstanceModule::PostInitPhase_Implementation() {}

void UKBFLGameInstanceModule::InitPhase_Implementation() {}

void UKBFLGameInstanceModule::ConstructionPhase_Implementation()
{
	if (!mUseAssetRegistry || !mRegisterAGS)
	{
		return;
	}

	TSet<USMLSessionSetting*> AllSessionSettings;
	UKBFLAssetDataSubsystem::FindAllDataAssetsOfClass(AllSessionSettings);
	SessionSettings.Empty();

	for (USMLSessionSetting* AGS : AllSessionSettings)
	{
		if (!IsOwnerModObject(AGS))
		{
			continue;
		}
		SessionSettings.Add(AGS);
	}

	UE_LOG(KBFLGameInstanceModuleLog, Log,
		   TEXT("UKBFLGameInstanceModule::ConstructionPhase : %s added %d AGS to SessionSettings"),
		   *GetOwnerModReference().ToString(), SessionSettings.Num());
}
