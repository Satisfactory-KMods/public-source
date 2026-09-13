// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class SBS_API FSBSBlueprintFileTransaction
{
public:
	FSBSBlueprintFileTransaction(FString Directory, FString Name, TArray<uint8> Sbp, TArray<uint8> Config);
	FSBSBlueprintFileTransaction(const FSBSBlueprintFileTransaction&) = delete;
	FSBSBlueprintFileTransaction& operator=(const FSBSBlueprintFileTransaction&) = delete;
	~FSBSBlueprintFileTransaction();
	bool Stage();
	bool Commit();
	bool Rollback();
	void Accept();
	const FString& GetDirectory() const { return mDirectory; }

private:
	FString mDirectory;
	FString mName;
	FString mStagingDirectory;
	TArray<uint8> mSbp;
	TArray<uint8> mConfig;
	bool bBackedUp[2] = {};
	bool bInstalled[2] = {};
	bool bKeepRecoveryFiles = false;
	FString Destination(int32 Index) const;
	FString Staged(int32 Index) const;
	FString Backup(int32 Index) const;
	void Cleanup();
};
