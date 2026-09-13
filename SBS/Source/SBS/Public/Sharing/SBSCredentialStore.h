// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Structures/SBSSharingTypes.h"

struct SBS_API FSBSCredentialStore
{
	static bool IsSupported();
	static FSBSOperationResult Read(FString& Key);
	static FSBSOperationResult Write(const FString& Key);
	static FSBSOperationResult Delete();
#if WITH_EDITOR

	static bool ValidateIsolatedStore();
#endif
};
