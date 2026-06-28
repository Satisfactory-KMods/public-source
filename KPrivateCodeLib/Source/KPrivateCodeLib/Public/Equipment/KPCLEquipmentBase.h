// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "Equipment/FGEquipment.h"

#include "KPCLEquipmentBase.generated.h"

UCLASS()
class KPRIVATECODELIB_API AKPCLEquipmentBase : public AFGEquipment
{
	GENERATED_BODY()

public:
	AKPCLEquipmentBase();

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
	//~ Begin AFGEquipment Interface
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Equip(AFGCharacterPlayer* character) override;
	virtual void UnEquip() override;
	virtual void DisableEquipment() override;
	virtual void WasEquipped_Implementation() override;
	virtual void WasUnEquipped_Implementation() override;
	virtual void AddEquipmentActionBindings() override;
	//~ End AFGEquipment Interface

	virtual void Clear();
	virtual void Cache();
	virtual void Trace();

	virtual void OnNewActorHit(AActor* Hit, AActor* Last);
	virtual void OnLastActorChanged(AActor* NewLast, AActor* LastLast);

	template <class T>
	T* GetHit(bool LastHit = false);

	/** Input Actions */
	void Input_PrimaryFire(const FInputActionValue& actionValue);
	void Input_SecondaryFire(const FInputActionValue& actionValue);
	void Input_Wheel(const FInputActionValue& actionValue);

	UFUNCTION(NetMulticast, Reliable)
	virtual void MultiCast_OnLeftClick();
	virtual void OnLeftClick();
	virtual void OnLeftClickReleased();

	UFUNCTION(NetMulticast, Reliable)
	virtual void MultiCast_OnRightClick();
	virtual void OnRightClick();
	virtual void OnRightClickReleased();

	UFUNCTION(NetMulticast, Reliable)
	virtual void MultiCast_OnMiddleMouseButton();
	virtual void OnMiddleMouseButton();
	virtual void OnMiddleMouseButtonReleased();

	bool bLeftIsClicked = false;
	bool bRightIsClicked = false;
	bool bMiddleMouseButtonIsClicked = false;

	UPROPERTY()
	TObjectPtr<UFGOutlineComponent> mCachedOutlineComponent;

	UPROPERTY(EditDefaultsOnly, Category = "KMods|LineTrace")
	bool mShouldUseLineTrace = false;

	UPROPERTY(EditDefaultsOnly, Category = "KMods|LineTrace",
			  meta = (EditCondition = mShouldUseLineTrace, EditConditionHides))
	FVector mTraceBoxHalfSize = FVector(10.f);

	UPROPERTY(EditDefaultsOnly, Category = "KMods|LineTrace",
			  meta = (EditCondition = mShouldUseLineTrace, EditConditionHides))
	TArray<TEnumAsByte<EObjectTypeQuery>> mTraceObjects;

	UPROPERTY(EditDefaultsOnly, Category = "KMods|LineTrace",
			  meta = (EditCondition = mShouldUseLineTrace, EditConditionHides))
	float mTraceRange = 15000.f;

	UPROPERTY()
	TObjectPtr<AActor> mCurrentActor;

	UPROPERTY()
	TObjectPtr<AActor> mLastActor;
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
