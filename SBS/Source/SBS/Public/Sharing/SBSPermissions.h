// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Structures/ApiPostStruct.h"

class SBS_API FSBSPermissions
{
public:
	static bool IsPermissionKey(const FString& Key);
	static bool HasPermission(const FSBSUserData& User, const FString& Action, const FString& Service);
	static bool CanEditContent(const FSBSUserData& User, const FString& OwnerId);
};
