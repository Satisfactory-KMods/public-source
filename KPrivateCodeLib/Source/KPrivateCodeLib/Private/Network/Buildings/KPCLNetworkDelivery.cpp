// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#include "Network/Buildings/KPCLNetworkDelivery.h"

#include "Components/KPCLMovableColoredInstanceMeshProxy.h"
#include "Components/SceneComponent.h"
#include "Network/Buildings/KPCLNetworkCore.h"
#include "Network/KPCLNetworkAsyncHelpers.h"
#include "Network/KPCLNetworkDeliveryHologram.h"
#include "Subsystem/KPCLDeliveryTaskSubsystem.h"
#include "Subsystem/KPCLUnlockSubsystem.h"

namespace
{
	const FItemAmount* FindItemAmount(const TArray<FItemAmount>& Amounts, TSubclassOf<UFGItemDescriptor> Item)
	{
		return Amounts.FindByPredicate([Item](const FItemAmount& Candidate) { return Candidate.ItemClass == Item; });
	}

	float CalculateSharedOrbitRadius(float InnerRadius, float CollisionPadding, float MaximumMeshBoundsRadius)
	{
		return FMath::Max(FMath::Max(0.f, InnerRadius),
						  FMath::Max(0.f, MaximumMeshBoundsRadius) + FMath::Max(1.f, CollisionPadding));
	}
}

AKPCLNetworkDelivery::AKPCLNetworkDelivery()
{
	PrimaryActorTick.bCanEverTick = true;
	mDeliveryOrbitCenter = CreateDefaultSubobject<USceneComponent>(TEXT("DeliveryOrbitCenter"));
	mDeliveryOrbitCenter->SetupAttachment(RootComponent);
	mDeliveryOrbitCenter->SetRelativeTransform(FTransform::Identity);
	mDeliveryOrbitCenter->SetMobility(EComponentMobility::Static);
	mHologramClass = AKPCLNetworkDeliveryHologram::StaticClass();
	mProductionHandle.SetNewTime(1.f);
}

UFGFactoryClipboardSettings* AKPCLNetworkDelivery::CopySettings_Implementation()
{
	UKPCLNetworkDeliveryClipboardSettings* Settings = NewObject<UKPCLNetworkDeliveryClipboardSettings>();
	Settings->mNetworkId = mNetworkId;
	Settings->mConnectedNetworkId = mConnectedNetworkId;
	Settings->bIsPaused = IsProductionPaused();
	return Settings;
}

bool AKPCLNetworkDelivery::PasteSettings_Implementation(UFGFactoryClipboardSettings* FactoryClipboard,
														AFGPlayerController* Player)
{
	UKPCLNetworkDeliveryClipboardSettings* Settings = Cast<UKPCLNetworkDeliveryClipboardSettings>(FactoryClipboard);
	if (!Settings || !Super::PasteSettings_Implementation(FactoryClipboard, Player))
	{
		return false;
	}

	return true;
}

void AKPCLNetworkDelivery::BeginPlay()
{
	Super::BeginPlay();
	BindCoreSettings(GetFaxitCore());
	InitializeDeliveryOrbitMeshes();
}

void AKPCLNetworkDelivery::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindCoreSettings();
	mDeliveryOrbitMeshes.Reset();
	Super::EndPlay(EndPlayReason);
}

void AKPCLNetworkDelivery::Tick(float Dt)
{
	Super::Tick(Dt);
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	if (!GetIsSignificant())
	{
		return;
	}

	UpdateDeliveryOrbitMeshes(Dt);
}

void AKPCLNetworkDelivery::EndProductionTime()
{
	Super::EndProductionTime();

	if (!HasAuthority() || bDeliveryDispatchPending.Exchange(true))
	{
		return;
	}

	RunOnGameThreadIfValid(this,
						   [](AKPCLNetworkDelivery* Self)
						   {
							   Self->ProcessDeliveryOnGameThread();
							   Self->bDeliveryDispatchPending.Store(false);
						   });
}

void AKPCLNetworkDelivery::OnNetworkAdded_Internal(AKPCLNetworkCore* Core)
{
	Super::OnNetworkAdded_Internal(Core);
	BindCoreSettings(Core);
}

void AKPCLNetworkDelivery::OnNetworkDestoryed_Internal()
{
	UnbindCoreSettings();
	Super::OnNetworkDestoryed_Internal();
}

void AKPCLNetworkDelivery::BindCoreSettings(AKPCLNetworkCore* Core)
{
	if (!IsValid(Core) || mBoundSettingsCore.Get() == Core)
	{
		return;
	}

	UnbindCoreSettings();
	mBoundSettingsCore = Core;
	Core->mOnMinimumStoragePercentageChanged.AddUniqueDynamic(
		this, &AKPCLNetworkDelivery::HandleCoreMinimumStoragePercentageChanged);
	HandleCoreMinimumStoragePercentageChanged(Core->GetMinimumStoragePercentage());
}

