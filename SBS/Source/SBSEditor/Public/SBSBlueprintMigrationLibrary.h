// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SBSBlueprintMigrationLibrary.generated.h"

class UBlueprint;

UCLASS()
class SBSEDITOR_API USBSBlueprintMigrationLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "SBS|Editor")
	static FString InspectBlueprint(UBlueprint* Blueprint);

	UFUNCTION(BlueprintCallable, Category = "SBS|Editor")
	static FString ValidateNativeData();
	UFUNCTION(BlueprintCallable, Category = "SBS|Editor")
	static FString ValidateSharingData();
	UFUNCTION(BlueprintCallable, Category = "SBS|Editor")
	static FString ValidateDownloadIdData();

	UFUNCTION(BlueprintCallable, Category = "SBS|Editor")
	static int32 MigrateImageUrls(UBlueprint* Blueprint);
};
