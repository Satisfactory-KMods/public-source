#include "BFL/KBFL_ConfigTools.h"

#include "Configuration/Properties/ConfigPropertyClass.h"

DECLARE_LOG_CATEGORY_EXTERN(KBFLConfigToolLog, Log, All)

DEFINE_LOG_CATEGORY(KBFLConfigToolLog)

UConfigPropertySection* UKBFL_ConfigTools::GetPropertySection(TSubclassOf<UModConfiguration> Config,
															  UObject* WorldContext)
{
	return Cast<UConfigPropertySection>(
		URuntimeBlueprintFunctionLibrary::GetModConfigurationPropertyByClass(Config, WorldContext));
}

UConfigProperty* UKBFL_ConfigTools::GetConfigPropertyByKey(TSubclassOf<UModConfiguration> Config, FString Key,
														   UObject* WorldContext)
{
	if (UConfigPropertySection* Configuration = GetPropertySection(Config, WorldContext))
	{
		return URuntimeBlueprintFunctionLibrary::Conv_ConfigPropertySectionToConfigProperty(Configuration, Key);
	}
	return nullptr;
}

void UKBFL_ConfigTools::SaveProperty(TSubclassOf<UModConfiguration> Config, FString Key, UObject* WorldContext)
{
	if (UConfigPropertySection* Section = GetPropertySection(Config, WorldContext))
	{
		Section->MarkDirty();
		if (UConfigProperty* Property = GetPropertyByKey<UConfigProperty>(Config, Key, WorldContext))
		{
			Property->MarkDirty();
		}
	}
}

bool UKBFL_ConfigTools::GetBoolFromConfig(TSubclassOf<UModConfiguration> Config, FString Key, UObject* WorldContext)
{
	return URuntimeBlueprintFunctionLibrary::Conv_ConfigPropertyToBool(
		GetConfigPropertyByKey(Config, Key, WorldContext));
}

void UKBFL_ConfigTools::SetBoolInConfig(TSubclassOf<UModConfiguration> Config, FString Key, bool Value,
										UObject* WorldContext)
{
	if (UConfigPropertyBool* Property = GetPropertyByKey<UConfigPropertyBool>(Config, Key, WorldContext))
	{
		Property->Value = Value;
		SaveProperty(Config, Key, WorldContext);
	}
}

float UKBFL_ConfigTools::GetFloatFromConfig(TSubclassOf<UModConfiguration> Config, FString Key, UObject* WorldContext)
{
	return URuntimeBlueprintFunctionLibrary::Conv_ConfigPropertyToFloat(
		GetConfigPropertyByKey(Config, Key, WorldContext));
}

void UKBFL_ConfigTools::SetFloatInConfig(TSubclassOf<UModConfiguration> Config, FString Key, float Value,
										 UObject* WorldContext)
{
	if (UConfigPropertyFloat* Property = GetPropertyByKey<UConfigPropertyFloat>(Config, Key, WorldContext))
	{
		Property->Value = Value;
		SaveProperty(Config, Key, WorldContext);
	}
}

UClass* UKBFL_ConfigTools::GetClassFromConfig(TSubclassOf<UModConfiguration> Config, FString Key, UObject* WorldContext)
{
	if (UConfigPropertyClass* Property = GetPropertyByKey<UConfigPropertyClass>(Config, Key, WorldContext))
	{
		return Property->BaseClass;
	}
	return nullptr;
}

void UKBFL_ConfigTools::SetClassInConfig(TSubclassOf<UModConfiguration> Config, FString Key, UClass* Value,
										 UObject* WorldContext)
{
	if (UConfigPropertyClass* Property = GetPropertyByKey<UConfigPropertyClass>(Config, Key, WorldContext))
	{
		Property->Value = Value;
		SaveProperty(Config, Key, WorldContext);
	}
}

int UKBFL_ConfigTools::GetIntFromConfig(TSubclassOf<UModConfiguration> Config, FString Key, UObject* WorldContext)
{
	return URuntimeBlueprintFunctionLibrary::Conv_ConfigPropertyToInteger(
		GetConfigPropertyByKey(Config, Key, WorldContext));
}

void UKBFL_ConfigTools::SetIntInConfig(TSubclassOf<UModConfiguration> Config, FString Key, int Value,
									   UObject* WorldContext)
{
	if (UConfigPropertyInteger* Property = GetPropertyByKey<UConfigPropertyInteger>(Config, Key, WorldContext))
	{
		Property->Value = Value;
		SaveProperty(Config, Key, WorldContext);
	}
}

FName UKBFL_ConfigTools::GetNameFromConfig(TSubclassOf<UModConfiguration> Config, FString Key, UObject* WorldContext)
{
	return URuntimeBlueprintFunctionLibrary::Conv_ConfigPropertyToName(
		GetConfigPropertyByKey(Config, Key, WorldContext));
}

void UKBFL_ConfigTools::SetNameInConfig(TSubclassOf<UModConfiguration> Config, FString Key, FName Value,
										UObject* WorldContext)
{
	if (UConfigPropertyString* Property = GetPropertyByKey<UConfigPropertyString>(Config, Key, WorldContext))
	{
		Property->Value = Value.ToString();
		SaveProperty(Config, Key, WorldContext);
	}
}

FString UKBFL_ConfigTools::GetStringFromConfig(TSubclassOf<UModConfiguration> Config, FString Key,
											   UObject* WorldContext)
{
	return URuntimeBlueprintFunctionLibrary::Conv_ConfigPropertyToString(
		GetConfigPropertyByKey(Config, Key, WorldContext));
}

void UKBFL_ConfigTools::SetStringInConfig(TSubclassOf<UModConfiguration> Config, FString Key, FString Value,
										  UObject* WorldContext)
{
	if (UConfigPropertyString* Property = GetPropertyByKey<UConfigPropertyString>(Config, Key, WorldContext))
	{
		Property->Value = Value;
		SaveProperty(Config, Key, WorldContext);
	}
}

FText UKBFL_ConfigTools::GetTextFromConfig(TSubclassOf<UModConfiguration> Config, FString Key, UObject* WorldContext)
{
	return URuntimeBlueprintFunctionLibrary::Conv_ConfigPropertyToText(
		GetConfigPropertyByKey(Config, Key, WorldContext));
}

void UKBFL_ConfigTools::SetTextInConfig(TSubclassOf<UModConfiguration> Config, FString Key, FText Value,
										UObject* WorldContext)
{
	if (UConfigPropertyString* Property = GetPropertyByKey<UConfigPropertyString>(Config, Key, WorldContext))
	{
		Property->Value = Value.ToString();
		SaveProperty(Config, Key, WorldContext);
	}
}
