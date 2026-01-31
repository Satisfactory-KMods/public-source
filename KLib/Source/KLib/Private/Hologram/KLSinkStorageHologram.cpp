// Copyright Coffee Stain Studios. All Rights Reserved.


#include "Hologram/KLSinkStorageHologram.h"

#include "Buildable/Storage/KLSinkStorage.h"
#include "Buildables/FGBuildableStorage.h"

void AKLSinkStorageHologram::GetSupportedBuildModes_Implementation(
	TArray<TSubclassOf<UFGBuildGunModeDescriptor>>& out_buildmodes) const
{
	Super::GetSupportedBuildModes_Implementation(out_buildmodes);
	if (mUpgradeBuildMode)
	{
		out_buildmodes.AddUnique(mUpgradeBuildMode);
	}
}

void AKLSinkStorageHologram::OnBuildModeChanged(TSubclassOf<UFGHologramBuildModeDescriptor> buildMode)
{
	Super::OnBuildModeChanged(buildMode);

	if (!IsCurrentBuildMode(mUpgradeBuildMode))
	{
		mUpgradeActor = nullptr;
	}
}

AActor* AKLSinkStorageHologram::GetUpgradedActor() const
{
	return mUpgradeActor;
}

bool AKLSinkStorageHologram::TryUpgrade(const FHitResult& hitResult)
{
	if (!IsCurrentBuildMode(mUpgradeBuildMode))
	{
		mUpgradeActor = nullptr;
		return Super::TryUpgrade(hitResult);
	}

	if (hitResult.GetActor() != mUpgradeActor)
	{
		mUpgradeActor = nullptr;
	}

	if (AFGBuildableStorage* AsStorage = Cast<AFGBuildableStorage>(hitResult.GetActor()))
	{
		AFGBuildableStorage* Default = GetDefaultBuildable<AFGBuildableStorage>();

		if (AsStorage->GetClass() != mBuildClass && Default->mInventorySizeX == AsStorage->mInventorySizeX && Default->
			mInventorySizeY == AsStorage->mInventorySizeY)
		{
			mUpgradeActor = hitResult.GetActor();
			SetActorLocationAndRotation(hitResult.GetActor()->GetActorLocation(),
			                            hitResult.GetActor()->GetActorRotation());
			return mUpgradeActor != nullptr;
		}
	}
	return Super::TryUpgrade(hitResult);
}

void AKLSinkStorageHologram::ConfigureActor(AFGBuildable* inBuildable) const
{
	Super::ConfigureActor(inBuildable);

	if (GetUpgradedActor() && HasAuthority())
	{
		AFGBuildableStorage* AsStorage = Cast<AFGBuildableStorage>(GetUpgradedActor());
		AFGBuildableStorage* NewAsStorage = Cast<AFGBuildableStorage>(inBuildable);

		if (IsValid(AsStorage) && IsValid(NewAsStorage))
		{
			if (AsStorage->GetStorageInventory() && NewAsStorage->GetStorageInventory())
			{
				NewAsStorage->GetStorageInventory()->CopyFromOtherComponent(AsStorage->GetStorageInventory());
				NewAsStorage->GetStorageInventory()->Resize(
					NewAsStorage->mInventorySizeX * NewAsStorage->mInventorySizeY);
				AsStorage->GetStorageInventory()->Empty();
			}
		}
	}
}
