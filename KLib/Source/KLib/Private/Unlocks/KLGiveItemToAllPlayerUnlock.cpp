// Copyright Coffee Stain Studios. All Rights Reserved.

#include "Unlocks/KLGiveItemToAllPlayerUnlock.h"

#include "Buildables/FGBuildableStorage.h"
#include "Buildables/FGBuildableTradingPost.h"
#include "FGInventoryComponent.h"
#include "FGItemPickup_Spawnable.h"


bool UKLGiveItemToAllPlayerUnlock::IsRepeatPurchasesAllowed_Implementation() const { return bCanRepeat; }

void UKLGiveItemToAllPlayerUnlock::Unlock(AFGUnlockSubsystem* unlockSubssytem)
{
	Super::Unlock(unlockSubssytem);

	if (!IsValid(unlockSubssytem))
	{
		return;
	}

	UWorld* World = unlockSubssytem->GetWorld();
	if (!IsValid(World) || World->GetNetMode() == NM_Client)
	{
		return;
	}

	AFGTutorialIntroManager* TutorialIntro = AFGTutorialIntroManager::Get(unlockSubssytem);
	AFGBuildableTradingPost* TradingPost = IsValid(TutorialIntro) ? TutorialIntro->mTradingPost.Get() : nullptr;
	AFGBuildableStorage* TradingPostStorage =
		IsValid(TradingPost) ? Cast<AFGBuildableStorage>(TradingPost->mStorage) : nullptr;
	UFGInventoryComponent* StorageInventory =
		IsValid(TradingPostStorage) ? TradingPostStorage->GetStorageInventory() : nullptr;

	TArray<FInventoryStack> OverflowStacks;
	for (const FItemAmount& Amount : mAmounts)
	{
		if (!Amount.ItemClass || Amount.Amount <= 0)
		{
			continue;
		}

		FInventoryStack Stack(Amount.Amount, Amount.ItemClass);
		const int32 StoredAmount = IsValid(StorageInventory) ? StorageInventory->AddStack(Stack, true) : 0;
		if (StoredAmount < Stack.NumItems)
		{
			Stack.NumItems -= StoredAmount;
			OverflowStacks.Add(Stack);
		}
	}

	if (!OverflowStacks.IsEmpty())
	{
		const FVector DropLocation = IsValid(TradingPostStorage) ? TradingPostStorage->GetActorLocation()
			: IsValid(TradingPost)								 ? TradingPost->GetActorLocation()
																 : unlockSubssytem->GetActorLocation();
		AFGItemPickup_Spawnable::SpawnInventoryCrate(World, OverflowStacks, DropLocation);
	}
}
