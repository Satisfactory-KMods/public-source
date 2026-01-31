// Copyright Coffee Stain Studios. All Rights Reserved.


#include "Components/KPCLNetworkPlayerComponent.h"

#include "KPrivateCodeLibModule.h"
#include "Buildables/FGBuildable.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Network/KPCLNetwork.h"
#include "Network/KPCLNetworkConnectionComponent.h"
#include "Network/Buildings/KPCLNetworkCore.h"
#include "Subsystem/KPCLFaxitSubsystem.h"

void UKPCLNetworkPlayerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UKPCLNetworkPlayerComponent, mNextBuilding);
	DOREPLIFETIME(UKPCLNetworkPlayerComponent, mDistanceStrength);
	DOREPLIFETIME(UKPCLNetworkPlayerComponent, mNextCore);
	DOREPLIFETIME(UKPCLNetworkPlayerComponent, mUnlockedPermissions);
	DOREPLIFETIME(UKPCLNetworkPlayerComponent, mIsAllowedToSetOverflow);
	DOREPLIFETIME(UKPCLNetworkPlayerComponent, mUnlockedPermissions);
}

void UKPCLNetworkPlayerComponent::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("UKPCLNetworkPlayerComponent::BeginPlay: %d"), mUnlockedPermissions.Num())
	if (GetOwner()->HasAuthority())
	{
		// TODO: asöldäkfmölasdfgklöjasöpdlokfölkasdkföpo
		// DoDistanceCheck();
	}
}

void UKPCLNetworkPlayerComponent::PostLoadGame_Implementation(int32 saveVersion, int32 gameVersion)
{
	if (GetOwner()->HasAuthority())
	{
		// DoDistanceCheck();
	}
}

bool UKPCLNetworkPlayerComponent::ShouldSave_Implementation() const
{
	return true;
}

void UKPCLNetworkPlayerComponent::DoDistanceCheck()
{
	if (!IsStateActive())
	{
		return;
	}

	FVector Loc = GetPlayerCharacterLocation();
	const TArray<AActor*> ActorsToIgnore{};
	TArray<AActor*> OutActors{};

	if (UKismetSystemLibrary::SphereOverlapActors(this, Loc, mMaxDistance, mObjectTypes, AFGBuildable::StaticClass(),
	                                              ActorsToIgnore, OutActors))
	{
		float LowestDistance = mMaxDistance;
		AFGBuildable* NearstActor = nullptr;
		UKPCLNetworkConnectionComponent* NearstNetworkComponent = nullptr;

		for (AActor* Actor : OutActors)
		{
			if (IsValid(Actor))
			{
				if (UKismetSystemLibrary::DoesImplementInterface(Actor, UKPCLNetworkDataInterface::StaticClass()))
				{
					UKPCLNetworkConnectionComponent* Comp = Actor->FindComponentByClass<
						UKPCLNetworkConnectionComponent>();
					float Dist = FVector::Distance(Loc, Comp->GetComponentLocation());
					if (Dist <= LowestDistance)
					{
						NearstActor = Cast<AFGBuildable>(Actor);
						NearstNetworkComponent = Comp;
						if (IsValid(NearstActor))
						{
							LowestDistance = Dist;
						}
					}
				}
			}
		}

		if (IsValid(NearstActor))
		{
			if (IsValid(NearstNetworkComponent))
			{
				const UKPCLNetwork* Network = Cast<UKPCLNetwork>(NearstNetworkComponent->GetPowerCircuit());
				if (IsValid(Network))
				{
					if (Network->NetworkHasCore())
					{
						mNextCore = Network->GetCore();
					}
				}
			}

			mNextBuilding = NearstActor;
			mDistanceStrength = FMath::Clamp<int32>(
				FMath::CeilToInt(
					FVector::Distance(
						Loc, NearstActor->FindComponentByClass<UKPCLNetworkConnectionComponent>()->
						                  GetComponentLocation()) / mMaxDistance * 100), 0, 100);

			OnRep_DistanceUpdated();
		}
	}
	else
	{
		mNextBuilding = nullptr;
		mNextCore = nullptr;
		mDistanceStrength = 0;

		OnRep_DistanceUpdated();
	}
}

void UKPCLNetworkPlayerComponent::CustomTick(float dt)
{
	if (GetOwner()->HasAuthority())
	{
		if (mDistanceCheckTimer.Tick(dt))
		{
			// DoDistanceCheck();
		}
	}
}

FVector UKPCLNetworkPlayerComponent::GetPlayerCharacterLocation() const
{
	if (IsValid(GetPlayerPawn()))
	{
		return GetPlayerPawn()->GetActorLocation();
	}
	return FVector(0);
}

AFGPlayerState* UKPCLNetworkPlayerComponent::GetPlayerState() const
{
	return Cast<AFGPlayerState>(GetOwner());
}

