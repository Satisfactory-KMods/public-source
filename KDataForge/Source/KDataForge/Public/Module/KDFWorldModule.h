#pragma once

#include "CoreMinimal.h"

#include "Module/GameWorldModule.h"

#include "KDFWorldModule.generated.h"

UCLASS()
class KDATAFORGE_API UKDFWorldModule : public UGameWorldModule
{
	GENERATED_BODY()

public:
	UKDFWorldModule();

	virtual void DispatchLifecycleEvent(ELifecyclePhase Phase) override;
};
