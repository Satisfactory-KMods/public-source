// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#include "Replication/SBSDefaultRCO.h"

#include "Net/UnrealNetwork.h"

void USBSDefaultRCO::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USBSDefaultRCO, mDummy2);
}
