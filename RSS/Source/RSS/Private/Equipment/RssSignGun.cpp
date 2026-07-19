// Fill out your copyright notice in the Description page of Project Settings.


#include "Equipment/RssSignGun.h"

#include "BFL/KBFL_Player.h"
#include "Buildable/RSSSignRCO.h"
#include "Buildables/FGBuildableWidgetSign.h"
#include "Components/WidgetComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "EnhancedInputComponent.h"
#include "FGCharacterPlayer.h"
#include "FGFactoryClipboard.h"
#include "FGTrain.h"
#include "FGVehicle.h"
#include "Interface/RssSignInterface.h"
#include "Kismet/KismetSystemLibrary.h"


// Sets default values
ARssSignGun::ARssSignGun()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

void ARssSignGun::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// traces so mTargetActor remains valid for CustomizerLikePaste and the outline system.
	mTraceAccumulator += DeltaSeconds;
	if (mTraceAccumulator >= mTraceInterval)
	{
		mTraceAccumulator = FMath::Fmod(mTraceAccumulator, mTraceInterval);
		TraceForSign();
	}

	if (bLeftIsClicked)
	{
		CustomizerLikePaste(DeltaSeconds);
	}
}

void ARssSignGun::TraceForSign()
{
	AFGCharacterPlayer* Character = GetInstigatorCharacter();
	if (!IsValid(Character))
	{
		return;
	}

	const TArray<AActor*> IgnoredActors{this, Character};
	FHitResult Result;

	const FVector Start = Character->GetCameraComponentWorldLocation();
	const FVector End = Start + mSignGunRange * Character->GetCameraComponentForwardVector();

	// Trace for signs
	if (UKismetSystemLibrary::BoxTraceSingleForObjects(GetWorld(), Start, End, mTraceBoxHalfSize, FRotator(),
													   mTraceObjects, false, IgnoredActors, EDrawDebugTrace::None,
													   Result, false))
	{
		if (Result.IsValidBlockingHit())
		{
			mLastTargetActor = mTargetActor;
			mTargetActor = Result.GetActor();

			if (mTargetActor != mLastTargetActor)
			{
				StopLookAtActor(mLastTargetActor);

				if (IsValid(mRssDataManager) &&
					UKismetSystemLibrary::DoesImplementInterface(mTargetActor, URssSignInterface::StaticClass()))
				{
					mRssDataManager->ForceLoadSign(mTargetActor);
				}
			}

			LookAtActor(mTargetActor);
			CheckVanillaSignPreview();

			return;
		}
	}

	StopLookAtActor(mTargetActor);
	mTargetActor = nullptr;
	CheckVanillaSignPreview();
}

void ARssSignGun::CheckVanillaSignPreview()
{
	if (mTargetPreviewActor != mTargetActor)
	{
		if (IsValid(mTargetPreviewActor) && HasCopyData())
		{
			mTargetPreviewActor->UpdateSignElements(mCachedTargetData);
		}

		mTargetPreviewActor = GetTarget<AFGBuildableWidgetSign>();

		if (IsValid(mTargetPreviewActor))
		{
			if (HasCopyData())
			{
				AFGBuildableWidgetSign* Copied = GetCopiedActor<AFGBuildableWidgetSign>();
				if (GetVanillaSignDataFromTarget(mCachedCopiedData, Copied) &&
					GetVanillaSignDataFromTarget(mCachedTargetData, mTargetPreviewActor))
				{
					mTargetPreviewActor->UpdateSignElements(mCachedCopiedData);
				}
			}
		}
	}
}

void ARssSignGun::CustomizerLikePaste(float DeltaSeconds)
{
	if (CanPastePasteSign(mTargetActor))
	{
		PastePasteSign();
	}
}

void ARssSignGun::BeginPlay()
{
	mRssDataManager = ARssDataManagerSubsystem::GetRSSDataManagerSubsystem(GetWorld());

	// set display on sign gun
	SetRenderTarget();
	Super::BeginPlay();

	Clear();
	Cache();

	// cache outline
	const AFGCharacterPlayer* Char = GetInstigatorCharacter();
	if (IsValid(Char))
	{
		mCachedOutlineComponent = Char->GetOutline();
	}
}

void ARssSignGun::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	Clear();
}

void ARssSignGun::Equip(AFGCharacterPlayer* character)
{
	Super::Equip(character);
	Clear();
	Cache();
}

void ARssSignGun::UnEquip()
{
	Super::UnEquip();
	Clear();
}

