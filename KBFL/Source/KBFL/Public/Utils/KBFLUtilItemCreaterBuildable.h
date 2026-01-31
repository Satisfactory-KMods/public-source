// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "Buildables/FGBuildableStorage.h"
#include "FGPipeConnectionFactory.h"
#include "Hologram/FGFactoryBuildingHologram.h"
#include "KBFLUtilItemCreaterBuildable.generated.h"

UCLASS()
class KBFL_API AKBFLUtilItemCreaterBuildable : public AFGBuildableStorage
{
	GENERATED_BODY()

public:
	AKBFLUtilItemCreaterBuildable();

	virtual void Factory_Tick(float dt) override;
	virtual void PostInitializeComponents() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void BeginPlay() override;

	void DestroyItems();
	void CreateItems();

	UFUNCTION(BlueprintCallable)
	void SetBeltItem(TSubclassOf<UFGItemDescriptor> Item);

	UFUNCTION(BlueprintCallable)
	void SetPipeItem(TSubclassOf<UFGItemDescriptor> Item);

	UFUNCTION(BlueprintPure)
	FORCEINLINE TSubclassOf<UFGItemDescriptor> GetBeltItem() const { return mBeltItemClassToGenerate; }

	UFUNCTION(BlueprintPure)
	FORCEINLINE TSubclassOf<UFGItemDescriptor> GetPipeItem() const { return mPipeItemClassToGenerate; }

private:
	UPROPERTY(SaveGame, Replicated)
	TSubclassOf<UFGItemDescriptor> mBeltItemClassToGenerate;

	UPROPERTY(SaveGame, Replicated)
	TSubclassOf<UFGItemDescriptor> mPipeItemClassToGenerate;

	UPROPERTY(Transient)
	TObjectPtr<UFGPipeConnectionFactory> mPipeInput;

	UPROPERTY(Transient)
	TObjectPtr<UFGPipeConnectionFactory> mPipeOutput;

	UPROPERTY(Transient)
	TObjectPtr<UFGFactoryConnectionComponent> mBeltInput;

	UPROPERTY(Transient)
	TObjectPtr<UFGFactoryConnectionComponent> mBeltOutput;
};


UCLASS()
class KBFL_API UKBFLDefaultRCO : public UFGRemoteCallObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Server, WithValidation, Unreliable)
	void Server_CheatBuilding_SetPipeItem(AKBFLUtilItemCreaterBuildable* Building,
										  TSubclassOf<UFGItemDescriptor> ItemToSet);

	bool Server_CheatBuilding_SetPipeItem_Validate(AKBFLUtilItemCreaterBuildable* Building,
												   TSubclassOf<UFGItemDescriptor> ItemToSet)
	{
		return true;
	}

	UFUNCTION(BlueprintCallable, Server, WithValidation, Unreliable)
	void Server_CheatBuilding_SetBeltItem(AKBFLUtilItemCreaterBuildable* Building,
										  TSubclassOf<UFGItemDescriptor> ItemToSet);

	bool Server_CheatBuilding_SetBeltItem_Validate(AKBFLUtilItemCreaterBuildable* Building,
												   TSubclassOf<UFGItemDescriptor> ItemToSet)
	{
		return true;
	}

	UFUNCTION(BlueprintCallable, Server, WithValidation, Unreliable)
	void Server_AddStackToCharacter(AFGCharacterPlayer* Player, TSubclassOf<UFGItemDescriptor> ItemToAdd);

	bool Server_AddStackToCharacter_Validate(AFGCharacterPlayer* Player, TSubclassOf<UFGItemDescriptor> ItemToAdd)
	{
		return true;
	}

	UFUNCTION(BlueprintCallable, Server, WithValidation, Unreliable)
	void Server_ClearCharacterInventory(AFGCharacterPlayer* Player);
	bool Server_ClearCharacterInventory_Validate(AFGCharacterPlayer* Player) { return true; }

	UPROPERTY(Replicated)
	bool bDummy = true;
};
