//

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"

#include "KBFLDeveloperSettings.generated.h"

/**
 * Project settings for KBFL (Project Settings -> Plugins -> KMods (KBFL)).
 * Stored in DefaultGame.ini.
 */
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "KMods (KBFL)"))
class KBFL_API UKBFLDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const UKBFLDeveloperSettings* Get() { return GetDefault<UKBFLDeveloperSettings>(); }

	virtual FName GetCategoryName() const override { return FName("Plugins"); }

	/**
	 * Mute the CDO overwrite logs (LogKBFLCDOOverwrite + ContentCDOHelperSubsystem).
	 * Applying many CDOs spends a lot of time formatting log strings; muting gives a large speedup.
	 */
	UPROPERTY(EditAnywhere, config, Category = "CDO")
	bool bMuteCDOLogs = true;
};
