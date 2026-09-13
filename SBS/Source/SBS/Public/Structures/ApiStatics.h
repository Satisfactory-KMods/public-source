// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "Configuration/ModConfiguration.h"
#include "CoreMinimal.h"
#include "ApiStatics.generated.h"

USTRUCT()
struct SBS_API FSBSStatics
{
	GENERATED_BODY()

	inline static FString API_KEY;
	inline static FString API_URL = TEXT("https://k-mods.com/api/v1/sbs/");
	inline static FString API_URL_LOCALDEV = TEXT("http://127.0.0.1/api/v1/");
	inline static FString API_URL_DEV = TEXT("https://dev.k-mods.com/api/v1/sbs/");
	inline static FString API_AUTH = TEXT("mod/authcheck");
	inline static FString API_BLUEPRINT = TEXT("mod/getblueprints");
	inline static FString API_BLUEPRINTPACKS = TEXT("mod/getblueprintpacks");
	inline static FString API_TAGS = TEXT("mod/gettags");
	inline static FString API_RATEBP = TEXT("mod/rateblueprint");
	inline static FString API_BLUEPRINTDOWNLOAD = TEXT("download/");
	inline static TSubclassOf<UModConfiguration> MODCONFIG = nullptr;

	static TSubclassOf<UModConfiguration> GETMODCONFIG();
	static FString MakeUrl(FString To, UObject* WorldContext);
	static FString GetAccountKey(UObject* WorldContext);

	static int32 GetLocalTestPort();
	static FString EncodePathSegment(const FString& Segment);
	static bool IsSafeIdentifier(const FString& Identifier);
	static bool IsSafeFileName(const FString& Name);
};