void ARssSignGun::DisableEquipment()
{
	Super::DisableEquipment();
	Clear();
}

void ARssSignGun::WasEquipped_Implementation()
{
	Super::WasEquipped_Implementation();
	Clear();
	Cache();
}

void ARssSignGun::WasUnEquipped_Implementation()
{
	Super::WasUnEquipped_Implementation();
	Clear();
}

void ARssSignGun::Clear()
{
	// remove all stored things and tell the targets whats happen
	StopLookAtActor(mTargetActor);
	StopLookAtActor(mLastTargetActor);

	mTargetActor = nullptr;
	CheckVanillaSignPreview();
	mTargetPreviewActor = nullptr;
	mLastTargetActor = nullptr;
	mPastedActors.Empty();

	// Note: We intentionally do NOT clear mCachedClipboardSettings and mCopiedClipboardMappingClass
	// so that the copied data persists across equipment cycles

	bLeftIsClicked = false;
	bRightIsClicked = false;
	bMiddleMouseButtonIsClicked = false;
}

void ARssSignGun::Cache()
{
	mRssDataManager = ARssDataManagerSubsystem::GetRSSDataManagerSubsystem(GetWorld());
	const AFGCharacterPlayer* Char = GetInstigatorCharacter();
	if (IsValid(Char))
	{
		mCachedOutlineComponent = Char->GetOutline();
	}
}

void ARssSignGun::AddEquipmentActionBindings()
{
	Super::AddEquipmentActionBindings();

	if (const AFGCharacterPlayer* Character = GetInstigatorCharacter())
	{
		if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(Character->InputComponent))
		{
			// Primary Fire (Left Click) - Paste
			if (mInputActionPrimaryFire)
			{
				EIC->BindAction(mInputActionPrimaryFire, ETriggerEvent::Started, this, &ARssSignGun::Input_PrimaryFire);
				EIC->BindAction(mInputActionPrimaryFire, ETriggerEvent::Completed, this,
								&ARssSignGun::Input_PrimaryFire);
			}

			// Secondary Fire (Right Click) - Copy
			if (mInputActionSecondaryFire)
			{
				EIC->BindAction(mInputActionSecondaryFire, ETriggerEvent::Started, this,
								&ARssSignGun::Input_SecondaryFire);
				EIC->BindAction(mInputActionSecondaryFire, ETriggerEvent::Completed, this,
								&ARssSignGun::Input_SecondaryFire);
			}

			// Open Menu (Middle Mouse Button)
			if (mInputActionOpenMenu)
			{
				EIC->BindAction(mInputActionOpenMenu, ETriggerEvent::Started, this, &ARssSignGun::Input_OpenMenu);
				EIC->BindAction(mInputActionOpenMenu, ETriggerEvent::Completed, this, &ARssSignGun::Input_OpenMenu);
			}
		}
	}
}

void ARssSignGun::Input_PrimaryFire(const FInputActionValue& ActionValue)
{
	const bool bIsPressedLocal = ActionValue.Get<bool>();
	if (bIsPressedLocal)
	{
		OnLeftClick();
	}
	else
	{
		OnLeftClickReleased();
	}
}

void ARssSignGun::Input_SecondaryFire(const FInputActionValue& ActionValue)
{
	const bool bIsPressedLocal = ActionValue.Get<bool>();

	if (bIsPressedLocal)
	{
		OnRightClick();
	}
	else
	{
		OnRightClickReleased();
	}
}

void ARssSignGun::Input_OpenMenu(const FInputActionValue& ActionValue)
{
	// Fixed to mirror Input_PrimaryFire / Input_SecondaryFire pattern.
	if (ActionValue.Get<bool>())
	{
		OnMiddleMouseButton();
	}
	else
	{
		OnMiddleMouseButtonReleased();
	}
}

void ARssSignGun::OnLeftClick()
{
	// Do a paste in left
	PastePasteSign();
	bLeftIsClicked = true;
}

void ARssSignGun::OnLeftClickReleased()
{
	// Clear the pasted that next leftclick is rdy again
	mPastedActors.Empty();
	bLeftIsClicked = false;
}

void ARssSignGun::OnRightClick()
{
	// We copy the sign
	DoCopySign();
	bRightIsClicked = true;
}

void ARssSignGun::OnRightClickReleased() { bRightIsClicked = false; }

void ARssSignGun::OnMiddleMouseButton()
{
	// we want to open the Interface of a sign
	OpenSignUi();
	bMiddleMouseButtonIsClicked = true;
}

