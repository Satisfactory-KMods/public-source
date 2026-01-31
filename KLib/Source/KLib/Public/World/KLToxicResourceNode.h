//

#pragma once

#include "CoreMinimal.h"
#include "FGGasPillar.h"
#include "Resources/FGResourceNode.h"

#include "KLToxicResourceNode.generated.h"

UCLASS()
class KLIB_API AKLToxicResourceNode : public AFGResourceNode
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AKLToxicResourceNode();

	virtual FText GetLookAtDecription_Implementation(class AFGCharacterPlayer* byCharacter,
													 const FUseState& state) const override;

	virtual void BeginPlay() override;

	void InitializePillar();
	void EnsureGasCloud();
	TSubclassOf<AFGGasPillar> GetRandomGasPillarClass() const;

	virtual bool IsOccupied() const override;
	virtual bool CanBecomeOccupied() const override;
	virtual bool CanPlaceResourceExtractor() const override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KMods|Toxic Resource Node")
	TArray<TSubclassOf<AFGGasPillar>> mGasPillarClasses;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KMods|Toxic Resource Node")
	TSubclassOf<AFGGasPillarCloud> mGasCloudClass;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "KMods|Toxic Resource Node")
	bool bGasPillarSpawned = false;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "KMods|Toxic Resource Node")
	TWeakObjectPtr<AFGGasPillar> mGasPillarRef;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "KMods|Toxic Resource Node")
	TWeakObjectPtr<AFGGasPillarCloud> mGasCloudRef;
};
