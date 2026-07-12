// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "BFL/KBFL_ConfigTools.h"
#include "Buildables/FGBuildable.h"
#include "Buildables/FGBuildableSignBase.h"
#include "FGPowerConnectionComponent.h"
#include "FGRemoteCallObject.h"
#include "FGSignTypes.h"
#include "Hologram/FGStandaloneSignHologram.h"

#include "EnumStruc/RssEnum.h"
#include "EnumStruc/RssStruc.h"
#include "Interface/RssSignInterface.h"
#include "RSSBuildableSign.h"
#include "Subsystem/RSSImageSubsystem.h"
#include "Widget/RssWidgetRenderComponent.h"

#include "RSSBuildableSignVanilla.generated.h"

UCLASS()
class ARSSBuildableSignVanilla : public AFGBuildableSignBase, public IRssSignInterface, public IFGSignificanceInterface
{
	GENERATED_BODY()

public:
	ARSSBuildableSignVanilla();

	//~ Begin AFGBuildableSignBase Interface
	// virtual void Factory_Tick(float dt) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual bool CanUseFactoryClipboard_Implementation() override { return true; }

	virtual UFGFactoryClipboardSettings* CopySettings_Implementation() override
	{
		URSSSignClipboardSettings* Clipboard = NewObject<URSSSignClipboardSettings>();
		Clipboard->mSignData = mSignData;
		return Clipboard;
	};

	virtual bool PasteSettings_Implementation(UFGFactoryClipboardSettings* factoryClipboard,
											  class AFGPlayerController* player) override
	{
		if (URSSSignClipboardSettings* Clipboard = Cast<URSSSignClipboardSettings>(factoryClipboard))
		{
			if (mSignData.mSignTypeSize != Clipboard->mSignData.mSignTypeSize ||
				mSignData.mSignType != Clipboard->mSignData.mSignType)
			{
				return false;
			}

			Execute_PasteSignData(this, Clipboard->mSignData);
			return true;
		}
		return false;
	};

	virtual TSubclassOf<UObject> GetClipboardMappingClass_Implementation() override
	{
		return URSSSignClipboardSettings::StaticClass();
	};

	virtual void StartIsLookedAt_Implementation(AFGCharacterPlayer* byCharacter, const FUseState& state) override
	{
		Super::StartIsLookedAt_Implementation(byCharacter, state);
		if (ARssDataManagerSubsystem* DataManager = ARssDataManagerSubsystem::GetRSSDataManagerSubsystem(GetWorld()))
		{
			DataManager->ForceLoadSign(this);
		}
	};
	//~ End AFGBuildableSignBase Interface

	//~ Begin IFGSignificanceInterface Interface
	virtual void GainedSignificance_Implementation() override;
	virtual void LostSignificance_Implementation() override;

	virtual float GetSignificanceRange_Implementation() const override
	{
		ARssDataManagerSubsystem* DataManager = ARssDataManagerSubsystem::GetRSSDataManagerSubsystem(GetWorld());
		return DataManager ? DataManager->GetSignRenderingDistance() * mDistanceMultiplier : 0.f;
	}
	//~ End IFGSignificanceInterface Interface

	//~ Begin IRssSignInterface Interface
	virtual FRssUiData GetSignUiIData_Implementation() const override { return mSignUiData; };
	virtual TArray<struct FRssMeshInfo> GetRenderMeshInfos_Implementation() override;
	virtual TArray<struct FRssMeshInfo> GetScreenMeshInfos_Implementation() override;
	virtual UActorComponent* GetRenderComponent_Implementation() override;
	virtual bool IsBuildingSignificance_Implementation() override { return mIsSignificance; }

	virtual TSubclassOf<URssWidgetRenderComponent> GetWidgetRenderComponentClass_Implementation() override
	{
		return mWidgetRenderClass;
	}