void ARssSignGun::OnMiddleMouseButtonReleased() { bMiddleMouseButtonIsClicked = false; }

bool ARssSignGun::HasCopyData() const
{
	// Check if we have clipboard data for the current target
	if (IsValid(mTargetActor) && HasClipboardDataForTarget(mTargetActor))
	{
		return true;
	}

	// Fallback to data manager for RSS sign data
	if (IsValid(mRssDataManager))
	{
		FRssSignData SignData = mRssDataManager->GetCopiedSignData();
		if (SignData.mSignTypeSize != ESignSize::RSS_InValid && SignData.mSignType != ESignType::RSS_InValid)
		{
			return true;
		}

		// Check if we have a copied actor
		if (IsValid(mRssDataManager->GetLastCopiedActor()))
		{
			return true;
		}
	}

	return false;
}

AFGPlayerState* ARssSignGun::GetPlayerState() const
{
	if (const AFGCharacterPlayer* Character = GetInstigatorCharacter())
	{
		if (AController* Controller = Character->GetController())
		{
			return Controller->GetPlayerState<AFGPlayerState>();
		}
	}
	return nullptr;
}

bool ARssSignGun::HasClipboardDataForTarget(AActor* Target) const
{
	if (!IsValid(Target))
	{
		return false;
	}

	// Check if target supports clipboard
	if (!TargetSupportsClipboard_Internal(Target))
	{
		return false;
	}

	if (!IFGFactoryClipboardInterface::Execute_CanUseFactoryClipboard(Target))
	{
		return false;
	}

	// Get the mapping class for this target
	TSubclassOf<UObject> MappingClass = IFGFactoryClipboardInterface::Execute_GetClipboardMappingClass(Target);
	if (!MappingClass)
	{
		return false;
	}

	// Check if we have a copied actor with matching clipboard class
	if (IsValid(mRssDataManager) && IsValid(mRssDataManager->GetLastCopiedActor()))
	{
		AActor* CopiedActor = mRssDataManager->GetLastCopiedActor();
		if (TargetSupportsClipboard_Internal(CopiedActor))
		{
			TSubclassOf<UObject> CopiedMappingClass =
				IFGFactoryClipboardInterface::Execute_GetClipboardMappingClass(CopiedActor);
			return CopiedMappingClass == MappingClass;
		}
	}

	return false;
}

void ARssSignGun::LookAtActor(AActor* Actor)
{
	if (UKismetSystemLibrary::DoesImplementInterface(Actor, UFGUseableInterface::StaticClass()) &&
		IsValid(mCachedOutlineComponent) && IsLocalInstigator())
	{
		bool RssSign = WeLookAtRssSign_Internal(Actor);
		bool VanillaSign = WeLookAtVanillaSign_Internal(Actor);
		if (RssSign || VanillaSign)
		{
			bool CanPaste = CanPastePasteSign(Actor);
			// show the outline or change the color
			mCachedOutlineComponent->ShowOutline(Actor, CanPaste ? mCanCopyPasteOutline : mCanNOTCopyPasteOutline);

			if (RssSign && CanPaste)
			{
				// enable preview on sign
				IRssSignInterface::Execute_SignGun_StartLookingAtSign(Actor, mRssDataManager->GetCopiedSignData());
			}
		}
	}
}

void ARssSignGun::StopLookAtActor(AActor* Actor)
{
	if (UKismetSystemLibrary::DoesImplementInterface(Actor, URssSignInterface::StaticClass()))
	{
		IRssSignInterface::Execute_SignGun_EndLookingAtSign(Actor);
	}

	if (UKismetSystemLibrary::DoesImplementInterface(Actor, UFGUseableInterface::StaticClass()) &&
		IsValid(mCachedOutlineComponent) && IsLocalInstigator())
	{
		mCachedOutlineComponent->HideOutline(Actor);
		if (UKismetSystemLibrary::DoesImplementInterface(Actor, URssSignInterface::StaticClass()))
		{
			IRssSignInterface::Execute_SignGun_EndLookingAtSign(Actor);
		}
	}
}

bool ARssSignGun::WeLookAtVanillaSign() const
{
	return GetTarget<AFGBuildableWidgetSign>() != nullptr &&
		!UKismetSystemLibrary::DoesImplementInterface(mTargetActor, URssSignInterface::StaticClass());
}

bool ARssSignGun::WeLookAtVanillaSign_Internal(AActor* Actor) const
{
	return Cast<AFGBuildableWidgetSign>(Actor) != nullptr &&
		!UKismetSystemLibrary::DoesImplementInterface(Actor, URssSignInterface::StaticClass());
}

