// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Descriptors/KAPIModularAttachmentDescriptor.h"
#include "Structures/KPCLFunctionalStructure.h"
#include "UObject/Interface.h"
#include "KPCLModularBuildingInterface.generated.h"

class AFGBuildable;
// This class does not need to be modified.
UINTERFACE()
class UKPCLModularBuildingInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class KPRIVATECODELIB_API IKPCLModularBuildingInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Modular Building Interface")
	bool IsAllowedToSnap(AFGBuildable* Actor);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Modular Building Interface")
	void SetAttachedActor(AFGBuildable* Actor);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Modular Building Interface")
	void RemoveAttachedActor(AFGBuildable* Actor);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Modular Building Interface")
	bool AttachedActor(AFGBuildable* Actor, TSubclassOf<UKAPIModularAttachmentDescriptor> Attachment,
	                   FTransform Location, float Distance = 500.0f);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Modular Building Interface")
	bool GetCanHaveModules();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Modular Building Interface")
	class AFGBuildable* GetMasterBuilding();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Modular Building Interface")
	void SetMasterBuilding(AFGBuildable* Actor);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Modular Building Interface")
	class UKPCLModularBuildingHandlerBase* GetModularHandler();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Modular Building Interface")
	FPowerOptions GetPowerOptions();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Modular Building Interface")
	bool IsModuleTypeOf(uint8 Type);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Modular Building Interface")
	void ApplyModularIndex(int32 Index);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Modular Building Interface")
	int32 GetModularIndex();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Modular Building Interface")
	TSubclassOf<UKAPIModularAttachmentDescriptor> GetModularAttachmentClass();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Modular Building Interface")
	void Stacker_AddBuildingHeight(float& Height);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Modular Building Interface")
	void POST_RemoveAttachedActor();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Modular Building Interface")
	void OnModulesUpdated();
};
