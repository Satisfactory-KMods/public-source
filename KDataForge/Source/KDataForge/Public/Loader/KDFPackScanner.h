// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "KDFTypes.h"
#include "Loader/KDFLoaderTypes.h"

class KDATAFORGE_API FKDFPackScanner
{
public:

	static FString GetSearchRoot();

	static TArray<FString> FindDataEditorRoots();

	static void ScanPacks(TArray<FKDFPack>& OutPacks, TMap<FString, TArray<FString>>& OutFiles,
						  TArray<FKDFDiagnostic>& Diagnostics);
};
