// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "Replication/KPCLDefaultRCO.h"

#include "KLDefaultRCO.generated.h"

class UKAPICleanerItemDescription;

UCLASS()
class KLIB_API UKLDefaultRCO : public UKPCLDefaultRCO
{
	GENERATED_BODY()

public:
	//~ Begin UObject Interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ End UObject Interface

	UFUNCTION(Server, WithValidation, Reliable, BlueprintCallable)
	void Server_SetCleanerRecipe(AFGBuildable* Building, UKAPICleanerItemDescription* NewRecipe);
	bool Server_SetCleanerRecipe_Validate(AFGBuildable* Building, UKAPICleanerItemDescription* NewRecipe)
	{
		return true;
	}

	UFUNCTION(Server, WithValidation, Unreliable, BlueprintCallable)
	void Server_SetNewModuleTime(AFGBuildable* Building, EKAPISlugTime NewTime);
	bool Server_SetNewModuleTime_Validate(AFGBuildable* Building, EKAPISlugTime NewTime) { return true; }

	UFUNCTION(Server, WithValidation, Unreliable, BlueprintCallable)
	void Server_SetTargetHumidity(AFGBuildable* Building, float NewTargetHumidity);
	bool Server_SetTargetHumidity_Validate(AFGBuildable* Building, float NewTargetHumidity) { return true; }

	UFUNCTION(Server, WithValidation, Unreliable, BlueprintCallable)
	void Server_SetTargetTemperature(AFGBuildable* Building, float NewTargetTemperature);
	bool Server_SetTargetTemperature_Validate(AFGBuildable* Building, float NewTargetTemperature) { return true; }

	UPROPERTY(Replicated)
	bool mDummy2 = true;
};
