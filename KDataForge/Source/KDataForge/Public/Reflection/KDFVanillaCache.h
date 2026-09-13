// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectKey.h"

class KDATAFORGE_API FKDFVanillaCache
{
public:

	void RecordSnapshot(const UObject* Object, const FString& PropertyPath, const FString& ExportedValue);

	const FString* FindSnapshot(const UObject* Object, const FString& PropertyPath) const;

	bool HasSnapshotUnderProperty(const UObject* Object, const FString& PropertyName) const;

	const TMap<FString, FString>* FindObjectSnapshots(const UObject* Object) const;

	bool HasAnySnapshot(const UObject* Object) const;

	int32 NumObjects() const { return mSnapshots.Num(); }

	const TMap<FObjectKey, TMap<FString, FString>>& GetAllSnapshots() const { return mSnapshots; }

	void Reset();

private:
	TMap<FObjectKey, TMap<FString, FString>> mSnapshots;
};
