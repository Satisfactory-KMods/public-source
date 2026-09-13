// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Structures/SBSDownloadIdTypes.h"
#include "SBSApiFunctionLibrary.generated.h"

UCLASS()
class SBS_API USBSApiFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, meta = (WorldContext = "WorldContext"), Category = "KMods|Subsystem|SBS",
			  DisplayName = "GetSBSDownloadSubsystem")
	static bool IsBlueprintInstalled(UObject* WorldContext, FString Blueprint);

	UFUNCTION(BlueprintPure, meta = (WorldContext = "WorldContext"), Category = "KMods|Subsystem|SBS")
	static FString GetImageBaseUrl(UObject* WorldContext);

	UFUNCTION(BlueprintPure, Category = "SBS|Download ID")
	static FSBSOperationResult MakeDownloadId(ESBSDownloadKind Kind, FString ID, FString& DownloadId);

	UFUNCTION(BlueprintCallable, Category = "SBS|Download ID")
	static FSBSOperationResult CopyDownloadId(ESBSDownloadKind Kind, FString ID);

	UFUNCTION(BlueprintPure, Category = "SBS|Download ID")
	static FSBSOperationResult ParseDownloadId(FString Input, FSBSDownloadReference& Reference);

	UFUNCTION(BlueprintPure, Category = "SBS|Permissions")
	static bool HasPermission(const FSBSUserData& User, FString Action, FString Service);

	UFUNCTION(BlueprintPure, Category = "SBS|Permissions")
	static bool CanEditContent(const FSBSUserData& User, FString OwnerId);
};
