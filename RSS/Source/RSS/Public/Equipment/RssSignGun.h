// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "Buildables/FGBuildable.h"
#include "Buildables/FGBuildableWidgetSign.h"
#include "Equipment/FGEquipment.h"
#include "FGFactoryClipboard.h"
#include "FGPlayerController.h"
#include "FGPlayerState.h"
#include "FGSignTypes.h"
#include "InputActionValue.h"

#include "Subsystem/RSSDataManagerSubsystem.h"

#include "RssSignGun.generated.h"

// Sign Gun Log Category
DECLARE_LOG_CATEGORY_CLASS(LogRssSignGun, Log, All);

class UInputAction;

UCLASS()
class RSS_API ARssSignGun : public AFGEquipment
{
	GENERATED_BODY()

public:
	ARssSignGun();

protected:
	//~ Begin AFGEquipment Interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void Equip(AFGCharacterPlayer* character) override;
	virtual void UnEquip() override;
	virtual void DisableEquipment() override;
	virtual void WasEquipped_Implementation() override;
	virtual void WasUnEquipped_Implementation() override;
	virtual void AddEquipmentActionBindings() override;
	//~ End AFGEquipment Interface

	/**
	 * Trace for the Sign in view so we can handle the logic
	 */
	void TraceForSign();

	/**
	 * Try to preview the Vanilla Signs!
	 */
	void CheckVanillaSignPreview();

	/**
	 * Click and move over the signs to paste the data on them!
	 */
	void CustomizerLikePaste(float DeltaSeconds);

	void Clear();
	void Cache();

	/**
	 * Check if the current Target is a Vanilla Sign
	 */
	UFUNCTION(BlueprintPure, Category = "Sign Gun")
	bool WeLookAtVanillaSign() const;
	bool WeLookAtVanillaSign_Internal(AActor* Actor) const;

	/**
	 * Check if the current Target is a RSS Sign
	 */
	UFUNCTION(BlueprintPure, Category = "Sign Gun")
	bool WeLookAtRssSign() const;
	bool WeLookAtRssSign_Internal(AActor* Actor) const;

	/**
	 * Check if the target supports the Factory Clipboard Interface
	 */
	UFUNCTION(BlueprintPure, Category = "Sign Gun")
	bool TargetSupportsClipboard() const;
	bool TargetSupportsClipboard_Internal(AActor* Actor) const;

	UFUNCTION(BlueprintPure, Category = "Sign Gun", DisplayName = "GetTarget")
	AActor* BP_GetTarget() const;

	UFUNCTION(BlueprintPure, Category = "Sign Gun", DisplayName = "GetLastTarget")
	AActor* BP_GetLastTarget() const;

	UFUNCTION(BlueprintPure, Category = "Sign Gun", DisplayName = "GetCopiedActor")
	AActor* BP_GetCopiedActor() const;

	/**
	 * Called when ever we copy a sign
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Sign Gun")
	void OnCopied();

	/**
	 * Called when ever we paste a sign
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Sign Gun")
	void OnPaste();

	/**
	 * Called when paste failed (incompatible types)
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Sign Gun")
	void OnPasteFailed();

	/**
	 * We want to open the Sign UI in larger ranged with the sign gun
	 */
	void OpenSignUi();

	/**
	 * Copy settings from sign using AFGPlayerState::CopyFactoryClipboard
	 */
	void DoCopySign();

	/**
	 * Paste settings to sign using AFGPlayerState::PasteFactoryClipboard
	 */
	void PastePasteSign();

	/**
	 * Check if we can paste to the target sign
	 */
	bool CanPastePasteSign(AActor* Actor) const;

	FRssSignData GetSignDataFromTarget() const;
	bool GetVanillaSignDataFromTarget(FPrefabSignData& PrefData, AActor* Target) const;
	static bool AreSignDataCompatible(const FRssSignData& A, const FRssSignData& B);

	UFUNCTION(BlueprintPure, Category = "Sign Gun")
	bool HasCopyData() const;

	bool HasClipboardDataForTarget(AActor* Target) const;
	AFGPlayerState* GetPlayerState() const;

	template <class T>
	T* GetTarget() const;

	template <class T>
	T* GetLastTarget() const;

	template <class T>
	T* GetCopiedActor() const;

	friend class URSSSignRCO;

private:
	/** Enhanced Input Action handlers */
	void Input_PrimaryFire(const FInputActionValue& ActionValue);
	void Input_SecondaryFire(const FInputActionValue& ActionValue);
	void Input_OpenMenu(const FInputActionValue& ActionValue);

	void OnLeftClick();
	void OnLeftClickReleased();

	void OnRightClick();
	void OnRightClickReleased();

	void OnMiddleMouseButton();
	void OnMiddleMouseButtonReleased();

	void LookAtActor(AActor* Actor);
	void StopLookAtActor(AActor* Actor);
	void SetRenderTarget();

	// Cached Vanilla Information from Copied
	FPrefabSignData mCachedCopiedData;

	// Cached Vanilla Information From Target
	FPrefabSignData mCachedTargetData;

	UPROPERTY()
	TObjectPtr<AFGBuildableWidgetSign> mTargetPreviewActor = nullptr;

	UPROPERTY()
	TObjectPtr<UFGOutlineComponent> mCachedOutlineComponent;

	UPROPERTY()
	TObjectPtr<AActor> mLastTargetActor = nullptr;

	UPROPERTY()
	TObjectPtr<AActor> mTargetActor = nullptr;

	UPROPERTY()
	TSet<TObjectPtr<AActor>> mPastedActors;

	UPROPERTY()
	TObjectPtr<ARssDataManagerSubsystem> mRssDataManager;

	float mTraceAccumulator = 0.f;
	static constexpr float mTraceInterval = 0.07f;

	UPROPERTY(EditDefaultsOnly, Category = "RSS")
	FVector mTraceBoxHalfSize = FVector(10.f);

	UPROPERTY(EditDefaultsOnly, Category = "RSS")
	TArray<TEnumAsByte<EObjectTypeQuery>> mTraceObjects;

	UPROPERTY(EditDefaultsOnly, Category = "RSS")
	float mSignGunRange = 15000.f;

	UPROPERTY(EditDefaultsOnly, Category = "RSS")
	EOutlineColor mCanCopyPasteOutline = EOutlineColor::OC_USABLE;

	UPROPERTY(EditDefaultsOnly, Category = "RSS")
	EOutlineColor mCanNOTCopyPasteOutline = EOutlineColor::OC_RED;

	UPROPERTY(EditDefaultsOnly, Category = "RSS|Input")
	TObjectPtr<UInputAction> mInputActionPrimaryFire;

	UPROPERTY(EditDefaultsOnly, Category = "RSS|Input")
	TObjectPtr<UInputAction> mInputActionSecondaryFire;

	UPROPERTY(EditDefaultsOnly, Category = "RSS|Input")
	TObjectPtr<UInputAction> mInputActionOpenMenu;

	bool bLeftIsClicked = false;
	bool bRightIsClicked = false;
	bool bMiddleMouseButtonIsClicked = false;
	bool bIsPressed = false;
};

template <class T>
T* ARssSignGun::GetTarget() const
{
	return Cast<T>(mTargetActor);
}

template <class T>
T* ARssSignGun::GetLastTarget() const
{
	return Cast<T>(mLastTargetActor);
}

template <class T>
T* ARssSignGun::GetCopiedActor() const
{
	if (IsValid(mRssDataManager))
	{
		return Cast<T>(mRssDataManager->GetLastCopiedActor());
	}
	return nullptr;
}
