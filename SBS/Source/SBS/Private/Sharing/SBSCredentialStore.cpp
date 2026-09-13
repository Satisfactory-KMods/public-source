// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#include "Sharing/SBSCredentialStore.h"

#include "Sharing/SBSSharingProtocol.h"

#if PLATFORM_WINDOWS && !UE_SERVER
#include "Windows/WindowsHWrapper.h"
#include "Windows/AllowWindowsPlatformTypes.h"
#include <wincred.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

namespace
{
#if PLATFORM_WINDOWS && !UE_SERVER
#if WITH_EDITOR
	FString TestTarget;
#endif
	const TCHAR* CredentialTarget()
	{
#if WITH_EDITOR
		if (!TestTarget.IsEmpty())
		{
			return *TestTarget;
		}
#endif
		return TEXT("KMods.SBS:https://k-mods.com/api/v1/sbs:user-key:v1");
	}
	FSBSOperationResult StorageError()
	{
		return FSBSOperationResult::Make(
			TEXT("credential_store_error"),
			NSLOCTEXT("SBS", "Sharing.CredentialStoreError", "Windows could not access the saved SBS login."));
	}
#endif
}

bool FSBSCredentialStore::IsSupported()
{
#if PLATFORM_WINDOWS && !UE_SERVER
	return true;
#else
	return false;
#endif
}

FSBSOperationResult FSBSCredentialStore::Read(FString& Key)
{
	FSBSSharingProtocol::ClearSecret(Key);
#if PLATFORM_WINDOWS && !UE_SERVER
	PCREDENTIALW Credential = nullptr;
	if (!CredReadW(CredentialTarget(), CRED_TYPE_GENERIC, 0, &Credential))
	{
		return GetLastError() == ERROR_NOT_FOUND
			? FSBSOperationResult::Make(TEXT("no_saved_login"),
										NSLOCTEXT("SBS", "Sharing.NoSavedLogin", "No SBS login is saved."))
			: StorageError();
	}

	if (Credential->CredentialBlobSize == 48 && Credential->CredentialBlob)
	{
		for (uint32 Index = 0; Index < Credential->CredentialBlobSize; ++Index)
		{
			Key.AppendChar(static_cast<TCHAR>(Credential->CredentialBlob[Index]));
		}
	}
	if (Credential->CredentialBlob && Credential->CredentialBlobSize)
	{
		FMemory::Memzero(Credential->CredentialBlob, Credential->CredentialBlobSize);
	}
	CredFree(Credential);
	if (FSBSSharingProtocol::IsUserKey(Key))
	{
		return FSBSOperationResult::Make(TEXT("credential_loaded"),
										 NSLOCTEXT("SBS", "Sharing.CredentialLoaded", "Saved SBS login loaded."), true);
	}
	FSBSSharingProtocol::ClearSecret(Key);
	return StorageError();
#else
	return FSBSOperationResult::Make(
		TEXT("storage_unsupported"),
		NSLOCTEXT("SBS", "Sharing.StorageUnsupported", "Secure login storage is unavailable on this platform."));
#endif
}

FSBSOperationResult FSBSCredentialStore::Write(const FString& Key)
{
	if (!FSBSSharingProtocol::IsUserKey(Key))
	{
		return FSBSOperationResult::Make(
			TEXT("invalid_key"),
			NSLOCTEXT("SBS", "Sharing.InvalidKey", "Enter an SBS user key created on K-Mods.com."));
	}
#if PLATFORM_WINDOWS && !UE_SERVER
	uint8 Bytes[48];
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Bytes); ++Index)
	{
		Bytes[Index] = static_cast<uint8>(Key[Index]);
	}
	CREDENTIALW Credential{};
	Credential.Type = CRED_TYPE_GENERIC;
	Credential.TargetName = const_cast<TCHAR*>(CredentialTarget());
	Credential.CredentialBlobSize = sizeof(Bytes);
	Credential.CredentialBlob = Bytes;
	Credential.Persist = CRED_PERSIST_LOCAL_MACHINE;
	const bool bSaved = CredWriteW(&Credential, 0) != 0;
	FMemory::Memzero(Bytes, sizeof(Bytes));
	return bSaved
		? FSBSOperationResult::Make(TEXT("credential_saved"),
									NSLOCTEXT("SBS", "Sharing.CredentialSaved", "SBS login saved on this computer."),
									true)
		: StorageError();
#else
	return FSBSOperationResult::Make(
		TEXT("storage_unsupported"),
		NSLOCTEXT("SBS", "Sharing.StorageUnsupported", "Secure login storage is unavailable on this platform."));
#endif
}

FSBSOperationResult FSBSCredentialStore::Delete()
{
#if PLATFORM_WINDOWS && !UE_SERVER
	if (CredDeleteW(CredentialTarget(), CRED_TYPE_GENERIC, 0) || GetLastError() == ERROR_NOT_FOUND)
	{
		return FSBSOperationResult::Make(TEXT("credential_deleted"),
										 NSLOCTEXT("SBS", "Sharing.CredentialDeleted", "Saved SBS login removed."),
										 true);
	}
	return StorageError();
#else
	return FSBSOperationResult::Make(
		TEXT("storage_unsupported"),
		NSLOCTEXT("SBS", "Sharing.StorageUnsupported", "Secure login storage is unavailable on this platform."));
#endif
}

#if WITH_EDITOR
bool FSBSCredentialStore::ValidateIsolatedStore()
{
#if PLATFORM_WINDOWS && !UE_SERVER
	if (!IsRunningCommandlet() || !IsInGameThread())
	{
		return false;
	}
	const TGuardValue<FString> Guard(TestTarget, TEXT("KMods.SBS:commandlet-test:") + FGuid::NewGuid().ToString());
	FString Loaded;
	const bool bInitiallyMissing = Read(Loaded).Code == TEXT("no_saved_login");
	const bool bWritten = Write(FSBSSharingProtocol::FixtureKey()).bSuccess;
	const bool bLoaded = Read(Loaded).bSuccess && Loaded == FSBSSharingProtocol::FixtureKey();
	FSBSSharingProtocol::ClearSecret(Loaded);
	const bool bDeleted = Delete().bSuccess;
	const bool bMissingAfterDelete = Read(Loaded).Code == TEXT("no_saved_login") && Loaded.IsEmpty();
	return bInitiallyMissing && bWritten && bLoaded && bDeleted && bMissingAfterDelete;
#else
	return !IsSupported();
#endif
}
#endif
