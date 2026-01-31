#pragma once

#include "CoreMinimal.h"
#include "KBFLActorSpawnDescriptorBase.h"

#include "KBFLActorSpawnDescriptor.generated.h"

/**
 *
 */
UCLASS(Blueprintable, EditInlineNew, abstract, DefaultToInstanced)
class KBFL_API UKBFLActorSpawnDescriptor : public UKBFLActorSpawnDescriptorBase
{
	GENERATED_BODY()

public:
	virtual void ForeachLocations(TArray<AActor*>& ActorArray) override;

	virtual TArray<TSubclassOf<AActor>> GetSearchingActorClasses() override { return {mActorClass}; };

	FORCEINLINE virtual TSubclassOf<AActor> GetActorClass() override { return mActorClass; };

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Actor")
	TSubclassOf<AActor> mActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Actor")
	TArray<FTransform> mLocations;
};
