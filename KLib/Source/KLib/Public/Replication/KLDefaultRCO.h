// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Replication/KPCLDefaultRCO.h"
#include "KLDefaultRCO.generated.h"

/**
 * 
 */
UCLASS()
class KLIB_API UKLDefaultRCO : public UKPCLDefaultRCO
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(Server, WithValidation, Unreliable, BlueprintCallable)
	void Server_SetTargetHumidity(AFGBuildable* Building, float NewTargetHumidity);
	bool Server_SetTargetHumidity_Validate(AFGBuildable* Building, float NewTargetHumidity) { return true; }

	UFUNCTION(Server, WithValidation, Unreliable, BlueprintCallable)
	void Server_SetTargetTemperature(AFGBuildable* Building, float NewTargetTemperature);
	bool Server_SetTargetTemperature_Validate(AFGBuildable* Building, float NewTargetTemperature) { return true; }

	UFUNCTION(Server, WithValidation, Unreliable, BlueprintCallable)
	void Server_SetNewModuleTime(AFGBuildable* Building, EKAPISlugTime NewTime);
	bool Server_SetNewModuleTime_Validate(AFGBuildable* Building, EKAPISlugTime NewTime) { return true; }

	UPROPERTY(Replicated)
	bool mDummy2 = true;
};
