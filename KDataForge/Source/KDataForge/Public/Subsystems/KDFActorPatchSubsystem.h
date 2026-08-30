#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "KDFActorPatchSubsystem.generated.h"

class AActor;

UCLASS()
class KDATAFORGE_API UKDFActorPatchSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	int32 ApplyPatchesToActor(AActor* Actor);

private:
	void OnActorSpawned(AActor* Actor);

	FDelegateHandle mSpawnHandle;
};
