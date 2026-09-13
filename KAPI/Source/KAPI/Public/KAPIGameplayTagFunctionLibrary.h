// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "KAPIGameplayTagFunctionLibrary.generated.h"

class UKAPIDataAssetBase;

UCLASS()
class KAPI_API UKAPIGameplayTagFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintPure, Category = "KMods|GameplayTags")
	static FGameplayTag MakeGameplayTagFromName(FName TagName);

	UFUNCTION(BlueprintPure, Category = "KMods|GameplayTags")
	static FGameplayTagContainer MakeGameplayTagContainerFromNames(const TArray<FName>& TagNames);

	UFUNCTION(BlueprintPure, Category = "KMods|GameplayTags")
	static TArray<FName> GetTagNames(const FGameplayTagContainer& Container);

	UFUNCTION(BlueprintPure, Category = "KMods|GameplayTags")
	static FGameplayTagContainer GetGameplayTagsForClass(UClass* InClass);

	UFUNCTION(BlueprintPure, Category = "KMods|GameplayTags")
	static FGameplayTagContainer GetGameplayTagsForObject(const UObject* Object);

	UFUNCTION(BlueprintPure, Category = "KMods|GameplayTags")
	static bool ClassHasTag(UClass* InClass, FGameplayTag Tag, bool bExactMatch = false);

	UFUNCTION(BlueprintPure, Category = "KMods|GameplayTags")
	static bool ClassHasAnyTags(UClass* InClass, const FGameplayTagContainer& Tags, bool bExactMatch = false);

	UFUNCTION(BlueprintPure, Category = "KMods|GameplayTags")
	static bool ClassHasAllTags(UClass* InClass, const FGameplayTagContainer& Tags, bool bExactMatch = false);

	UFUNCTION(BlueprintPure, Category = "KMods|GameplayTags")
	static bool ObjectHasTag(const UObject* Object, FGameplayTag Tag, bool bExactMatch = false);

	UFUNCTION(BlueprintPure, Category = "KMods|GameplayTags")
	static bool ObjectHasAnyTags(const UObject* Object, const FGameplayTagContainer& Tags, bool bExactMatch = false);

	UFUNCTION(BlueprintPure, Category = "KMods|GameplayTags")
	static bool ObjectHasAllTags(const UObject* Object, const FGameplayTagContainer& Tags, bool bExactMatch = false);

	UFUNCTION(BlueprintCallable, Category = "KMods|GameplayTags")
	static bool AddTagToContainer(UPARAM(ref) FGameplayTagContainer& Container, FGameplayTag Tag);

	UFUNCTION(BlueprintCallable, Category = "KMods|GameplayTags")
	static void AppendTagsToContainer(UPARAM(ref) FGameplayTagContainer& Container, const FGameplayTagContainer& Other);

	UFUNCTION(BlueprintCallable, Category = "KMods|GameplayTags")
	static bool RemoveTagFromContainer(UPARAM(ref) FGameplayTagContainer& Container, FGameplayTag Tag);

	UFUNCTION(BlueprintCallable, Category = "KMods|GameplayTags")
	static void RemoveTagsFromContainer(UPARAM(ref) FGameplayTagContainer& Container,
										const FGameplayTagContainer& Other);

	UFUNCTION(BlueprintPure, Category = "KMods|GameplayTags")
	static bool ContainersAreEqual(const FGameplayTagContainer& A, const FGameplayTagContainer& B,
								   bool bExactMatch = true);

	UFUNCTION(BlueprintPure, Category = "KMods|GameplayTags")
	static FGameplayTagContainer FilterContainer(const FGameplayTagContainer& Container,
												 const FGameplayTagContainer& Filter, bool bExactMatch = false);
};
