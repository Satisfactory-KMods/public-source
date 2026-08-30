#include "Network/KPCLNetworkWidgetPanel.h"

#include "Buildables/FGBuildable.h"
#include "Interfaces/KPCLNetworkDataInterface.h"
#include "KPrivateCodeLibModule.h"
#include "Network/Buildings/KPCLNetworkCore.h"
#include "Network/KPCLNetworkBuildingBase.h"

void UKPCLNetworkWidgetPanel::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(mInteractObject))
	{
		InitializeNetworkPanel(mInteractObject);
	}
}

void UKPCLNetworkWidgetPanel::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!IsValid(mInteractObject))
	{
		return;
	}

	if (bNetworkPanelInitialized && IsValid(mNetworkBuilding) &&
		mNetworkBuilding->GetNetworkId() != mInitializedNetworkId)
	{
		bNetworkPanelInitialized = false;
	}

	if (!bNetworkPanelInitialized || !IsValid(mFaxitSubsystem))
	{
		InitializeNetworkPanel(mInteractObject);
	}

	if (bNetworkPanelInitialized)
	{
		WidgetTick(MyGeometry, InDeltaTime);
	}
}

void UKPCLNetworkWidgetPanel::InitializeNetworkPanel(UObject* InteractObject)
{
	if (mInteractObject != InteractObject)
	{
		bNetworkPanelInitialized = false;
		mFaxitSubsystem = nullptr;
		mNetworkCore = nullptr;
		mNetworkBuilding = nullptr;
		mUiData = FNetworkUIData();
		mInitializedNetworkId = FString();
		bNetworkPanelSetupCompleted = false;
	}

	mInteractObject = InteractObject;

	if (!IsValid(InteractObject) ||
		!InteractObject->GetClass()->ImplementsInterface(UKPCLNetworkDataInterface::StaticClass()))
	{
		return;
	}

	if (bNetworkPanelInitialized && IsValid(mFaxitSubsystem))
	{
		return;
	}

	mNetworkBuilding = Cast<AKPCLNetworkBuildingBase>(InteractObject);
	mNetworkCore = IKPCLNetworkDataInterface::Execute_GetCore(mInteractObject);

	mFaxitSubsystem = AKPCLFaxitSubsystem::Get(GetWorld());
	if (!IsValid(mFaxitSubsystem) || !mFaxitSubsystem->HasReceivedInitialNetworkData())
	{
		return;
	}

	mNetworkCore = IKPCLNetworkDataInterface::Execute_GetCore(mInteractObject);

	if (IsValid(mNetworkBuilding))
	{
		mUiData = IKPCLNetworkDataInterface::Execute_GetUIDData(mNetworkBuilding);

		const FString CurrentNetworkId = mNetworkBuilding->GetNetworkId();
		if (!CurrentNetworkId.IsEmpty())
		{
			FKPCLFaxitNetwork CurrentNetwork;
			if (!mFaxitSubsystem->GetNetwork(mNetworkBuilding, CurrentNetwork) || !IsValid(CurrentNetwork.mCore))
			{
				return;
			}
		}

		mInitializedNetworkId = CurrentNetworkId;
	}

	bNetworkPanelInitialized = true;
	if (GetWorld()->GetNetMode() == NM_Client)
	{
		UE_LOG(LogFaxit, Log, TEXT("Client network panel ready for %s: NetworkId=%s, Networks=%d, Core=%s"),
			   *GetNameSafe(mNetworkBuilding), *mInitializedNetworkId, mFaxitSubsystem->GetNetworkCount(),
			   *GetNameSafe(mNetworkCore));
	}
	if (!bNetworkPanelSetupCompleted)
	{
		bNetworkPanelSetupCompleted = true;
		AfterInitializeNetworkPanel();
	}
}

void UKPCLNetworkWidgetPanel::AddBuildingToCore(FKPCLFaxitNetwork Network)
{
	AKPCLNetworkBuildingBase* NetworkBuilding = Cast<AKPCLNetworkBuildingBase>(mNetworkBuilding);
	if (!Network.mIsValid || !IsValid(NetworkBuilding))
	{
		return;
	}

	if (!IsValid(mFaxitSubsystem))
	{
		return;
	}

	mFaxitSubsystem->AddBuildingToCore(NetworkBuilding, Network.mCore);
}

bool UKPCLNetworkWidgetPanel::HasNetwork()
{
	bool bSuccess = false;
	GetCurrentNetwork(bSuccess);
	return bSuccess;
}

FKPCLFaxitNetworkInfo UKPCLNetworkWidgetPanel::GetNetworkInfo(bool& bSuccess, const FString& NetworkId)
{
	bSuccess = false;
	if (!IsValid(mFaxitSubsystem))
	{
		return FKPCLFaxitNetworkInfo();
	}

	const FString ResolvedId =
		NetworkId.IsEmpty() ? (IsValid(mNetworkBuilding) ? mNetworkBuilding->GetNetworkId() : FString()) : NetworkId;

	if (ResolvedId.IsEmpty())
	{
		return FKPCLFaxitNetworkInfo();
	}

	return mFaxitSubsystem->GetNetworkByIdWithInfo(ResolvedId, bSuccess);
}

void UKPCLNetworkWidgetPanel::WidgetTick_Implementation(const FGeometry& MyGeometry, float InDeltaTime) {}

AKPCLNetworkCore* UKPCLNetworkWidgetPanel::GetNetworkCore()
{
	bool bSuccess = false;
	FKPCLFaxitNetwork Network = GetCurrentNetwork(bSuccess);

	if (!IsValid(Network.mCore) && IsValid(mNetworkBuilding))
	{
		return IKPCLNetworkDataInterface::Execute_GetCore(mNetworkBuilding);
	}

	return Network.mCore;
}

FKPCLFaxitNetwork UKPCLNetworkWidgetPanel::GetCurrentNetwork(bool& bSuccess)
{
	bSuccess = false;
	if (!IsValid(mNetworkBuilding) || !IsValid(mFaxitSubsystem))
	{
		return FKPCLFaxitNetwork();
	}

	FKPCLFaxitNetwork Network;
	bSuccess = mFaxitSubsystem->GetNetwork(mNetworkBuilding, Network);
	return Network;
}
