// Copyright Coffee Stain Studios. All Rights Reserved.

#include "Unlocks/KLGiveItemToAllPlayerUnlock.h"

#include "Buildables/FGBuildableHubTerminal.h"
#include "Buildables/FGBuildableTradingPost.h"
#include "FGItemPickup_Spawnable.h"


bool UKLGiveItemToAllPlayerUnlock::IsRepeatPurchasesAllowed_Implementation() const { return mCanRepeat; }

void UKLGiveItemToAllPlayerUnlock::Unlock(AFGUnlockSubsystem* unlockSubssytem)
{
	Super::Unlock(unlockSubssytem);

	if (unlockSubssytem->GetWorld()->GetNetMode() != NM_Client)
	{
		AFGTutorialIntroManager* TutorialIntro = AFGTutorialIntroManager::Get(unlockSubssytem);
		FVector TerminalLocation = FVector::ZeroVector;

		if (IsValid(TutorialIntro) && IsValid(TutorialIntro->mTradingPost) &&
			IsValid(TutorialIntro->mTradingPost->mHubTerminal))
		{
			TerminalLocation = TutorialIntro->mTradingPost->mHubTerminal->GetActorLocation();
		}

		AFGCharacterPlayer* NearstPlayerToHub = nullptr;
		for (TPlayerControllerIterator<AFGPlayerController>::ServerAll It(unlockSubssytem->GetWorld()); It; ++It)
		{
			if (!It || !IsValid(*It))
			{
				continue;
			}
			AFGPlayerController* PC = *It;
			AFGCharacterPlayer* Player = Cast<AFGCharacterPlayer>(PC->GetCharacter());

			if (!IsValid(Player))
			{
				continue;
			}

			if (mToAllPlayer)
			{
				GiveToPlayer(Player);
			}
			else
			{
				if (!IsValid(NearstPlayerToHub))
				{
					NearstPlayerToHub = Player;
					continue;
				}

				float CurrentDistance = FVector::DistSquared(Player->GetActorLocation(), TerminalLocation);
				float NearestDistance = FVector::DistSquared(NearstPlayerToHub->GetActorLocation(), TerminalLocation);
				if (CurrentDistance < NearestDistance)
				{
					NearstPlayerToHub = Player;
				}
			}
		}

		if (!mToAllPlayer)
		{
			fgcheck(IsValid(NearstPlayerToHub)) GiveToPlayer(NearstPlayerToHub);
		}
	}
}

void UKLGiveItemToAllPlayerUnlock::GiveToPlayer(AFGCharacterPlayer* Player)
{
	if (!IsValid(Player))
	{
		return;
	}

	TArray<FInventoryStack> Stacks;
	for (const FItemAmount& Amount : mAmounts)
	{
		Stacks.Add(FInventoryStack(Amount.Amount, Amount.ItemClass));
	}

	TArray<FInventoryStack> StacksThatCantFitIntoInventory;

	UFGInventoryComponent* PlayerInventory = Player->GetInventory();
	if (!IsValid(PlayerInventory))
	{
		return;
	}

	for (FInventoryStack Stack : Stacks)
	{
		int32 StoredAmount = PlayerInventory->AddStack(Stack, true);
		if (StoredAmount < Stack.NumItems)
		{
			Stack.NumItems -= StoredAmount;
			StacksThatCantFitIntoInventory.Add(Stack);
		}
	}

	if (!StacksThatCantFitIntoInventory.IsEmpty())
	{
		FVector DropLocation = Player->GetInventoryDropLocation();
		AFGCrate* SpawnedCrate = nullptr;
		AFGItemPickup_Spawnable::SpawnInventoryCrate(Player->GetWorld(), StacksThatCantFitIntoInventory, DropLocation,
													 {}, SpawnedCrate, EFGCrateType::CT_None);
	}
}
