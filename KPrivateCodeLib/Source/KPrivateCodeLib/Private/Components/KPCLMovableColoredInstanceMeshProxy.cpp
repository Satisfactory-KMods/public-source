// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#include "Components/KPCLMovableColoredInstanceMeshProxy.h"

#include "FGBuildableSubsystem.h"
#include "FGColoredInstanceManager.h"
#include "Materials/MaterialInstanceDynamic.h"

UKPCLMovableColoredInstanceMeshProxy::UKPCLMovableColoredInstanceMeshProxy()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetMobility(EComponentMobility::Movable);
	SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetGenerateOverlapEvents(false);
	SetCanEverAffectNavigation(false);
	SetCastShadow(false);
	mBlockBuildableCustomizationUpdates = true;
}

bool UKPCLMovableColoredInstanceMeshProxy::ShouldSave_Implementation() const { return false; }

void UKPCLMovableColoredInstanceMeshProxy::MoveToWorldTransform(const FTransform& NewTransform)
{
	check(IsInGameThread());
	SetWorldTransform(NewTransform, false, nullptr, ETeleportType::TeleportPhysics);
	UpdateBounds();

	if (mInstanceHandle.IsInstanced())
	{
		if (AFGBuildableSubsystem* BuildableSubsystem = AFGBuildableSubsystem::Get(this))
		{
			if (UFGColoredInstanceManager* InstanceManager = BuildableSubsystem->GetColoredInstanceManager(this))
			{
				InstanceManager->UpdateTransformForInstance(NewTransform, mInstanceHandle.GetHandleID());
			}
		}
	}
}

void UKPCLMovableColoredInstanceMeshProxy::OverrideAllMaterials(UMaterialInterface* Material)
{
	if (!IsValid(Material))
	{
		return;
	}

	check(IsInGameThread());
	const bool bWasInstanced = mInstanceHandle.IsInstanced();
	if (bWasInstanced)
	{
		SetInstanced(false);
	}

	const int32 MaterialCount = GetNumMaterials();
	for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
	{
		SetMaterial(MaterialIndex, Material);
	}

	if (bWasInstanced)
	{
		SetInstanced(true);
	}
}

void UKPCLMovableColoredInstanceMeshProxy::ApplyMaterialColor(UMaterialInterface* OverrideMaterial, FName ParameterName,
															  FLinearColor Color)
{
	mDynamicMaterials.Reset();
	const int32 MaterialCount = GetNumMaterials();
	for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
	{
		UMaterialInterface* SourceMaterial = IsValid(OverrideMaterial) ? OverrideMaterial : GetMaterial(MaterialIndex);
		if (!IsValid(SourceMaterial))
		{
			continue;
		}

		UMaterialInstanceDynamic* DynamicMaterial = UMaterialInstanceDynamic::Create(SourceMaterial, this);
		if (!IsValid(DynamicMaterial))
		{
			continue;
		}

		DynamicMaterial->SetVectorParameterValue(ParameterName, Color);
		mDynamicMaterials.Add(DynamicMaterial);
		SetMaterial(MaterialIndex, DynamicMaterial);
	}
}

void UKPCLMovableColoredInstanceMeshProxy::ApplyMaterialColors(UMaterialInterface* OverrideMaterial,
															   FName FirstParameterName, FLinearColor FirstColor,
															   FName SecondParameterName, FLinearColor SecondColor)
{
	check(IsInGameThread());
	const bool bWasInstanced = mInstanceHandle.IsInstanced();
	if (bWasInstanced)
	{
		SetInstanced(false);
	}

	mDynamicMaterials.Reset();
	const int32 MaterialCount = GetNumMaterials();
	for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
	{
		UMaterialInterface* SourceMaterial = IsValid(OverrideMaterial) ? OverrideMaterial : GetMaterial(MaterialIndex);
		if (!IsValid(SourceMaterial))
		{
			continue;
		}

		UMaterialInstanceDynamic* DynamicMaterial = UMaterialInstanceDynamic::Create(SourceMaterial, this);
		if (!IsValid(DynamicMaterial))
		{
			continue;
		}

		DynamicMaterial->SetVectorParameterValue(FirstParameterName, FirstColor);
		DynamicMaterial->SetVectorParameterValue(SecondParameterName, SecondColor);
		mDynamicMaterials.Add(DynamicMaterial);
		SetMaterial(MaterialIndex, DynamicMaterial);
	}

	if (bWasInstanced)
	{
		SetInstanced(true);
	}
}