	virtual void UpdateSign_Implementation() override;
	virtual void UpdateSignData_Implementation(FRssSignData Data) override;
	virtual void ApplySignData_Implementation(FRssSignData Data) override;
	virtual void UpdateSignDisplayWidget_Implementation() override;
	virtual void UpdateSignToCustomImage_Implementation(FRssSignRequestData Request) override;
	virtual void PasteSignData_Implementation(FRssSignData Data) override;
	virtual void RequestCustomSign_Implementation(FRssSignRequestData Request, bool ComesFromServer = false) override;

	virtual void SignGun_EndLookingAtSign_Implementation() override
	{
		mCachedSignData = FRssSignData();
		bSignGunIsLookingAt = false;
		Execute_UpdateSign(this);
	};

	virtual void SignGun_StartLookingAtSign_Implementation(FRssSignData TempSignData) override
	{
		if (!bSignGunIsLookingAt)
		{
			mCachedSignData = TempSignData;
			bSignGunIsLookingAt = true;
			Execute_UpdateSign(this);
		}
	};

	virtual FRssSignData GetSignData_Implementation() const override
	{
		return bSignGunIsLookingAt ? mCachedSignData : mSignData;
	}

	virtual FRssSignData SignGun_GetRealSignData_Implementation() override { return mSignData; };
	//~ End IRssSignInterface Interface

	UFUNCTION(BlueprintCallable, Category = "RSS")
	void SetSignData(FRssSignData Data);

	UFUNCTION(BlueprintCallable, Category = "RSS")
	void ValidateCustom();

	/** Same as ValidateCustom(), but retries next-tick until ARSSImageSubsystem exists in the world */
	void ValidateCustomDeferred();

	/** Release any cached image references this sign is using */
	void ReleaseCacheReferences();

	UFUNCTION(BlueprintCallable, NetMulticast, Reliable)
	virtual void RequestCustomSignMulticast(FRssSignRequestData Request);

	UFUNCTION()
	void OnRep_SignDataDirty();

	void SetIfDirty(bool Dirty)
	{
		if (Dirty && HasAuthority())
		{
			ApplyDirtyState();
		}
	}

	void ApplyDirtyState()
	{
		if (IsInGameThread())
		{
			Execute_ApplySignData(this, mSignData);
			return;
		}

		FRssSignData DataCopy = mSignData;
		TWeakObjectPtr<AActor> WeakThis(this);
		FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(FSimpleDelegateGraphTask::FDelegate::CreateLambda(
																 [WeakThis, DataCopy]()
																 {
																	 if (AActor* Actor = WeakThis.Get())
																	 {
																		 Execute_ApplySignData(Actor, DataCopy);
																	 }
																 }),
															 TStatId(), nullptr, ENamedThreads::GameThread);
	}

	bool bSignGunIsLookingAt = false;
	FRssSignData mCachedSignData = FRssSignData();

	UPROPERTY(EditDefaultsOnly, Category = "RSS|Config")
	float mDistanceMultiplier = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RSS|Rendering")
	TSubclassOf<URssWidgetRenderComponent> mWidgetRenderClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, SaveGame, ReplicatedUsing = OnRep_SignDataDirty,
			  Category = "RSS|Sign")
	FRssSignData mSignData = FRssSignData();

	UPROPERTY(ReplicatedUsing = OnRep_SignDataDirty)
	int32 SignDataDirty = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RSS|UI")
	FRssUiData mSignUiData = FRssUiData();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RSS|Rendering")
	int32 mScreenMaterialIndex = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RSS|Rendering")
	float mSignificanceRange = 5000;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RSS|Rendering")
	float mRenderedToleranz = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RSS|Subsystem")
	TSubclassOf<ARSSImageSubsystem> mSubsystemClass;

	UPROPERTY(BlueprintReadOnly, Category = "RSS|Subsystem")
	TObjectPtr<ARSSImageSubsystem> mSubsystem;

	UPROPERTY(BlueprintReadOnly, Category = "RSS|Rendering")
	bool mIsSignificance = true;

	UPROPERTY()
	TObjectPtr<URssWidgetRenderComponent> mScreenRender;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	TObjectPtr<class UStaticMeshComponent> mScreenMesh;
};
