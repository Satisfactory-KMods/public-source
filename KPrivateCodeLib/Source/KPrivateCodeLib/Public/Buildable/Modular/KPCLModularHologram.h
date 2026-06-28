// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "Descriptors/KAPIModularAttachmentDescriptor.h"
#include "FGConstructDisqualifier.h"
#include "Hologram/FGFactoryHologram.h"

#include "KPCLModularHologram.generated.h"

UCLASS()
class KPRIVATECODELIB_API AKPCLModularHologram : public AFGFactoryHologram
{
	GENERATED_BODY()

public:
	AKPCLModularHologram();

	//~ Begin AFGHologram Interface
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool IsValidHitResult(const FHitResult& hitResult) const override;
	virtual void SetHologramLocationAndRotation(const FHitResult& hitResult) override;
	virtual void ConfigureComponents(AFGBuildable* inBuildable) const override;
	virtual void CheckValidPlacement() override;
	virtual AActor* GetUpgradedActor() const override;
	virtual bool TryUpgrade(const FHitResult& hitResult) override;
	virtual void Scroll(int32 delta) override;
	//~ End AFGHologram Interface

	virtual bool IsModuleAllowed(class UKPCLModularBuildingHandlerBase* Handler, AFGBuildable* TargetBuildable,
								 const FHitResult& hitResult);

	UPROPERTY(Transient, Replicated)
	TObjectPtr<AFGBuildable> mModuleMasterHit;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> TopMesh;

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> TopSkel;

	UPROPERTY(Replicated)
	TObjectPtr<AFGBuildable> mUpgradedActorRef;

	FTransform mSnapLocation;
	FTransform mNextSnapLocation;
	float mRotation = 0;

	UPROPERTY(EditDefaultsOnly, Category = "KMods")
	TSubclassOf<UKAPIModularAttachmentDescriptor> mAttachmentDescriptor;

	UPROPERTY(EditDefaultsOnly, Category = "KMods")
	bool mPreventUpgrade = false;

	UPROPERTY(EditDefaultsOnly, Category = "KMods")
	float mSnapDistance = 500.0f;

	UPROPERTY(EditDefaultsOnly, Category = "KMods")
	bool mIsStacker = true;

	UPROPERTY(EditDefaultsOnly, Category = "KMods")
	bool mCanRotate = true;

	UPROPERTY(EditDefaultsOnly, Category = "KMods|Disqualifier")
	TSubclassOf<UFGConstructDisqualifier> mMissingMasterModule = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "KMods|Disqualifier")
	TSubclassOf<UFGConstructDisqualifier> mToMuchModules = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "KMods|Disqualifier")
	TSubclassOf<UFGConstructDisqualifier> mModuleIsNotAllowed = nullptr;
};