AFGCharacterPlayer* UKPCLNetworkPlayerComponent::GetPlayerCharacter() const
{
	if (IsStateActive())
	{
		return Cast<AFGCharacterPlayer>(GetPlayerState()->GetOwnedPawn());
	}

	return nullptr;
}

bool UKPCLNetworkPlayerComponent::IsPlayerDead() const
{
	AFGCharacterPlayer* Player = GetPlayerCharacter();
	if (IsValid(Player))
	{
		return !Player->IsAliveAndWell();
	}

	return true;
}

APawn* UKPCLNetworkPlayerComponent::GetPlayerPawn() const
{
	if (IsStateActive())
	{
		return GetPlayerState()->GetOwnedPawn();
	}

	return nullptr;
}

bool UKPCLNetworkPlayerComponent::IsStateActive() const
{
	if (IsValid(GetPlayerState()))
	{
		return GetPlayerState()->IsInPlayerArray();
	}
	return false;
}

bool UKPCLNetworkPlayerComponent::GetPlayerCharacterInventory(UFGInventoryComponent*& OutInventory)
{
	if (!IsStateActive())
	{
		return false;
	}

	if (!IsValid(mCachedPlayerCharacterInventory))
	{
		if (IsValid(GetPlayerCharacter()))
		{
			mCachedPlayerCharacterInventory = GetPlayerCharacter()->GetInventory();
		}
	}

	OutInventory = mCachedPlayerCharacterInventory;
	return IsValid(OutInventory);
}

bool UKPCLNetworkPlayerComponent::DistanceAccessUnlocked() const
{
	AKPCLFaxitSubsystem* Faxit = AKPCLFaxitSubsystem::Get(GetWorld());
	if (!Faxit)
	{
		return false;
	}
	return Faxit->mRemoteAccessUnlocked;
}

UKPCLNetworkPlayerComponent* UKPCLNetworkPlayerComponent::GetOrCreateNetworkComponentToPlayerState(
	UObject* WorldContextObject, AFGPlayerState* State, TSubclassOf<UKPCLNetworkPlayerComponent> ComponentClass)
{
	// Try to get Player State if it's invalid
	if (!IsValid(State) && IsValid(WorldContextObject))
	{
		if (IsValid(WorldContextObject->GetWorld()))
		{
			if (IsValid(WorldContextObject->GetWorld()->GetFirstPlayerController()))
			{
				State = WorldContextObject->GetWorld()->GetFirstPlayerController()->GetPlayerState<AFGPlayerState>();
			}
		}
	}

	// If still invalid return nullptr
	if (!IsValid(State))
	{
		UE_LOG(LogKPCL, Error, TEXT("GetOrCreateNetworkComponentToPlayerState: Invalid Player State!"))
		return nullptr;
	}

	UKPCLNetworkPlayerComponent* Component = State->FindComponentByClass<UKPCLNetworkPlayerComponent>();
	if (!IsValid(Component))
	{
		if (!State->HasAuthority())
		{
			return Component;
		}

		const TSubclassOf<UKPCLNetworkPlayerComponent> Class = !IsValid(ComponentClass)
			                                                       ? TSubclassOf<UKPCLNetworkPlayerComponent>{
				                                                       StaticClass()
			                                                       }
			                                                       : ComponentClass;
		if (IsValid(Class))
		{
			Component = NewObject<UKPCLNetworkPlayerComponent>(State, Class, FName("NetworkComp"));
			Component->SetIsReplicatedByDefault(true);
			Component->SetIsReplicated(true);
			Component->RegisterComponentWithWorld(State->GetWorld());
			Component->SetIsReplicated(true);
			State->ForceNetUpdate();
			UE_LOG(LogKPCL, Warning, TEXT("GetOrCreateNetworkComponentToPlayerState: New Component Created!"))
		}
		else
		{
			UE_LOG(LogKPCL, Error, TEXT("GetOrCreateNetworkComponentToPlayerState: Invalid Class!"))
		}
	}

	return Component;
}

bool UKPCLNetworkPlayerComponent::CanAccessTo() const
{
	return IsValid(GetNextBuilding()) && DistanceAccessUnlocked();
}

int32 UKPCLNetworkPlayerComponent::GetDistanceStrength() const
{
	if (!CanAccessTo())
	{
		return 0;
	}

	return mDistanceStrength;
}

AFGBuildable* UKPCLNetworkPlayerComponent::GetNextBuilding() const
{
	if (IsValid(mNextCore))
	{
		return Cast<AFGBuildable>(mNextCore);
	}

	return mNextBuilding;
}

void UKPCLNetworkPlayerComponent::OnRep_DistanceUpdated()
{
	if (mOnDistanceUpdated.IsBound())
	{
		mOnDistanceUpdated.Broadcast(mDistanceStrength);
	}
}
