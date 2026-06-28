// Fill out your copyright notice in the Description page of Project Settings.


#include "Buildable/RSSSignRCO.h"

#include "Buildable/RSSBuildableSign.h"
#include "Buildable/RSSBuildableSignFoundation.h"
#include "Buildable/RSSBuildableSignVanilla.h"
#include "Buildable/RSSBuildableSignWalkway.h"
#include "Buildable/RSSBuildableSignWall.h"
#include "Equipment/RssSignGun.h"
#include "Interface/RssSignInterface.h"
#include "Kismet/KismetSystemLibrary.h"

#include "Net/UnrealNetwork.h"


// RCO

void URSSSignRCO::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(URSSSignRCO, bDummy);
}

void URSSSignRCO::RCO_Server_UpdateSignData_Implementation(AActor* building, FRssSignData Data)
{
	if (UKismetSystemLibrary::DoesImplementInterface(building, URssSignInterface::StaticClass()))
	{
		IRssSignInterface::Execute_UpdateSignData(building, Data);
		building->ForceNetUpdate();
	}
}

bool URSSSignRCO::RCO_Server_UpdateSignData_Validate(AActor* building, FRssSignData Data) { return true; }

void URSSSignRCO::RCO_MultiCast_ApplySignData_Implementation(AActor* building, FRssSignData Data)
{
	if (UKismetSystemLibrary::DoesImplementInterface(building, URssSignInterface::StaticClass()))
	{
		IRssSignInterface::Execute_ApplySignData(building, Data);
		building->ForceNetUpdate();
	}
}

bool URSSSignRCO::RCO_MultiCast_ApplySignData_Validate(AActor* building, FRssSignData Data) { return true; }

void URSSSignRCO::RCO_MultiCast_RequestSignData_Implementation(AActor* building, FRssSignRequestData Request)
{
	if (UKismetSystemLibrary::DoesImplementInterface(building, URssSignInterface::StaticClass()))
	{
		IRssSignInterface::Execute_RequestCustomSign(building, Request, false);
		building->ForceNetUpdate();
	}
}

bool URSSSignRCO::RCO_MultiCast_RequestSignData_Validate(AActor* building, FRssSignRequestData Request) { return true; }

void URSSSignRCO::RCO_Server_RequestSignData_Implementation(AActor* building, FRssSignRequestData Request)
{
	if (UKismetSystemLibrary::DoesImplementInterface(building, URssSignInterface::StaticClass()))
	{
		IRssSignInterface::Execute_RequestCustomSign(building, Request, true);
		building->ForceNetUpdate();
	}
}

bool URSSSignRCO::RCO_Server_RequestSignData_Validate(AActor* building, FRssSignRequestData Request) { return true; }

void URSSSignRCO::RCO_Client_RequestSignData_Implementation(AActor* building, FRssSignRequestData Request)
{
	if (UKismetSystemLibrary::DoesImplementInterface(building, URssSignInterface::StaticClass()))
	{
		IRssSignInterface::Execute_RequestCustomSign(building, Request, true);
		building->ForceNetUpdate();
	}
}

bool URSSSignRCO::RCO_Client_RequestSignData_Validate(AActor* building, FRssSignRequestData Request) { return true; }

void URSSSignRCO::RCO_Server_OpenWidget_Implementation(AActor* building, AFGPlayerController* PlayerController)
{
	if (UKismetSystemLibrary::DoesImplementInterface(building, URssSignInterface::StaticClass()))
	{
		IRssSignInterface::Execute_RequestInteractWidget(building, PlayerController);
		building->ForceNetUpdate();
	}
}

bool URSSSignRCO::RCO_Server_OpenWidget_Validate(AActor* building, AFGPlayerController* PlayerController)
{
	return true;
}

void URSSSignRCO::RCO_Server_CloseWidget_Implementation(AActor* building, AFGPlayerController* PlayerController)
{
	if (UKismetSystemLibrary::DoesImplementInterface(building, URssSignInterface::StaticClass()))
	{
		IRssSignInterface::Execute_RequestCloseWidget(building, PlayerController);
		building->ForceNetUpdate();
	}
}

bool URSSSignRCO::RCO_Server_CloseWidget_Validate(AActor* building, AFGPlayerController* PlayerController)
{
	return true;
}

void URSSSignRCO::RCO_Client_PasteSignData_Implementation(ARssSignGun* SignGun)
{
	if (IsValid(SignGun))
	{
		SignGun->PastePasteSign();
	}
}

bool URSSSignRCO::RCO_Client_PasteSignData_Validate(ARssSignGun* SignGun) { return true; }
