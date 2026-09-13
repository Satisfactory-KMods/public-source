// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#include "Sharing/SBSPermissions.h"

namespace
{
	bool IsComponent(const FString& Value)
	{
		if (Value.IsEmpty() || Value.Len() > 64)
		{
			return false;
		}
		for (TCHAR Character : Value)
		{
			if (!((Character >= 'a' && Character <= 'z') || (Character >= '0' && Character <= '9') ||
				  Character == '_' || Character == '.' || Character == '-'))
			{
				return false;
			}
		}
		return true;
	}
}

bool FSBSPermissions::IsPermissionKey(const FString& Key)
{
	FString Action, Service;
	return Key.Split(TEXT(":"), &Action, &Service) && IsComponent(Action) && IsComponent(Service);
}

bool FSBSPermissions::HasPermission(const FSBSUserData& User, const FString& Action, const FString& Service)
{
	if (!FSBSStatics::IsSafeIdentifier(User.ID) || !IsComponent(Action) || !IsComponent(Service))
	{
		return false;
	}
	return User.Permissions.Contains(TEXT("developer:all")) || User.Permissions.Contains(Action + TEXT(":") + Service);
}

bool FSBSPermissions::CanEditContent(const FSBSUserData& User, const FString& OwnerId)
{
	return FSBSStatics::IsSafeIdentifier(User.ID) && FSBSStatics::IsSafeIdentifier(OwnerId) &&
		(User.ID == OwnerId || HasPermission(User, TEXT("moderate"), TEXT("sbs")));
}
