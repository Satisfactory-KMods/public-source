// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"

#include "KBFLDeveloperSettings.generated.h"

UCLASS(config = Game, defaultconfig, meta = (DisplayName = "KMods (KBFL)"))
class KBFL_API UKBFLDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const UKBFLDeveloperSettings* Get() { return GetDefault<UKBFLDeveloperSettings>(); }

	virtual FName GetCategoryName() const override { return FName("Plugins"); }

	UPROPERTY(EditAnywhere, config, Category = "CDO")
	bool bMuteCDOLogs = true;
};