void AKPCLNetworkDelivery::UnbindCoreSettings()
{
	if (AKPCLNetworkCore* Core = mBoundSettingsCore.Get())
	{
		Core->mOnMinimumStoragePercentageChanged.RemoveDynamic(
			this, &AKPCLNetworkDelivery::HandleCoreMinimumStoragePercentageChanged);
	}
	mBoundSettingsCore.Reset();
}

void AKPCLNetworkDelivery::HandleCoreMinimumStoragePercentageChanged(float MinimumStoragePercentage)
{
	mOnMinimumStoragePercentageChanged.Broadcast(MinimumStoragePercentage);
	OnUiRequireUpdate();
}

float AKPCLNetworkDelivery::GetMinimumStoragePercentage() const
{
	const AKPCLNetworkCore* Core = GetFaxitCoreConst();
	return IsValid(Core) ? Core->GetMinimumStoragePercentage() : 0.f;
}

void AKPCLNetworkDelivery::SetMinimumStoragePercentage(float NewPercentage)
{
	if (AKPCLNetworkCore* Core = GetFaxitCore())
	{
		Core->SetMinimumStoragePercentage(NewPercentage);
	}
}

void AKPCLNetworkDelivery::ProcessDeliveryOnGameThread()
{
	check(IsInGameThread());
	if (!HasAuthority() || !CanProduce_Implementation())
	{
		return;
	}

	AKPCLNetworkCore* Core = GetFaxitCore();
	AKPCLDeliveryTaskSubsystem* DeliverySubsystem = AKPCLDeliveryTaskSubsystem::Get(this);
	const AKPCLUnlockSubsystem* UnlockSubsystem = AKPCLUnlockSubsystem::Get(this);
	if (!IsValid(Core) || !IsValid(DeliverySubsystem) || !IsValid(UnlockSubsystem) ||
		!UnlockSubsystem->GetIsDeliveryTaskSystemUnlocked())
	{
		return;
	}

	const TArray<FItemAmount> NeededAmounts = DeliverySubsystem->GetCurrentTaskNeededAmounts();
	if (NeededAmounts.IsEmpty())
	{
		return;
	}

	TArray<FItemAmount> WithdrawnAmounts;
	WithdrawnAmounts.Reserve(NeededAmounts.Num());
	for (const FItemAmount& NeededAmount : NeededAmounts)
	{
		const int32 Withdrawn = Core->TryToGrabItemAmountAboveReserve(NeededAmount.ItemClass, NeededAmount.Amount);
		if (Withdrawn > 0)
		{
			WithdrawnAmounts.Emplace(NeededAmount.ItemClass, Withdrawn);
		}
	}

	if (WithdrawnAmounts.IsEmpty())
	{
		return;
	}

	TArray<FItemAmount> DeliveredAmounts;
	DeliverySubsystem->TryToDeliverAmounts(WithdrawnAmounts, DeliveredAmounts);

	for (const FItemAmount& WithdrawnAmount : WithdrawnAmounts)
	{
		const FItemAmount* DeliveredAmount = FindItemAmount(DeliveredAmounts, WithdrawnAmount.ItemClass);
		const int32 ConsumedAmount = DeliveredAmount ? DeliveredAmount->Amount : 0;
		const int32 RefundAmount = FMath::Max(0, WithdrawnAmount.Amount - ConsumedAmount);
		if (RefundAmount > 0)
		{
			const int32 Refunded = Core->TryToStoreItemAmount(WithdrawnAmount.ItemClass, RefundAmount);
			ensureMsgf(Refunded == RefundAmount, TEXT("NetworkDelivery failed to refund %d of %s to Core %s"),
					   RefundAmount, *GetNameSafe(WithdrawnAmount.ItemClass), *GetNameSafe(Core));
		}
	}

	if (DeliveredAmounts.IsEmpty())
	{
		return;
	}

	for (const FItemAmount& DeliveredAmount : DeliveredAmounts)
	{
		AddDownloadToStats(DeliveredAmount.ItemClass, DeliveredAmount.Amount);
	}
	mOnItemsDelivered.Broadcast(DeliveredAmounts);
}

