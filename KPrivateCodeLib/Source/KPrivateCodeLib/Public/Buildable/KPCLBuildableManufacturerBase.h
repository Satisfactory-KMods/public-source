// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Buildable/KPCLProducerBase.h"
#include "Buildables/FGBuildableManufacturer.h"
#include "Structures/KPCLFunctionalStructure.h"
#include "KPCLBuildableManufacturerBase.generated.h"

UCLASS()
class KPRIVATECODELIB_API AKPCLBuildableManufacturerBase : public AFGBuildableManufacturer
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AKPCLBuildableManufacturerBase();

	// START: AActor
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual bool ShouldSave_Implementation() const override;

	virtual void BeginPlay() override;

	// END: AActor

protected:
	virtual void OnBuildEffectFinished() override;

	virtual void OnBuildEffectActorFinished() override;

	virtual void ReadyForVisuelUpdate();

	// START: AudioConfig
	virtual void InitAudioConfig();

	UFUNCTION(BlueprintImplementableEvent)
	void OnAudioConfigChanged();

	virtual void OnAudioConfigChanged_Native();

	UPROPERTY(EditDefaultsOnly, Category="KMods|Config")
	FKPCLModConfigHelper_Float mAudioConfig;
	TArray<FKPCLAudioComponent> mAudioComponents;
	// END: AudioConfig

	virtual void ReApplyColorForIndex(int32 Idx, const FFactoryCustomizationData& customizationData);

	virtual void ApplyCustomizationData_Native(const FFactoryCustomizationData& customizationData) override;

	virtual void
	SetCustomizationData_Native(const FFactoryCustomizationData& customizationData, bool skipCombine) override;

	virtual void InitMeshOverwriteInformation();

	void ApplyMeshOverwriteInformation(int32 Idx);

	void ApplyMeshInformation(FKPCLMeshOverwriteInformation Information);

	virtual bool ShouldOverwriteIndexHandle(int32 Idx, FKPCLMeshOverwriteInformation& Information);

public:
	UFUNCTION(BlueprintCallable, Category="KMods")
	bool AIO_OverwriteInstanceData(UStaticMesh* Mesh, int32 Idx);

	UFUNCTION(BlueprintCallable, Category="KMods")
	bool AIO_OverwriteInstanceData_Transform(UStaticMesh* Mesh, FTransform NewRelativTransform, int32 Idx);

	UFUNCTION(BlueprintCallable, Category="KMods")
	bool AIO_UpdateCustomFloat(int32 FloatIndex, float Data, int32 InstanceIdx, bool MarkDirty = true);

	UFUNCTION(BlueprintCallable, Category="KMods")
	bool AIO_UpdateCustomFloatAsColor(int32 StartFloatIndex, FLinearColor Data, int32 InstanceIdx,
	                                  bool MarkDirty = true);

	UFUNCTION(BlueprintCallable, Category="KMods")
	bool AIO_SetInstanceHidden(int32 InstanceIdx, bool IsHidden);

	UFUNCTION(BlueprintCallable, Category="KMods")
	bool AIO_SetInstanceWorldTransform(int32 InstanceIdx, FTransform Transform);

protected:
	TMap<int32, TMap<int32, float>> mCachedCustomData;
	TMap<int32, FTransform> mCachedTransforms;

	UPROPERTY(EditDefaultsOnly, Category="KMods|Mesh")
	TArray<FKPCLMeshOverwriteInformation> mDefaultMeshOverwriteInformations;

	UPROPERTY(Replicated, SaveGame)
	TArray<FKPCLMeshOverwriteInformation> mMeshOverwriteInformations;
};
