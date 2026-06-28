// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "Modules/ModuleInterface.h"

// RSS Log Category
DECLARE_LOG_CATEGORY_EXTERN(LogRSS, Log, All);

#undef LOCTEXT_CPP_STRING_TABLE
#define LOCTEXT_CPP_STRING_TABLE "RSS/ST_RSS_CPP"

/**
 * Macro to get a localized FText from string table using LOCTABLE
 * @param Key - The key in the string table
 *
 * Usage:
 *   FText Text = FI18N_TEXT("MyKey");
 */
#define FI18N_TEXT(Key) LOCTABLE(LOCTEXT_CPP_STRING_TABLE, Key)

/**
 * Macro to get a localized FString from string table with format arguments support
 * Uses LOCTABLE internally
 * @param Key - The key in the string table (uses LOCTEXT_CPP_STRING_TABLE as table ID)
 * @param ... - Optional format arguments for FString::Format
 *
 * Usage examples:
 *   FString Text = FI18N_FORMAT("MyKey");
 *   FString Text = FI18N_FORMAT("MyKey", Arg1, Arg2);
 */
#define FI18N_FORMAT(Key, ...) FI18N_FORMAT_IMPL(LOCTEXT_CPP_STRING_TABLE, Key, ##__VA_ARGS__)

#define FI18N_FORMAT_IMPL(TableId, Key, ...)                                                                           \
	(                                                                                                                  \
		[&]() -> FString                                                                                               \
		{                                                                                                              \
			FText LocalizedText = LOCTABLE(TableId, Key);                                                              \
			FString BaseString = LocalizedText.ToString();                                                             \
			TArray<FStringFormatArg> FormatArgs = {__VA_ARGS__};                                                       \
			if (FormatArgs.Num() > 0)                                                                                  \
			{                                                                                                          \
				return FString::Format(*BaseString, FormatArgs);                                                       \
			}                                                                                                          \
			return BaseString;                                                                                         \
		}())

/**
 * Macro to get localized FText using NSLOCTEXT (for code-defined strings, not string tables)
 * @param Key - The key for this text
 * @param SourceString - The source/default string
 *
 * Usage:
 *   FText Text = FI18N_NS("MyKey", "Default Text");
 */
#define FI18N_NS(Key, SourceString) NSLOCTEXT(LOCTEXT_CPP_STRING_TABLE, Key, SourceString)

/**
 * Macro to get formatted FString using NSLOCTEXT with format arguments
 * @param Key - The key for this text
 * @param SourceString - The source/default string with {0}, {1}, etc. placeholders
 * @param ... - Format arguments
 *
 * Usage:
 *   FString Text = FI18N_NS_FORMAT("WelcomeKey", "Welcome {0}!", PlayerName);
 */
#define FI18N_NS_FORMAT(Key, SourceString, ...)                                                                        \
	(                                                                                                                  \
		[&]() -> FString                                                                                               \
		{                                                                                                              \
			FText LocalizedText = NSLOCTEXT(LOCTEXT_CPP_STRING_TABLE, Key, SourceString);                              \
			FString BaseString = LocalizedText.ToString();                                                             \
			TArray<FStringFormatArg> FormatArgs = {__VA_ARGS__};                                                       \
			if (FormatArgs.Num() > 0)                                                                                  \
			{                                                                                                          \
				return FString::Format(*BaseString, FormatArgs);                                                       \
			}                                                                                                          \
			return BaseString;                                                                                         \
		}())

class FRSSModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
};
