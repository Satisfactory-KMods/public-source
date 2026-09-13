// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UKDFSubsystem;

struct KDATAFORGE_API FKDFGeneratedRecord
{
	FString mPackRef;
	FString mParentClassPath;
	FString mId;
	bool bIsAsset = false;
};

class KDATAFORGE_API FKDFDynamicContentRegistry
{
public:
	void Initialize(UKDFSubsystem* Owner);

	UClass* GetOrCreateClass(const FString& PackRef, const FString& Id, UClass* ParentClass, FString& OutError);

	static UClass* GenerateRuntimeClass(const FString& PackageName, const FString& ClassName, UClass* ParentClass,
		FString& OutError);

	UObject* GetOrCreateAsset(const FString& PackRef, const FString& Id, UClass* AssetClass, FString& OutError);

	UClass* FindGeneratedClass(const FString& PackRef, const FString& Id) const;

	UObject* FindGeneratedAsset(const FString& PackRef, const FString& Id) const;

	void FindGeneratedClassesById(const FString& Id, TArray<UClass*>& OutMatches) const;

	void FindGeneratedAssetsById(const FString& Id, TArray<UObject*>& OutMatches) const;

	void ApplyRedirects(const FString& PackRef, const TMap<FString, FString>& OldIdToNewId);

	int32 ReconstructTombstones(TArray<FString>& OutTombstoneKeys);

	bool IsTombstone(const FString& PackRef, const FString& Id) const;

	void GetAllGeneratedClasses(TArray<UClass*>& OutClasses) const
	{
		for (const TTuple<FString, UClass*>& Pair : mClassesByKey)
		{
			OutClasses.Add(Pair.Value);
		}
	}

	int32 NumGenerated() const { return mClassesByKey.Num() + mAssetsByKey.Num(); }

	static FString SanitizeToken(const FString& Token);
	static FString MakeClassName(const FString& PackRef, const FString& Id);
	static FString MakeClassPackageName(const FString& PackRef);
	static FString MakeAssetPackageName(const FString& PackRef);

private:
	static FString MakeKey(const FString& PackRef, const FString& Id);
	bool ValidateRuntimeIdentity(const FString& PackRef, const FString& Id, bool bIsAsset, FString& OutError) const;
	void LoadManifest();
	void SaveManifest() const;
	void RecordGenerated(const FString& PackRef, const FString& Id, const FString& ParentPath, bool bIsAsset);

	TMap<FString, UClass*> mClassesByKey;
	TMap<FString, TWeakObjectPtr<UObject>> mAssetsByKey;
	TArray<FKDFGeneratedRecord> mManifest;
	TSet<FString> mTombstoneKeys;
	UKDFSubsystem* mOwner = nullptr;
};
