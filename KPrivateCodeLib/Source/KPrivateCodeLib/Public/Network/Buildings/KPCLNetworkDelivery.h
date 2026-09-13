// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "Network/KPCLNetworkBuildingBase.h"
#include "Resources/FGItemDescriptor.h"

#include "KPCLNetworkDelivery.generated.h"

class UKPCLMovableColoredInstanceMeshProxy;
class USceneComponent;

UCLASS()
class KPRIVATECODELIB_API UKPCLNetworkDeliveryClipboardSettings : public UKPCLFaxitBasicClipboardSettings
{
	GENERATED_BODY()

public:
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKPCLOnNetworkDeliverySettingChanged, float, MinimumStoragePercentage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKPCLOnNetworkDeliveryItemsDelivered, const TArray<FItemAmount>&,
											DeliveredAmounts);

USTRUCT()
struct FKPCLDeliveryOrbitMesh
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TObjectPtr<UKPCLMovableColoredInstanceMeshProxy> mMeshProxy;

	float mRadius = 0.f;
	float mAngleRadians = 0.f;
	float mAngularSpeedRadians = 0.f;
	FQuat mOrbitPlaneRotation = FQuat::Identity;
	FQuat mMeshRotationRelativeToCenter = FQuat::Identity;
	FVector mWorldScale = FVector::OneVector;
};

UCLASS()
class KPRIVATECODELIB_API AKPCLNetworkDelivery : public AKPCLNetworkBuildingBase
{
	GENERATED_BODY()

public:
	AKPCLNetworkDelivery();

	virtual UFGFactoryClipboardSettings* CopySettings_Implementation() override;
	virtual bool PasteSettings_Implementation(UFGFactoryClipboardSettings* FactoryClipboard,
											  AFGPlayerController* Player) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float Dt) override;
	virtual void OnNetworkAdded_Internal(AKPCLNetworkCore* Core) override;
	virtual void OnNetworkDestoryed_Internal() override;

	UFUNCTION(BlueprintPure, Category = "KMods|DeliveryTask")
	float GetMinimumStoragePercentage() const;

	UFUNCTION(BlueprintCallable, Category = "KMods|DeliveryTask")
	void SetMinimumStoragePercentage(float NewPercentage);

	UPROPERTY(BlueprintAssignable, Category = "KMods|DeliveryTask|Events")
	FKPCLOnNetworkDeliverySettingChanged mOnMinimumStoragePercentageChanged;

	UPROPERTY(BlueprintAssignable, Category = "KMods|DeliveryTask|Events")
	FKPCLOnNetworkDeliveryItemsDelivered mOnItemsDelivered;

	UFUNCTION(BlueprintImplementableEvent, Category = "KMods|DeliveryTask|Events")
	void OnUiRequireUpdate();

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KMods|DeliveryTask|Orbit")
	TObjectPtr<USceneComponent> mDeliveryOrbitCenter;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|DeliveryTask|Orbit", meta = (ClampMin = "0.0"))
	float mOrbitInnerRadius = 150.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|DeliveryTask|Orbit", meta = (ClampMin = "1.0"))
	float mOrbitCollisionPadding = 25.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|DeliveryTask|Orbit", meta = (ClampMin = "0.1"))
	float mOrbitAngularSpeed = 45.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|DeliveryTask|Orbit",
			  meta = (ClampMin = "0.0", ClampMax = "89.0"))
	float mOrbitMaximumPlaneTilt = 70.f;

private:

	virtual void EndProductionTime() override;

	void ProcessDeliveryOnGameThread();
	void InitializeDeliveryOrbitMeshes();
	void UpdateDeliveryOrbitMeshes(float Dt);
	void BindCoreSettings(AKPCLNetworkCore* Core);
	void UnbindCoreSettings();

	UFUNCTION()
	void HandleCoreMinimumStoragePercentageChanged(float MinimumStoragePercentage);

	TAtomic<bool> bDeliveryDispatchPending{false};

	UPROPERTY(Transient)
	TArray<FKPCLDeliveryOrbitMesh> mDeliveryOrbitMeshes;

	TWeakObjectPtr<AKPCLNetworkCore> mBoundSettingsCore;
};
