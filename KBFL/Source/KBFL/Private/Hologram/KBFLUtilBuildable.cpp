// Copyright Coffee Stain Studios. All Rights Reserved.


#include "Hologram/KBFLUtilBuildable.h"
#include "Subsystems/KBFLLocationSubsystem.h"

#include "Net/UnrealNetwork.h"

void AKBFLUtilBuildable::BeginPlay()
{
	Super::BeginPlay();
	AKBFLLocationSubsystem* Subsystem = AKBFLLocationSubsystem::GetAKBFLLocationSubsystem(GetWorld());
	if (Subsystem)
	{
		SetActorScale3D(Scale);
		Subsystem->SaveLocation(GetClass(), GetActorTransform());
	}
}

void AKBFLUtilBuildable::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AKBFLUtilBuildable, Scale);
}

void AKBFLUtilBuildable::OnRep_Scale() { SetActorScale3D(Scale); }

void AKBFLUtilBuildable::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (EndPlayReason == EEndPlayReason::Destroyed)
	{
		if (AKBFLLocationSubsystem* Subsystem = AKBFLLocationSubsystem::GetAKBFLLocationSubsystem(GetWorld()))
		{
			Subsystem->RemoveLocation(GetClass(), GetActorTransform());
		}
	}
	Super::EndPlay(EndPlayReason);
}