void AKPCLNetworkDelivery::InitializeDeliveryOrbitMeshes()
{
	mDeliveryOrbitMeshes.Reset();
	if (GetNetMode() == NM_DedicatedServer || !IsValid(mDeliveryOrbitCenter))
	{
		return;
	}

	TArray<USceneComponent*> OrbitChildren;
	mDeliveryOrbitCenter->GetChildrenComponents(true, OrbitChildren);
	TArray<UKPCLMovableColoredInstanceMeshProxy*> MeshProxies;
	for (USceneComponent* OrbitChild : OrbitChildren)
	{
		UKPCLMovableColoredInstanceMeshProxy* MeshProxy = Cast<UKPCLMovableColoredInstanceMeshProxy>(OrbitChild);
		if (IsValid(MeshProxy) && MeshProxy->GetOwner() == this)
		{
			MeshProxies.Add(MeshProxy);
		}
	}

	const int32 MeshCount = MeshProxies.Num();
	if (MeshCount == 0)
	{
		return;
	}

	const FTransform CenterTransform = mDeliveryOrbitCenter->GetComponentTransform();
	const FQuat CenterRotationInverse = CenterTransform.GetRotation().Inverse();
	const float AngularSpeedRadians = FMath::DegreesToRadians(FMath::Max(0.1f, mOrbitAngularSpeed));
	const float MaximumTilt = FMath::Clamp(mOrbitMaximumPlaneTilt, 0.f, 89.f);
	float MaximumBoundsRadius = 0.f;
	for (UKPCLMovableColoredInstanceMeshProxy* MeshProxy : MeshProxies)
	{
		MeshProxy->SetMobility(EComponentMobility::Movable);
		MeshProxy->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		MeshProxy->SetGenerateOverlapEvents(false);
		MeshProxy->SetCanEverAffectNavigation(false);
		MeshProxy->UpdateBounds();
		MaximumBoundsRadius = FMath::Max(MaximumBoundsRadius, MeshProxy->Bounds.SphereRadius);
	}

	const float SharedRadius =
		CalculateSharedOrbitRadius(mOrbitInnerRadius, mOrbitCollisionPadding, MaximumBoundsRadius);
	for (int32 MeshIndex = 0; MeshIndex < MeshCount; ++MeshIndex)
	{
		UKPCLMovableColoredInstanceMeshProxy* MeshProxy = MeshProxies[MeshIndex];

		FKPCLDeliveryOrbitMesh& OrbitMesh = mDeliveryOrbitMeshes.AddDefaulted_GetRef();
		OrbitMesh.mMeshProxy = MeshProxy;
		OrbitMesh.mRadius = SharedRadius;
		OrbitMesh.mAngleRadians = 2.f * UE_PI * static_cast<float>(MeshIndex) / static_cast<float>(MeshCount);
		OrbitMesh.mAngularSpeedRadians = MeshIndex % 2 == 0 ? AngularSpeedRadians : -AngularSpeedRadians;
		const float PlaneAlpha =
			MeshCount > 1 ? static_cast<float>(MeshIndex) / static_cast<float>(MeshCount - 1) : 0.5f;
		const float PlaneTilt = FMath::Lerp(-MaximumTilt, MaximumTilt, PlaneAlpha);
		const float PlaneYaw = FMath::Fmod(static_cast<float>(MeshIndex) * 137.507764f, 360.f);
		OrbitMesh.mOrbitPlaneRotation = FRotator(PlaneTilt, PlaneYaw, 0.f).Quaternion();
		OrbitMesh.mMeshRotationRelativeToCenter = CenterRotationInverse * MeshProxy->GetComponentQuat();
		OrbitMesh.mWorldScale = MeshProxy->GetComponentScale();
	}

	UpdateDeliveryOrbitMeshes(0.f);
}

void AKPCLNetworkDelivery::UpdateDeliveryOrbitMeshes(float Dt)
{
	if (!IsValid(mDeliveryOrbitCenter))
	{
		return;
	}

	const FTransform CenterTransform = mDeliveryOrbitCenter->GetComponentTransform();
	for (FKPCLDeliveryOrbitMesh& OrbitMesh : mDeliveryOrbitMeshes)
	{
		UKPCLMovableColoredInstanceMeshProxy* MeshProxy = OrbitMesh.mMeshProxy;
		if (!IsValid(MeshProxy))
		{
			continue;
		}

		OrbitMesh.mAngleRadians =
			FMath::Fmod(OrbitMesh.mAngleRadians + OrbitMesh.mAngularSpeedRadians * Dt, 2.f * UE_PI);
		const FVector OrbitOffset(OrbitMesh.mRadius * FMath::Cos(OrbitMesh.mAngleRadians),
								  OrbitMesh.mRadius * FMath::Sin(OrbitMesh.mAngleRadians), 0.f);
		const FVector WorldLocation =
			CenterTransform.TransformPositionNoScale(OrbitMesh.mOrbitPlaneRotation.RotateVector(OrbitOffset));
		const FQuat WorldRotation = CenterTransform.GetRotation() * OrbitMesh.mMeshRotationRelativeToCenter;
		MeshProxy->MoveToWorldTransform(FTransform(WorldRotation, WorldLocation, OrbitMesh.mWorldScale));
	}
}
