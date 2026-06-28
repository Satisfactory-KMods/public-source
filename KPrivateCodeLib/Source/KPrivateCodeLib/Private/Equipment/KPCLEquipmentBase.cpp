// Fill out your copyright notice in the Description page of Project Settings.

#include "Equipment/KPCLEquipmentBase.h"

#include "BlueprintFunctionLib/KPCLBlueprintFunctionLib.h"
#include "EnhancedInputComponent.h"
#include "FGCharacterPlayer.h"
#include "KPrivateCodeLibModule.h"

// Sets default values
AKPCLEquipmentBase::AKPCLEquipmentBase()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	// mMappingContext = LoadObject<UInputMappingContext>(nullptr, TEXT(''),
	// TEXT("/KPrivateCodeLib/InputAction/IMC_KPCL_Equipment.IMC_KPCL_Equipment_C"));
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

	// cache outline
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

	// Trace for signs
	/*if (UKismetSystemLibrary::BoxTraceSingleForObjects(GetWorld(), Start, End, mTraceBoxHalfSize, FRotator(),
													   mTraceObjects, false, IgnoredActors, EDrawDebugTrace::None,
													   Result, false))
	{
		UKPCLBlueprintFunctionLib::ResolveHitResult(GetWorld(), Result, Result);
		if (Result.IsValidBlockingHit())
		{
			if (mCurrentActor != Result.GetActor())
			{
				OnLastActorChanged(mCurrentActor, mLastActor);
				mLastActor = mCurrentActor;
				mCurrentActor = Result.GetActor();
				OnNewActorHit(mCurrentActor, mLastActor);
			}
		}
	}
	else
	{
		if (mCurrentActor != nullptr && mLastActor != nullptr)
		{
			OnLastActorChanged(mCurrentActor, mLastActor);
			mLastActor = mCurrentActor;
			mCurrentActor = nullptr;
			OnNewActorHit(mCurrentActor, mLastActor);
		}
	}*/
}

void AKPCLEquipmentBase::OnNewActorHit(AActor* Hit, AActor* Last) {}

void AKPCLEquipmentBase::OnLastActorChanged(AActor* NewLast, AActor* LastLast) {}

void AKPCLEquipmentBase::AddEquipmentActionBindings()
{
	// bind modded keybinds and make the consumed (for example MMB will copy otherwise the building to open the build
	// gun)
	Super::AddEquipmentActionBindings();

	if (IsValid(GetInstigatorCharacter()))
	{
		UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(GetInstigatorCharacter()->InputComponent);
		if (IsValid(EIC))
		{
			// SetEquipmentBindings(EIC);
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

/*void AKPCLEquipmentBase::SetEquipmentBindings(UEnhancedInputComponent* EIC) {
	const UInputMappingContext* Mapping = GetMappingContext();
	if(IsValid(Mapping)) {
		EIC->BindAction(Mapping->GetMappings()[0].Action, ETriggerEvent::Started, this, "OnLeftClick");
		EIC->BindAction(Mapping->GetMappings()[0].Action, ETriggerEvent::Completed, this, "OnLeftClickReleased");
		EIC->BindAction(Mapping->GetMappings()[1].Action, ETriggerEvent::Started, this, "OnRightClick");
		EIC->BindAction(Mapping->GetMappings()[1].Action, ETriggerEvent::Completed, this, "OnRightClickReleased");
		EIC->BindAction(Mapping->GetMappings()[2].Action, ETriggerEvent::Started, this, "OnMiddleMouseButton");
		EIC->BindAction(Mapping->GetMappings()[2].Action, ETriggerEvent::Completed, this,
"OnMiddleMouseButtonReleased"); } else { UE_LOG(LogKPCL, Error, TEXT("Invalid MappingContext"))
	}
}*/

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
