// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Buildings/KPCLNetworkCore.h"
#include "Hologram/FGWireHologram.h"
#include "KPCLNetworkCableHologram.generated.h"

UCLASS()
class KPRIVATECODELIB_API AKPCLNetworkCableHologram : public AFGWireHologram
{
	GENERATED_BODY()

public:
	AKPCLNetworkCableHologram();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void SetHologramLocationAndRotation(const FHitResult& hitResult) override;
	virtual void BeginPlay() override;
	virtual bool TryUpgrade(const FHitResult& hitResult) override;
	int32 GetConnectionToSet() const;
	bool IsConnectedToChild() const;

private:
	UPROPERTY()
	UStaticMeshComponent* mConnectionMesh;

	UPROPERTY(EditDefaultsOnly, Category="KMods|Attachment")
	TSubclassOf<UFGRecipe> mAttachmentRecipe;

	UPROPERTY(EditDefaultsOnly, Category = "KMods|Attachment")
	TArray<TSubclassOf<AFGBuildable>> mDisabledBuildings;
};
