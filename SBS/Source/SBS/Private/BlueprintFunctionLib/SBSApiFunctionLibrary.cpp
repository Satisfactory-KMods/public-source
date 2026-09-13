// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#include "BlueprintFunctionLib/SBSApiFunctionLibrary.h"

#include "Engine/Engine.h"
#include "Download/SBSDownloadIdProtocol.h"
#include "FGBlueprintSubsystem.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Sharing/SBSPermissions.h"
#include "Structures/ApiStatics.h"

FString USBSApiFunctionLibrary::GetImageBaseUrl(UObject* WorldContext)
{
	return FSBSStatics::MakeUrl(TEXT("image/"), WorldContext);
}

bool USBSApiFunctionLibrary::IsBlueprintInstalled(UObject* WorldContext, FString Blueprint)
{
	if (!FSBSStatics::IsSafeFileName(Blueprint))
	{
		return false;
	}
	UWorld* World =
		GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
	AFGBlueprintSubsystem* Subsystem = World ? AFGBlueprintSubsystem::Get(World) : nullptr;
	return IsValid(Subsystem) && Subsystem->DoesBlueprintExist(Blueprint);
}

FSBSOperationResult USBSApiFunctionLibrary::MakeDownloadId(ESBSDownloadKind Kind, FString ID, FString& DownloadId)
{
	return FSBSDownloadIdProtocol::Format(Kind, ID, DownloadId);
}

FSBSOperationResult USBSApiFunctionLibrary::ParseDownloadId(FString Input, FSBSDownloadReference& Reference)
{
	return FSBSDownloadIdProtocol::ParseInput(Input, Reference);
}

FSBSOperationResult USBSApiFunctionLibrary::CopyDownloadId(ESBSDownloadKind Kind, FString ID)
{
	FString DownloadId;
	const FSBSOperationResult Validation = MakeDownloadId(Kind, MoveTemp(ID), DownloadId);
	if (!Validation.bSuccess)
	{
		return Validation;
	}
	if (IsRunningDedicatedServer() || IsRunningCommandlet())
	{
		return FSBSOperationResult::Make(
			TEXT("clipboard_unavailable"),
			NSLOCTEXT("SBS", "DownloadId.ClipboardUnavailable", "The clipboard is unavailable here."));
	}
	FPlatformApplicationMisc::ClipboardCopy(*DownloadId);
	return FSBSOperationResult::Make(TEXT("download_id_copied"),
									 NSLOCTEXT("SBS", "DownloadId.Copied", "Download ID copied."), true);
}

bool USBSApiFunctionLibrary::HasPermission(const FSBSUserData& User, FString Action, FString Service)
{
	return FSBSPermissions::HasPermission(User, Action, Service);
}

bool USBSApiFunctionLibrary::CanEditContent(const FSBSUserData& User, FString OwnerId)
{
	return FSBSPermissions::CanEditContent(User, OwnerId);
}
