// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Equipment/FGEquipment.h"
#include "KPCLEquipmentBase.generated.h"

UCLASS()
class KPRIVATECODELIB_API AKPCLEquipmentBase : public AFGEquipment
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AKPCLEquipmentBase();

protected:
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void Equip(AFGCharacterPlayer* character) override;
	virtual void UnEquip() override;
	virtual void DisableEquipment() override;
	virtual void WasEquipped_Implementation() override;
	virtual void WasUnEquipped_Implementation() override;

	virtual void Clear();
	virtual void Cache();
	virtual void Trace();

	virtual void OnNewActorHit(AActor* Hit, AActor* Last);
	virtual void OnLastActorChanged(AActor* NewLast, AActor* LastLast);

	template <class T>
	T* GetHit(bool LastHit = false);

	// Bind Modded Inputs
	virtual void AddEquipmentActionBindings() override;
	//virtual void SetEquipmentBindings(UEnhancedInputComponent* EIC);

public:
	UFUNCTION(BlueprintPure)
	UFGOutlineComponent* GetCachedOutlineComponent() const;

	UFUNCTION(BlueprintPure)
	AActor* GetCurrentHit() const;

	template <class T>
	T* GetCurrentHit() const;

	UFUNCTION(BlueprintPure)
	AActor* GetLastHit() const;

	template <class T>
	T* GetLastHit() const;

protected:
	/** Input Actions */
	void Input_PrimaryFire(const FInputActionValue& actionValue);
	void Input_SecondaryFire(const FInputActionValue& actionValue);
	void Input_Wheel(const FInputActionValue& actionValue);

	UFUNCTION(NetMulticast, Reliable)
	virtual void MultiCast_OnLeftClick();
	virtual void OnLeftClick();
	virtual void OnLeftClickReleased();
	bool bLeftIsClicked = false;

	UFUNCTION(NetMulticast, Reliable)
	virtual void MultiCast_OnRightClick();
	virtual void OnRightClick();
	virtual void OnRightClickReleased();
	bool bRightIsClicked = false;

	UFUNCTION(NetMulticast, Reliable)
	virtual void MultiCast_OnMiddleMouseButton();
	virtual void OnMiddleMouseButton();
	virtual void OnMiddleMouseButtonReleased();
	bool bMiddleMouseButtonIsClicked = false;

	// Cached actor Outline component to add outlines to the actors/signs
	UPROPERTY()
	UFGOutlineComponent* mCachedOutlineComponent;

	// Trace BoxSize
	UPROPERTY(EditDefaultsOnly, Category="KMods|LineTrace")
	bool mShouldUseLineTrace = false;

	// Trace BoxSize
	UPROPERTY(EditDefaultsOnly, Category="KMods|LineTrace",
		meta=( EditCondition = mShouldUseLineTrace, EditConditionHides ))
	FVector mTraceBoxHalfSize = FVector(10.f);

	// Channels for the line trace
	UPROPERTY(EditDefaultsOnly, Category="KMods|LineTrace",
		meta=( EditCondition = mShouldUseLineTrace, EditConditionHides ))
	TArray<TEnumAsByte<EObjectTypeQuery>> mTraceObjects;

	// range for the Sign Gun
	UPROPERTY(EditDefaultsOnly, Category="KMods|LineTrace",
		meta=( EditCondition = mShouldUseLineTrace, EditConditionHides ))
	float mTraceRange = 15000.f;

	UPROPERTY()
	AActor* mCurrentActor;

	UPROPERTY()
	AActor* mLastActor;
};

template <class T>
T* AKPCLEquipmentBase::GetHit(bool LastHit)
{
	return Cast<T>(LastHit ? mLastActor : mCurrentActor);
}

template <class T>
T* AKPCLEquipmentBase::GetCurrentHit() const
{
	return Cast<T>(GetCurrentHit());
}

template <class T>
T* AKPCLEquipmentBase::GetLastHit() const
{
	return Cast<T>(GetLastHit());
}
