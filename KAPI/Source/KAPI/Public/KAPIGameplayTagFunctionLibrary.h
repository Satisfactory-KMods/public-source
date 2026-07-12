// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "KAPIGameplayTagFunctionLibrary.generated.h"

class UKAPIDataAssetBase;

/**
 * Gameplay tag helpers for Blueprint and Python tooling.
 *
 * Python cannot construct FGameplayTag values by name (the struct exposes no editable properties),
 * so editor automation scripts use these functions to mint tags for asset migrations.
 *
 * The class/object helpers cover all common tag carriers: item/equipment/vehicle/building
 * descriptors (UFGItemDescriptor), recipes (UFGRecipe), schematics (UFGSchematic), research trees
 * (UFGResearchTree), KAPI data assets (UKAPIDataAssetBase::mTags) and anything implementing
 * IGameplayTagAssetInterface. Buildables carry their tags on the building descriptor, not on the
 * actor class.
 */
UCLASS()
class KAPI_API UKAPIGameplayTagFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Request a gameplay tag by name. Returns an empty tag when the name is not registered (no error). */
	UFUNCTION(BlueprintPure, Category = "KMods|GameplayTags")
	static FGameplayTag MakeGameplayTagFromName(FName TagName);

	/** Request multiple gameplay tags by name. Unregistered names are skipped. */
	UFUNCTION(BlueprintPure, Category = "KMods|GameplayTags")
	static FGameplayTagContainer MakeGameplayTagContainerFromNames(const TArray<FName>& TagNames);

	/** Tag names of all tags in the container — inverse of MakeGameplayTagContainerFromNames. */
	UFUNCTION(BlueprintPure, Category = "KMods|GameplayTags")
	static TArray<FName> GetTagNames(const FGameplayTagContainer& Container);

	/**
	 * All gameplay tags of a class. Dispatches to the matching tag getter for item descriptors
	 * (including building/equipment/vehicle descriptors), recipes, schematics and research trees;
	 * any other class is read via its CDO when it implements IGameplayTagAssetInterface.
	 */
	UFUNCTION(BlueprintPure, Category = "KMods|GameplayTags")
	static FGameplayTagContainer GetGameplayTagsForClass(UClass* InClass);

	/**
	 * All gameplay tags of an object: IGameplayTagAssetInterface implementers and KAPI data assets
	 * (mTags) are read directly, UClass values forward to GetGameplayTagsForClass, every other
	 * instance falls back to the tags of its class.
	 */
	UFUNCTION(BlueprintPure, Category = "KMods|GameplayTags")
	static FGameplayTagContainer GetGameplayTagsForObject(const UObject* Object);

	/** True when the class carries the tag (see GetGameplayTagsForClass for coverage). */
	UFUNCTION(BlueprintPure, Category = "KMods|GameplayTags")
	static bool ClassHasTag(UClass* InClass, FGameplayTag Tag, bool bExactMatch = false);

	/** True when the class carries at least one tag of the container. */
	UFUNCTION(BlueprintPure, Category = "KMods|GameplayTags")
	static bool ClassHasAnyTags(UClass* InClass, const FGameplayTagContainer& Tags, bool bExactMatch = false);

	/** True when the class carries every tag of the container. An empty container returns true. */
	UFUNCTION(BlueprintPure, Category = "KMods|GameplayTags")
	static bool ClassHasAllTags(UClass* InClass, const FGameplayTagContainer& Tags, bool bExactMatch = false);

	/** True when the object carries the tag (see GetGameplayTagsForObject for coverage). */
	UFUNCTION(BlueprintPure, Category = "KMods|GameplayTags")
	static bool ObjectHasTag(const UObject* Object, FGameplayTag Tag, bool bExactMatch = false);

	/** True when the object carries at least one tag of the container. */
	UFUNCTION(BlueprintPure, Category = "KMods|GameplayTags")
	static bool ObjectHasAnyTags(const UObject* Object, const FGameplayTagContainer& Tags, bool bExactMatch = false);

	/** True when the object carries every tag of the container. An empty container returns true. */
	UFUNCTION(BlueprintPure, Category = "KMods|GameplayTags")
	static bool ObjectHasAllTags(const UObject* Object, const FGameplayTagContainer& Tags, bool bExactMatch = false);

	/** Adds a tag to the container. Invalid tags are ignored. Returns true when the tag was added. */
	UFUNCTION(BlueprintCallable, Category = "KMods|GameplayTags")
	static bool AddTagToContainer(UPARAM(ref) FGameplayTagContainer& Container, FGameplayTag Tag);

	/** Appends all tags of Other to the container. */
	UFUNCTION(BlueprintCallable, Category = "KMods|GameplayTags")
	static void AppendTagsToContainer(UPARAM(ref) FGameplayTagContainer& Container, const FGameplayTagContainer& Other);

	/** Removes a tag from the container. Returns true when the tag was present. */
	UFUNCTION(BlueprintCallable, Category = "KMods|GameplayTags")
	static bool RemoveTagFromContainer(UPARAM(ref) FGameplayTagContainer& Container, FGameplayTag Tag);

	/** Removes all tags of Other from the container. */
	UFUNCTION(BlueprintCallable, Category = "KMods|GameplayTags")
	static void RemoveTagsFromContainer(UPARAM(ref) FGameplayTagContainer& Container,
										const FGameplayTagContainer& Other);

	/**
	 * Compares two containers. Exact match compares tag-for-tag equality; otherwise both containers
	 * only need to match each other including parent tags (A.HasAll(B) && B.HasAll(A)).
	 */
	UFUNCTION(BlueprintPure, Category = "KMods|GameplayTags")
	static bool ContainersAreEqual(const FGameplayTagContainer& A, const FGameplayTagContainer& B,
								   bool bExactMatch = true);

	/** Intersection of Container and Filter — all tags of Container that match any tag in Filter. */
	UFUNCTION(BlueprintPure, Category = "KMods|GameplayTags")
	static FGameplayTagContainer FilterContainer(const FGameplayTagContainer& Container,
												 const FGameplayTagContainer& Filter, bool bExactMatch = false);
};