bool ARssSignGun::WeLookAtRssSign() const
{
	return UKismetSystemLibrary::DoesImplementInterface(mTargetActor, URssSignInterface::StaticClass());
}

bool ARssSignGun::WeLookAtRssSign_Internal(AActor* Actor) const
{
	return UKismetSystemLibrary::DoesImplementInterface(Actor, URssSignInterface::StaticClass());
}

AActor* ARssSignGun::BP_GetTarget() const { return GetTarget<AActor>(); }

AActor* ARssSignGun::BP_GetLastTarget() const { return GetLastTarget<AActor>(); }

AActor* ARssSignGun::BP_GetCopiedActor() const { return GetCopiedActor<AActor>(); }

void ARssSignGun::OpenSignUi()
{
	if (!IsValid(GetInstigatorCharacter()))
	{
		return;
	}

	AFGVehicle* Vehicle = Cast<AFGVehicle>(mTargetActor);
	AFGTrain* ASTrain = Cast<AFGTrain>(mTargetActor);
	if (IsValid(Vehicle) || IsValid(ASTrain))
	{
		if (UKismetSystemLibrary::DoesImplementInterface(mTargetActor, URssSignInterface::StaticClass()))
		{
			IRssSignInterface::Execute_RequestInteractWidget(
				mTargetActor, GetInstigatorCharacter()->GetController<AFGPlayerController>());
			return;
		}
	}

	if (UKismetSystemLibrary::DoesImplementInterface(mTargetActor, UFGUseableInterface::StaticClass()))
	{
		FUseState State;
		State.UseLocation = mTargetActor->GetActorLocation();
		IFGUseableInterface::Execute_OnUse(mTargetActor, GetInstigatorCharacter(), State);
	}
}

void ARssSignGun::DoCopySign()
{
	if (!IsValid(mTargetActor))
	{
		return;
	}

	AFGPlayerState* PS = GetPlayerState();

	// Check if target supports the clipboard interface
	if (TargetSupportsClipboard_Internal(mTargetActor))
	{
		if (IFGFactoryClipboardInterface::Execute_CanUseFactoryClipboard(mTargetActor))
		{
			// Use AFGPlayerState to copy the clipboard - this handles everything
			if (PS)
			{
				PS->CopyFactoryClipboard(mTargetActor);
			}

			// Also store in data manager for our own tracking
			mRssDataManager->SetLastCopiedActor(mTargetActor);

			// For RSS signs, also store the sign data for preview
			if (WeLookAtRssSign_Internal(mTargetActor))
			{
				mRssDataManager->CopySignDataToClipboard(GetSignDataFromTarget());
				StopLookAtActor(mTargetActor);
				LookAtActor(mTargetActor);
			}
			else
			{
				// For vanilla signs, clear preview
				mTargetPreviewActor = nullptr;
			}

			OnCopied();
			return;
		}
	}

	// Fallback to legacy behavior for vanilla signs that might not fully support clipboard
	if (WeLookAtVanillaSign())
	{
		mRssDataManager->SetLastCopiedActor(mTargetActor);
		mTargetPreviewActor = nullptr;
		OnCopied();
	}
}

void ARssSignGun::PastePasteSign()
{
	if (!CanPastePasteSign(mTargetActor))
	{
		return;
	}

	// Fallback for vanilla signs using legacy method
	if (WeLookAtVanillaSign())
	{
		AFGBuildableWidgetSign* TargetSign = GetTarget<AFGBuildableWidgetSign>();

		if (!HasAuthority())
		{
			if (URSSSignRCO* RCO = URSSSignRCO::Get(GetWorld()))
			{
				OnPaste();
				mCachedTargetData = mCachedCopiedData;
				mPastedActors.Add(mTargetActor);
				RCO->RCO_Client_PasteSignData(this);
			}
			return;
		}

		AFGBuildableWidgetSign* CopiedSign = GetCopiedActor<AFGBuildableWidgetSign>();

		if (IsValid(TargetSign) && IsValid(CopiedSign))
		{
			FPrefabSignData Data;
			CopiedSign->GetSignPrefabData(Data);
			TargetSign->SetPrefabSignData(Data);
			mPastedActors.Add(mTargetActor);
			mCachedTargetData = mCachedCopiedData;
			OnPaste();
		}
	}

	AFGPlayerState* PS = GetPlayerState();

	// Use AFGPlayerState to paste - this handles the clipboard interface internally
	if (PS && TargetSupportsClipboard_Internal(mTargetActor))
	{
		if (IFGFactoryClipboardInterface::Execute_CanUseFactoryClipboard(mTargetActor))
		{
			// Use AFGPlayerState to paste the clipboard
			PS->PasteFactoryClipboard(mTargetActor);

			mPastedActors.Add(mTargetActor);

			// Update preview for RSS signs
			if (WeLookAtRssSign_Internal(mTargetActor))
			{
				StopLookAtActor(mTargetActor);
				LookAtActor(mTargetActor);
			}
			else
			{
				mCachedTargetData = mCachedCopiedData;
			}

			OnPaste();
		}
	}
}

