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
#include "RssBlueprintFunctionLibrary.h"

#include "Net/UnrealNetwork.h"


// RCO

namespace
{
bool IsRssSignActor(AActor* Building)
{
	return IsValid(Building) &&
		UKismetSystemLibrary::DoesImplementInterface(Building, URssSignInterface::StaticClass());
}

bool IsCallerNear(const URSSSignRCO* RCO, const AActor* Building)
{
	const AFGPlayerController* Controller = RCO ? RCO->GetTypedOuter<AFGPlayerController>() : nullptr;
	const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	return Pawn && Building && Pawn->GetWorld() == Building->GetWorld() &&
		FVector::DistSquared(Pawn->GetActorLocation(), Building->GetActorLocation()) <= FMath::Square(16000.0f);
}

bool IsRequestSafe(const AActor* Building, const FRssSignRequestData& Request)
{
	return Request.mBuildable == Building && Request.mRequestUrl.Len() <= 2048 &&
		Request.mRequestOnImageIndex >= -1 && Request.mRequestOnImageIndex < 128;
}
}

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

bool URSSSignRCO::RCO_Server_UpdateSignData_Validate(AActor* building, FRssSignData Data)
{
	return IsRssSignActor(building) && IsCallerNear(this, building) &&
		URssBlueprintFunctionLibrary::IsSignDataSafe(Data);
}

void URSSSignRCO::RCO_MultiCast_ApplySignData_Implementation(AActor* building, FRssSignData Data)
{
	if (UKismetSystemLibrary::DoesImplementInterface(building, URssSignInterface::StaticClass()))
	{
		IRssSignInterface::Execute_ApplySignData(building, Data);
		building->ForceNetUpdate();
	}
}

bool URSSSignRCO::RCO_MultiCast_ApplySignData_Validate(AActor* building, FRssSignData Data)
{
	return IsRssSignActor(building) && URssBlueprintFunctionLibrary::IsSignDataSafe(Data);
}

void URSSSignRCO::RCO_MultiCast_RequestSignData_Implementation(AActor* building, FRssSignRequestData Request)
{
	if (UKismetSystemLibrary::DoesImplementInterface(building, URssSignInterface::StaticClass()))
	{
		IRssSignInterface::Execute_RequestCustomSign(building, Request, false);
		building->ForceNetUpdate();
	}
}

bool URSSSignRCO::RCO_MultiCast_RequestSignData_Validate(AActor* building, FRssSignRequestData Request)
{
	return IsRssSignActor(building) && IsRequestSafe(building, Request);
}

void URSSSignRCO::RCO_Server_RequestSignData_Implementation(AActor* building, FRssSignRequestData Request)
{
	if (UKismetSystemLibrary::DoesImplementInterface(building, URssSignInterface::StaticClass()))
	{
		IRssSignInterface::Execute_RequestCustomSign(building, Request, true);
		building->ForceNetUpdate();
	}
}

bool URSSSignRCO::RCO_Server_RequestSignData_Validate(AActor* building, FRssSignRequestData Request)
{
	return IsRssSignActor(building) && IsCallerNear(this, building) && IsRequestSafe(building, Request);
}

void URSSSignRCO::RCO_Client_RequestSignData_Implementation(AActor* building, FRssSignRequestData Request)
{
	if (UKismetSystemLibrary::DoesImplementInterface(building, URssSignInterface::StaticClass()))
	{
		IRssSignInterface::Execute_RequestCustomSign(building, Request, true);
		building->ForceNetUpdate();
	}
}

bool URSSSignRCO::RCO_Client_RequestSignData_Validate(AActor* building, FRssSignRequestData Request)
{
	return IsRssSignActor(building) && IsRequestSafe(building, Request);
}

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
	return IsRssSignActor(building) && PlayerController == GetTypedOuter<AFGPlayerController>() &&
		IsCallerNear(this, building);
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
	return IsRssSignActor(building) && PlayerController == GetTypedOuter<AFGPlayerController>() &&
		IsCallerNear(this, building);
}

void URSSSignRCO::RCO_Client_PasteSignData_Implementation(ARssSignGun* SignGun)
{
	if (IsValid(SignGun))
	{
		SignGun->PastePasteSign();
	}
}

bool URSSSignRCO::RCO_Client_PasteSignData_Validate(ARssSignGun* SignGun)
{
	return IsValid(SignGun) && SignGun->GetWorld() == GetWorld();
}
