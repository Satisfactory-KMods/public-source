// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "Structures/SBSSharingTypes.h"

struct SBS_API FSBSSharingProtocol
{
	static constexpr int32 MaxBlueprintBytes = 64 * 1024 * 1024;
	static constexpr int32 MaxConfigBytes = 1024 * 1024;
	static constexpr int32 MaxResponseBytes = 64 * 1024;
	static const TCHAR* ProductionBase();
	static const TCHAR* FixtureKey();
	static bool IsUserKey(const FString& Key);
	static bool IsCredentialUrl(const FString& Url, const FString& Base);
	static FSBSOperationResult ValidateUpload(const FSBSBlueprintUpload& Upload);
	static FSBSOperationResult ReadLocalFiles(const FString& Directory, const FString& Name, TArray<uint8>& Sbp,
											  TArray<uint8>& Config);
	static FSBSOperationResult BuildMultipart(const FSBSBlueprintUpload& Upload, const TArray<uint8>& Sbp,
											  const TArray<uint8>& Config, const FString& Boundary,
											  TArray<uint8>& Body);
	static bool ParseSession(const TSharedPtr<FJsonObject>& Json, FSBSSession& Session);
	static bool ParseUpload(const TSharedPtr<FJsonObject>& Json, const FGuid& RequestId,
							FSBSBlueprintUploadResult& Result);
	static void ClearSecret(FString& Secret);
};