bool ARssSignGun::CanPastePasteSign(AActor* Actor) const
{
	if (!IsValid(Actor) || mPastedActors.Contains(Actor))
	{
		return false;
	}

	// Check if we have clipboard data for this target type
	if (TargetSupportsClipboard_Internal(Actor) && HasClipboardDataForTarget(Actor))
	{
		return IFGFactoryClipboardInterface::Execute_CanUseFactoryClipboard(Actor);
	}

	// Fallback to legacy checks
	if (WeLookAtVanillaSign_Internal(Actor))
	{
		const AFGBuildableWidgetSign* TargetSign = Cast<AFGBuildableWidgetSign>(Actor);
		const AFGBuildableWidgetSign* CopiedSign = GetCopiedActor<AFGBuildableWidgetSign>();

		if (IsValid(TargetSign) && IsValid(CopiedSign))
		{
			return TargetSign->GetClass() == CopiedSign->GetClass();
		}
	}
	else if (WeLookAtRssSign_Internal(Actor) && IsValid(mRssDataManager))
	{
		FRssSignData TargetData = IRssSignInterface::Execute_SignGun_GetRealSignData(Actor);
		return AreSignDataCompatible(TargetData, mRssDataManager->GetCopiedSignData());
	}

	return false;
}

bool ARssSignGun::TargetSupportsClipboard() const { return TargetSupportsClipboard_Internal(mTargetActor); }

bool ARssSignGun::TargetSupportsClipboard_Internal(AActor* Actor) const
{
	if (!IsValid(Actor))
	{
		return false;
	}

	return UKismetSystemLibrary::DoesImplementInterface(Actor, UFGFactoryClipboardInterface::StaticClass());
}

FRssSignData ARssSignGun::GetSignDataFromTarget() const
{
	if (WeLookAtRssSign())
	{
		return IRssSignInterface::Execute_SignGun_GetRealSignData(mTargetActor);
	}
	return FRssSignData();
}

bool ARssSignGun::GetVanillaSignDataFromTarget(FPrefabSignData& PrefData, AActor* Target) const
{
	if (WeLookAtVanillaSign_Internal(Target))
	{
		FPrefabSignData Data;
		Cast<AFGBuildableWidgetSign>(Target)->GetSignPrefabData(PrefData);
		return true;
	}
	return false;
}

bool ARssSignGun::AreSignDataCompatible(const FRssSignData& A, const FRssSignData& B)
{
	if (A.mSignTypeSize == ESignSize::RSS_InValid || A.mSignType == ESignType::RSS_InValid)
	{
		return false;
	}

	if (B.mSignTypeSize == ESignSize::RSS_InValid || B.mSignType == ESignType::RSS_InValid)
	{
		return false;
	}

	return A.mSignTypeSize == B.mSignTypeSize && A.mSignType == B.mSignType;
}

void ARssSignGun::SetRenderTarget()
{
	if (UWidgetComponent* WidgetRender = Cast<UWidgetComponent>(GetComponentByClass(UWidgetComponent::StaticClass())))
	{
		WidgetRender->SetTickMode(ETickMode::Automatic);
		USkeletalMeshComponent* GunMesh =
			Cast<USkeletalMeshComponent>(GetComponentByClass(USkeletalMeshComponent::StaticClass()));
		if (GunMesh && WidgetRender->GetRenderTarget())
		{
			UMaterialInstanceDynamic* Dyn = GunMesh->CreateAndSetMaterialInstanceDynamic(2);
			if (Dyn)
			{
				Dyn->SetTextureParameterValue("SlateUI", WidgetRender->GetRenderTarget());
				return;
			}
		}
	}

	FTimerHandle TimerHandle;
	this->GetWorldTimerManager().SetTimer(TimerHandle, this, &ARssSignGun::SetRenderTarget, 0.05f, false);
}
