// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#include "DataAssets/KAPICleanerItemDescription.h"

#include "Resources/FGNoneDescriptor.h"

bool UKAPICleanerItemDescription::CleanerItemNeeded() const
{
	if (!mNeedCleanItem)
	{
		return false;
	}
	return IsValid(mCleanerItemInfo.mProduceItem) &&
		!mCleanerItemInfo.mProduceItem->IsChildOf(UFGNoneDescriptor::StaticClass()) &&
		mCleanerItemInfo.mProduceAmount > 0 && mCleanerItemInfo.mProductionTime > 0.f;
}
