// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "Components/KPCLColoredStaticMesh.h"
#include "CoreMinimal.h"

#include "KPCLMovableColoredInstanceMeshProxy.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;

UCLASS(BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class KPRIVATECODELIB_API UKPCLMovableColoredInstanceMeshProxy : public UKPCLColoredStaticMesh
{
	GENERATED_BODY()

public:
	UKPCLMovableColoredInstanceMeshProxy();
	virtual bool ShouldSave_Implementation() const override;

	UFUNCTION(BlueprintCallable, Category = "KMods|MovableColoredMesh")
	void MoveToWorldTransform(const FTransform& NewTransform);

	UFUNCTION(BlueprintCallable, Category = "KMods|MovableColoredMesh")
	void OverrideAllMaterials(UMaterialInterface* Material);

	UFUNCTION(BlueprintCallable, Category = "KMods|MovableColoredMesh")
	void ApplyMaterialColor(UMaterialInterface* OverrideMaterial, FName ParameterName, FLinearColor Color);

	UFUNCTION(BlueprintCallable, Category = "KMods|MovableColoredMesh")
	void ApplyMaterialColors(UMaterialInterface* OverrideMaterial, FName FirstParameterName, FLinearColor FirstColor,
							 FName SecondParameterName, FLinearColor SecondColor);

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> mDynamicMaterials;
};
