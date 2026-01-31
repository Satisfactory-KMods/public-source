#pragma once

#include "Configuration/ModConfiguration.h"
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Kismet/RuntimeBlueprintFunctionLibrary.h"

#include "KBFL_ConfigTools.generated.h"

/**
 *
 */
UCLASS()
class KBFL_API UKBFL_ConfigTools : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// native helper
	static UConfigPropertySection* GetPropertySection(TSubclassOf<UModConfiguration> Config, UObject* WorldContext);

	template <class T>
	static T* GetPropertyByKey(TSubclassOf<UModConfiguration> Config, FString Key, UObject* WorldContext);

	static UConfigProperty* GetConfigPropertyByKey(TSubclassOf<UModConfiguration> Config, FString Key,
												   UObject* WorldContext);

	static void SaveProperty(TSubclassOf<UModConfiguration> Config, FString Key, UObject* WorldContext);

	/** get an Bool from given Config
	 * return false if the Config is invalid
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "KMods|ConfigHelper", meta = (WorldContext = "WorldContext"))
	static bool GetBoolFromConfig(TSubclassOf<UModConfiguration> Config, FString Key, UObject* WorldContext);

	/** Write an Bool from given Config */
	UFUNCTION(BlueprintCallable, Category = "KMods|ConfigHelper", meta = (WorldContext = "WorldContext"))
	static void SetBoolInConfig(TSubclassOf<UModConfiguration> Config, FString Key, bool Value, UObject* WorldContext);

	/** get an Float from given Config
	 * return 0.0f if the Config is invalid
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "KMods|ConfigHelper", meta = (WorldContext = "WorldContext"))
	static float GetFloatFromConfig(TSubclassOf<UModConfiguration> Config, FString Key, UObject* WorldContext);

	/** Write an Bool from given Config */
	UFUNCTION(BlueprintCallable, Category = "KMods|ConfigHelper", meta = (WorldContext = "WorldContext"))
	static void SetFloatInConfig(TSubclassOf<UModConfiguration> Config, FString Key, float Value,
								 UObject* WorldContext);

	/** get an UClass* from given Config
	 * return nullptr if the Config is invalid
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "KMods|ConfigHelper", meta = (WorldContext = "WorldContext"))
	static UClass* GetClassFromConfig(TSubclassOf<UModConfiguration> Config, FString Key, UObject* WorldContext);

	/** Write an UClass* from given Config */
	UFUNCTION(BlueprintCallable, Category = "KMods|ConfigHelper", meta = (WorldContext = "WorldContext"))
	static void SetClassInConfig(TSubclassOf<UModConfiguration> Config, FString Key, UClass* Value,
								 UObject* WorldContext);

	/** get an Int from given Config
	 * return 0 if the Config is invalid
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "KMods|ConfigHelper", meta = (WorldContext = "WorldContext"))
	static int GetIntFromConfig(TSubclassOf<UModConfiguration> Config, FString Key, UObject* WorldContext);

	/** Write an Int from given Config */
	UFUNCTION(BlueprintCallable, Category = "KMods|ConfigHelper", meta = (WorldContext = "WorldContext"))
	static void SetIntInConfig(TSubclassOf<UModConfiguration> Config, FString Key, int Value, UObject* WorldContext);

	/** get an Name from given Config
	 * return nothing if the Config is invalid
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "KMods|ConfigHelper", meta = (WorldContext = "WorldContext"))
	static FName GetNameFromConfig(TSubclassOf<UModConfiguration> Config, FString Key, UObject* WorldContext);

	/** Write an FName from given Config */
	UFUNCTION(BlueprintCallable, Category = "KMods|ConfigHelper", meta = (WorldContext = "WorldContext"))
	static void SetNameInConfig(TSubclassOf<UModConfiguration> Config, FString Key, FName Value, UObject* WorldContext);

	/** get an String from given Config
	 * return nothing if the Config is invalid
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "KMods|ConfigHelper", meta = (WorldContext = "WorldContext"))
	static FString GetStringFromConfig(TSubclassOf<UModConfiguration> Config, FString Key, UObject* WorldContext);

	/** Write an String from given Config */
	UFUNCTION(BlueprintCallable, Category = "KMods|ConfigHelper", meta = (WorldContext = "WorldContext"))
	static void SetStringInConfig(TSubclassOf<UModConfiguration> Config, FString Key, FString Value,
								  UObject* WorldContext);

	/** get an Text from given Config
	 * return nothing if the Config is invalid
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "KMods|ConfigHelper", meta = (WorldContext = "WorldContext"))
	static FText GetTextFromConfig(TSubclassOf<UModConfiguration> Config, FString Key, UObject* WorldContext);

	/** Write an Text from given Config */
	UFUNCTION(BlueprintCallable, Category = "KMods|ConfigHelper", meta = (WorldContext = "WorldContext"))
	static void SetTextInConfig(TSubclassOf<UModConfiguration> Config, FString Key, FText Value, UObject* WorldContext);
};

template <class T>
T* UKBFL_ConfigTools::GetPropertyByKey(TSubclassOf<UModConfiguration> Config, FString Key, UObject* WorldContext)
{
	if (UConfigPropertySection* Configuration = GetPropertySection(Config, WorldContext))
	{
		return Cast<T>(
			URuntimeBlueprintFunctionLibrary::Conv_ConfigPropertySectionToConfigProperty(Configuration, Key));
	}
	return nullptr;
}
