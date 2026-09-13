// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Structures/ApiJsonStruct.h"
#include "Structures/SBSSharingTypes.h"
#include "SBSDownloadIdTypes.generated.h"

UENUM(BlueprintType)
enum class ESBSDownloadKind : uint8
{
	Auto,
	Blueprint,
	Pack
};

USTRUCT(BlueprintType)
struct SBS_API FSBSDownloadReference
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "SBS|Download ID")
	ESBSDownloadKind Kind = ESBSDownloadKind::Auto;
	UPROPERTY(BlueprintReadOnly, Category = "SBS|Download ID")
	FString ID;

	UPROPERTY(BlueprintReadOnly, Category = "SBS|Download ID")
	FString DownloadId;
};

USTRUCT(BlueprintType)
struct SBS_API FSBSResolvedDownload
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "SBS|Download ID")
	FGuid RequestId;
	UPROPERTY(BlueprintReadOnly, Category = "SBS|Download ID")
	FSBSOperationResult Result;
	UPROPERTY(BlueprintReadOnly, Category = "SBS|Download ID")
	FSBSDownloadReference Reference;
	UPROPERTY(BlueprintReadOnly, Category = "SBS|Download ID")
	FBlueprintJsonStructure Blueprint;
	UPROPERTY(BlueprintReadOnly, Category = "SBS|Download ID")
	FBlueprintPackJsonStructure Pack;
};
