// Fill out your copyright notice in the Description page of Project Settings.


#include "Replication/KLDefaultRCO.h"

#include "Buildable/ModularMiner/KLMMBuildableBase.h"
#include "Buildable/Slugs/KLBuildableSlugBreeder.h"
#include "Buildable/Slugs/KLBuildableSlugHatchingModule.h"

#include "Net/UnrealNetwork.h"

void UKLDefaultRCO::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UKLDefaultRCO, mDummy2);
}

void UKLDefaultRCO::Server_SetTargetHumidity_Implementation(AFGBuildable* Building, float NewTargetHumidity)
{
	if (AKLBuildableSlugHatchingModule* Module = Cast<AKLBuildableSlugHatchingModule>(Building))
	{
		Module->ApplyNewHumidity(NewTargetHumidity);
		Module->ForceNetUpdate();
	}

	if (AKLBuildableSlugBreeder* Terrarium = Cast<AKLBuildableSlugBreeder>(Building))
	{
		Terrarium->ApplyNewHumidity(NewTargetHumidity);
		Terrarium->ForceNetUpdate();
	}
}

void UKLDefaultRCO::Server_SetTargetTemperature_Implementation(AFGBuildable* Building, float NewTargetTemperature)
{
	if (AKLBuildableSlugHatchingModule* Module = Cast<AKLBuildableSlugHatchingModule>(Building))
	{
		Module->ApplyNewTemperature(NewTargetTemperature);
		Module->ForceNetUpdate();
		return;
	}

	if (AKLBuildableSlugBreeder* Terrarium = Cast<AKLBuildableSlugBreeder>(Building))
	{
		Terrarium->ApplyNewTemperature(NewTargetTemperature);
		Terrarium->ForceNetUpdate();
	}
}

void UKLDefaultRCO::Server_SetNewModuleTime_Implementation(AFGBuildable* Building, EKAPISlugTime NewTime)
{
	if (AKLBuildableSlugHatchingModule* Module = Cast<AKLBuildableSlugHatchingModule>(Building))
	{
		Module->ApplyNewOverwriteTime(NewTime);
		Module->ForceNetUpdate();
	}
}
