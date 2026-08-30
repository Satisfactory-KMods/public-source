#pragma once

#include "CoreMinimal.h"

#include "Modules/ModuleManager.h"

#undef LOCTEXT_CPP_STRING_TABLE
#define LOCTEXT_CPP_STRING_TABLE "KAPI/ST_KAPI_CPP"

#define FI18N_TEXT(Key) LOCTABLE(LOCTEXT_CPP_STRING_TABLE, Key)

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

#define FI18N_NS(Key, SourceString) NSLOCTEXT(LOCTEXT_CPP_STRING_TABLE, Key, SourceString)

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

DECLARE_LOG_CATEGORY_EXTERN(LogKApi, Log, All)
DECLARE_LOG_CATEGORY_EXTERN(LogKApiCDOHelper, Log, All);

class FKAPIModule : public FDefaultGameModuleImpl
{
public:

	virtual void StartupModule() override;

	virtual bool IsGameModule() const override { return true; }

	void GameFix();
};
