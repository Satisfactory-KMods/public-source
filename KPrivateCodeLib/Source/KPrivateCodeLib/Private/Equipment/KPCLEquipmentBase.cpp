// Copyright Kyri123 / KMods 2026. All Rights Reserved.



#include "Equipment/KPCLEquipmentBase.h"

#include "BlueprintFunctionLib/KPCLBlueprintFunctionLib.h"
#include "EnhancedInputComponent.h"
#include "FGCharacterPlayer.h"
#include "KPrivateCodeLibModule.h"

AKPCLEquipmentBase::AKPCLEquipmentBase()
{

	PrimaryActorTick.bCanEverTick = true;

}

void AKPCLEquipmentBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (mShouldUseLineTrace)
	{
		Trace();
	}
}

void AKPCLEquipmentBase::BeginPlay()
{
	Super::BeginPlay();

	const AFGCharacterPlayer* Char = GetInstigatorCharacter();
	if (IsValid(Char))
	{
		mCachedOutlineComponent = Char->GetOutline();
	}
}

void AKPCLEquipmentBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	Clear();
}

void AKPCLEquipmentBase::Equip(AFGCharacterPlayer* character)
{
	Super::Equip(character);
	Clear();
	Cache();
}

void AKPCLEquipmentBase::UnEquip()
{
	Super::UnEquip();
	Clear();
}

void AKPCLEquipmentBase::DisableEquipment()
{
	Super::DisableEquipment();
	Clear();
}

void AKPCLEquipmentBase::WasEquipped_Implementation()
{
	Super::WasEquipped_Implementation();
	Clear();
	Cache();
}

void AKPCLEquipmentBase::WasUnEquipped_Implementation()
{
	Super::WasUnEquipped_Implementation();
	Clear();
}

void AKPCLEquipmentBase::Clear()
{
	bLeftIsClicked = false;
	bRightIsClicked = false;
	bMiddleMouseButtonIsClicked = false;
}

void AKPCLEquipmentBase::Cache()
{
	const AFGCharacterPlayer* Char = GetInstigatorCharacter();
	if (IsValid(Char))
	{
		mCachedOutlineComponent = Char->GetOutline();
	}
}

void AKPCLEquipmentBase::Trace()
{
	const TArray<AActor*> IgnoredActors{this, GetInstigatorCharacter()};
	FHitResult Result;

	if (!IsValid(GetInstigatorCharacter()))
	{
		return;
	}

	const FVector Start = GetInstigatorCharacter()->GetCameraComponentWorldLocation();
	const FVector End = Start + mTraceRange * GetInstigatorCharacter()->GetCameraComponentForwardVector();

}

void AKPCLEquipmentBase::OnNewActorHit(AActor* Hit, AActor* Last) {}

void AKPCLEquipmentBase::OnLastActorChanged(AActor* NewLast, AActor* LastLast) {}

void AKPCLEquipmentBase::AddEquipmentActionBindings()
{

	Super::AddEquipmentActionBindings();

	if (IsValid(GetInstigatorCharacter()))
	{
		UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(GetInstigatorCharacter()->InputComponent);
		if (IsValid(EIC))
		{

		}
		else
		{
			UE_LOG(LogKPCL, Error, TEXT("No EnhancedInputComponent found on %s"), *GetInstigatorCharacter()->GetName())
		}
	}
	else
	{
		UE_LOG(LogKPCL, Error, TEXT("Invalid InstigatorCharacter"))
	}
}

UFGOutlineComponent* AKPCLEquipmentBase::GetCachedOutlineComponent() const { return mCachedOutlineComponent; }

AActor* AKPCLEquipmentBase::GetCurrentHit() const { return mCurrentActor; }

AActor* AKPCLEquipmentBase::GetLastHit() const { return mLastActor; }

void AKPCLEquipmentBase::Input_PrimaryFire(const FInputActionValue& actionValue) {}

void AKPCLEquipmentBase::Input_SecondaryFire(const FInputActionValue& actionValue) {}

void AKPCLEquipmentBase::Input_Wheel(const FInputActionValue& actionValue) {}

void AKPCLEquipmentBase::MultiCast_OnLeftClick_Implementation() {}

void AKPCLEquipmentBase::OnLeftClick() { bLeftIsClicked = true; }

void AKPCLEquipmentBase::OnLeftClickReleased() { bLeftIsClicked = false; }

void AKPCLEquipmentBase::MultiCast_OnRightClick_Implementation() {}

void AKPCLEquipmentBase::OnRightClick() { bRightIsClicked = true; }

void AKPCLEquipmentBase::OnRightClickReleased() { bRightIsClicked = false; }

void AKPCLEquipmentBase::MultiCast_OnMiddleMouseButton_Implementation() {}

void AKPCLEquipmentBase::OnMiddleMouseButton() { bMiddleMouseButtonIsClicked = true; }

void AKPCLEquipmentBase::OnMiddleMouseButtonReleased() { bMiddleMouseButtonIsClicked = false; }
