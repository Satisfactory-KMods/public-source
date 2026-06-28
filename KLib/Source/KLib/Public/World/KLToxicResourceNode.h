// ILikeBanas

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
	AKLToxicResourceNode();

	//~ Begin AFGResourceNode Interface
	virtual bool CanBecomeOccupied() const override;
	virtual bool CanPlaceResourceExtractor() const override;
	virtual FText GetLookAtDecription_Implementation(class AFGCharacterPlayer* byCharacter,
													 const FUseState& state) const override;
	virtual bool IsOccupied() const override;
	//~ End AFGResourceNode Interface

	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	//~ End AActor Interface

	TSubclassOf<AFGGasPillar> GetRandomGasPillarClass() const;
	void EnsureGasCloud();
	void InitializePillar();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KMods|Toxic Resource Node")
	TSubclassOf<AFGGasPillarCloud> mGasCloudClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KMods|Toxic Resource Node")
	TArray<TSubclassOf<AFGGasPillar>> mGasPillarClasses;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "KMods|Toxic Resource Node")
	bool bGasPillarSpawned = false;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "KMods|Toxic Resource Node")
	TWeakObjectPtr<AFGGasPillar> mGasPillarRef;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "KMods|Toxic Resource Node")
	TWeakObjectPtr<AFGGasPillarCloud> mGasCloudRef;
};
