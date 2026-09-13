// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "Configuration/ModConfiguration.h"
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Kismet/RuntimeBlueprintFunctionLibrary.h"

#include "KBFL_ConfigTools.generated.h"

UCLASS()
class KBFL_API UKBFL_ConfigTools : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	static UConfigPropertySection* GetPropertySection(TSubclassOf<UModConfiguration> Config, UObject* WorldContext);

	template <class T>
	static T* GetPropertyByKey(TSubclassOf<UModConfiguration> Config, FString Key, UObject* WorldContext);

	static UConfigProperty* GetConfigPropertyByKey(TSubclassOf<UModConfiguration> Config, FString Key,
												   UObject* WorldContext);

	static void SaveProperty(TSubclassOf<UModConfiguration> Config, FString Key, UObject* WorldContext);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "KMods|ConfigHelper", meta = (WorldContext = "WorldContext"))
	static bool GetBoolFromConfig(TSubclassOf<UModConfiguration> Config, FString Key, UObject* WorldContext);

	UFUNCTION(BlueprintCallable, Category = "KMods|ConfigHelper", meta = (WorldContext = "WorldContext"))
	static void SetBoolInConfig(TSubclassOf<UModConfiguration> Config, FString Key, bool Value, UObject* WorldContext);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "KMods|ConfigHelper", meta = (WorldContext = "WorldContext"))
	static float GetFloatFromConfig(TSubclassOf<UModConfiguration> Config, FString Key, UObject* WorldContext);

	UFUNCTION(BlueprintCallable, Category = "KMods|ConfigHelper", meta = (WorldContext = "WorldContext"))
	static void SetFloatInConfig(TSubclassOf<UModConfiguration> Config, FString Key, float Value,
								 UObject* WorldContext);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "KMods|ConfigHelper", meta = (WorldContext = "WorldContext"))
	static UClass* GetClassFromConfig(TSubclassOf<UModConfiguration> Config, FString Key, UObject* WorldContext);

	UFUNCTION(BlueprintCallable, Category = "KMods|ConfigHelper", meta = (WorldContext = "WorldContext"))
	static void SetClassInConfig(TSubclassOf<UModConfiguration> Config, FString Key, UClass* Value,
								 UObject* WorldContext);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "KMods|ConfigHelper", meta = (WorldContext = "WorldContext"))
	static int GetIntFromConfig(TSubclassOf<UModConfiguration> Config, FString Key, UObject* WorldContext);

	UFUNCTION(BlueprintCallable, Category = "KMods|ConfigHelper", meta = (WorldContext = "WorldContext"))
	static void SetIntInConfig(TSubclassOf<UModConfiguration> Config, FString Key, int Value, UObject* WorldContext);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "KMods|ConfigHelper", meta = (WorldContext = "WorldContext"))
	static FName GetNameFromConfig(TSubclassOf<UModConfiguration> Config, FString Key, UObject* WorldContext);

	UFUNCTION(BlueprintCallable, Category = "KMods|ConfigHelper", meta = (WorldContext = "WorldContext"))
	static void SetNameInConfig(TSubclassOf<UModConfiguration> Config, FString Key, FName Value, UObject* WorldContext);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "KMods|ConfigHelper", meta = (WorldContext = "WorldContext"))
	static FString GetStringFromConfig(TSubclassOf<UModConfiguration> Config, FString Key, UObject* WorldContext);

	UFUNCTION(BlueprintCallable, Category = "KMods|ConfigHelper", meta = (WorldContext = "WorldContext"))
	static void SetStringInConfig(TSubclassOf<UModConfiguration> Config, FString Key, FString Value,
								  UObject* WorldContext);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "KMods|ConfigHelper", meta = (WorldContext = "WorldContext"))
	static FText GetTextFromConfig(TSubclassOf<UModConfiguration> Config, FString Key, UObject* WorldContext);

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
