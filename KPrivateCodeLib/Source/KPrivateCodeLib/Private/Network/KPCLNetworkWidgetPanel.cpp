//

#include "Network/KPCLNetworkWidgetPanel.h"

#include "Buildables/FGBuildable.h"
#include "Interfaces/KPCLNetworkDataInterface.h"
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

	WidgetTick(MyGeometry, InDeltaTime);
}

void UKPCLNetworkWidgetPanel::InitializeNetworkPanel(UObject* InteractObject)
{
	mInteractObject = InteractObject;

	fgcheckf(IsValid(InteractObject),
			 TEXT("UKPCLNetworkWidgetPanel::InitializeNetworkPanel - InteractObject is null! %s"), *GetName());

	mNetworkBuilding = Cast<AKPCLNetworkBuildingBase>(InteractObject);

	// GetCore uses the interface which tolerates null targets; guard mNetworkBuilding separately
	// before passing it to Execute_GetUIDData below.
	mNetworkCore = IKPCLNetworkDataInterface::Execute_GetCore(mInteractObject);

	mFaxitSubsystem = AKPCLFaxitSubsystem::Get(GetWorld());
	fgcheck(mFaxitSubsystem)

		// Fixed: if the Cast above failed, mNetworkBuilding is null.
		// Execute_GetUIDData with a null target is undefined; guard before calling.
		if (IsValid(mNetworkBuilding))
	{
		mUiData = IKPCLNetworkDataInterface::Execute_GetUIDData(mNetworkBuilding);
	}

	AfterInitializeNetworkPanel();
}

void UKPCLNetworkWidgetPanel::AddBuildingToCore(FKPCLFaxitNetwork Network)
{
	AKPCLNetworkBuildingBase* NetworkBuilding = Cast<AKPCLNetworkBuildingBase>(mNetworkBuilding);
	if (!Network.mIsValid || !IsValid(NetworkBuilding))
	{
		return;
	}

	// Fixed: previously called without null-checking mFaxitSubsystem (would crash if
	// InitializeNetworkPanel was never run or the subsystem fetch failed).
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

	// Fixed: dereferenced mNetworkBuilding without null check when NetworkId is empty.
	// Guard: fall back to empty string (no network) if mNetworkBuilding is null.
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
	if (!IsValid(mNetworkBuilding))
	{
		bSuccess = false;
		return FKPCLFaxitNetwork();
	}

	FKPCLFaxitNetwork Network = IKPCLNetworkDataInterface::Execute_GetNetworkData(mNetworkBuilding);
	bSuccess = Network.mIsValid;
	return Network;
}
