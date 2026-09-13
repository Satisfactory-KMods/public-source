// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Structures/SBSDownloadIdTypes.h"

class SBS_API FSBSDownloadIdProtocol
{
public:
	static constexpr int32 MaxInputCharacters = 256;
	static constexpr int32 MaxResponseBytes = 1024 * 1024;
	static constexpr int32 MaxPackMembers = 512;

	static FSBSOperationResult ParseInput(const FString& Input, FSBSDownloadReference& Reference);
	static FSBSOperationResult Format(ESBSDownloadKind Kind, const FString& ID, FString& DownloadId);
	static FString MakePayload(const FSBSDownloadReference& Reference);
	static bool ParseResponse(const TSharedPtr<FJsonObject>& Json, const FSBSDownloadReference& Requested,
							  FSBSResolvedDownload& Download, UObject* WorldContext);
	static FSBSOperationResult HttpFailure(int32 Status);
};
