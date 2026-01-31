// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FGConstructDisqualifier.h"
#include "Descriptors/KAPIModularAttachmentDescriptor.h"
#include "Hologram/FGFactoryHologram.h"
#include "KPCLModularHologram.generated.h"

UCLASS()
class KPRIVATECODELIB_API AKPCLModularHologram : public AFGFactoryHologram
{
	GENERATED_BODY()

public:
	AKPCLModularHologram();
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual bool IsValidHitResult(const FHitResult& hitResult) const override;
	virtual void SetHologramLocationAndRotation(const FHitResult& hitResult) override;
	virtual void ConfigureComponents(AFGBuildable* inBuildable) const override;
	virtual void CheckValidPlacement() override;

	virtual bool IsModuleAllowed(class UKPCLModularBuildingHandlerBase* Handler, AFGBuildable* TargetBuildable,
	                             const FHitResult& hitResult);

	virtual AActor* GetUpgradedActor() const override;
	virtual bool TryUpgrade(const FHitResult& hitResult) override;

	virtual void Scroll(int32 delta) override;

	UPROPERTY(Transient, Replicated)
	AFGBuildable* mModuleMasterHit;

	UPROPERTY(Transient)
	UStaticMeshComponent* TopMesh;

	UPROPERTY(Transient)
	USkeletalMeshComponent* TopSkel;

	UPROPERTY(Replicated)
	AFGBuildable* mUpgradedActorRef;

	FTransform mLastHitTransform;
	FTransform mSnapLocation;
	FTransform mNextSnapLocation;

	UPROPERTY(EditDefaultsOnly, Category="KMods")
	TSubclassOf<UKAPIModularAttachmentDescriptor> mAttachmentDescriptor;

	UPROPERTY(EditDefaultsOnly, Category="KMods")
	bool mPreventUpgrade = false;

	UPROPERTY(EditDefaultsOnly, Category="KMods")
	float mSnapDistance = 500.0f;
	float mRotation = 0;

	UPROPERTY(EditDefaultsOnly, Category="KMods")
	bool mIsStacker = true;

	UPROPERTY(EditDefaultsOnly, Category="KMods")
	bool mCanRotate = true;

	UPROPERTY(EditDefaultsOnly, Category="KMods|Disqualifier")
	TSubclassOf<UFGConstructDisqualifier> mMissingMasterModule = nullptr;

	UPROPERTY(EditDefaultsOnly, Category="KMods|Disqualifier")
	TSubclassOf<UFGConstructDisqualifier> mToMuchModules = nullptr;

	UPROPERTY(EditDefaultsOnly, Category="KMods|Disqualifier")
	TSubclassOf<UFGConstructDisqualifier> mModuleIsNotAllowed = nullptr;
};
