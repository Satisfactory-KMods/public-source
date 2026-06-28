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

AActor* AKLSinkStorageHologram::GetUpgradedActor() const { return mUpgradeActor; }

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

		if (AsStorage->GetClass() != mBuildClass && Default->mInventorySizeX == AsStorage->mInventorySizeX &&
			Default->mInventorySizeY == AsStorage->mInventorySizeY)
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
			if (UFGInventoryComponent* SourceInventory = AsStorage->GetStorageInventory())
			{
				if (UFGInventoryComponent* TargetInventory = NewAsStorage->GetStorageInventory())
				{
					// Size the target to its OWN intended slot count (from its CDO), not the
					// source size — using the source size compounds with BeginPlay's own
					// Resize() and FG's native upgrade transfer, doubling the inventory.
					const AFGBuildableStorage* TargetDefault = GetDefaultBuildable<AFGBuildableStorage>();
					const int32 TargetSize =
						IsValid(TargetDefault) ? TargetDefault->mInventorySizeX * TargetDefault->mInventorySizeY : 0;

					if (TargetSize > 0 && TargetInventory->GetSizeLinear() != TargetSize)
					{
						TargetInventory->Resize(TargetSize);
					}

					TargetInventory->Empty();

					// Only copy as many slots as the target can hold — prevents an oversized
					// source from silently growing the target beyond its intended capacity.
					const int32 CopyCount = FMath::Min(SourceInventory->GetSizeLinear(), TargetSize);
					for (int32 SlotIndex = 0; SlotIndex < CopyCount; ++SlotIndex)
					{
						FInventoryStack Stack;
						if (SourceInventory->GetStackFromIndex(SlotIndex, Stack) && Stack.HasItems())
						{
							TargetInventory->AddStackToIndex(SlotIndex, Stack, false);
						}
					}

					SourceInventory->Empty();
				}
			}
		}
	}
}
