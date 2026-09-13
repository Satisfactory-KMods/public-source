// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Module/GameInstanceModule.h"

#include "KDFGameInstanceModule.generated.h"

UCLASS()
class KDATAFORGE_API UKDFGameInstanceModule : public UGameInstanceModule
{
	GENERATED_BODY()

public:
	UKDFGameInstanceModule();

	virtual void DispatchLifecycleEvent(ELifecyclePhase Phase) override;
};
