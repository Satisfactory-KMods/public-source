// ILikeBanas

#include "KAPIGameplayTagFunctionLibrary.h"

#include "DataAssets/KAPIDataAssetBase.h"
#include "FGRecipe.h"
#include "FGResearchTree.h"
#include "FGSchematic.h"
#include "GameplayTagAssetInterface.h"
#include "Resources/FGItemDescriptor.h"

FGameplayTag UKAPIGameplayTagFunctionLibrary::MakeGameplayTagFromName(FName TagName)
{
	return FGameplayTag::RequestGameplayTag(TagName, false);
}

FGameplayTagContainer UKAPIGameplayTagFunctionLibrary::MakeGameplayTagContainerFromNames(const TArray<FName>& TagNames)
{
	FGameplayTagContainer Container;
	for (const FName& TagName : TagNames)
	{
		const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(TagName, false);
		if (Tag.IsValid())
		{
			Container.AddTag(Tag);
		}
	}
	return Container;
}

TArray<FName> UKAPIGameplayTagFunctionLibrary::GetTagNames(const FGameplayTagContainer& Container)
{
	TArray<FName> Names;
	Names.Reserve(Container.Num());
	for (const FGameplayTag& Tag : Container)
	{
		Names.Add(Tag.GetTagName());
	}
	return Names;
}

FGameplayTagContainer UKAPIGameplayTagFunctionLibrary::GetGameplayTagsForClass(UClass* InClass)
{
	if (!IsValid(InClass))
	{
		return FGameplayTagContainer();
	}

	// Covers items, equipment, vehicles and buildings — building tags live on the building descriptor.
	if (InClass->IsChildOf(UFGItemDescriptor::StaticClass()))
	{
		return UFGItemDescriptor::GetAllGameplayTags(InClass);
	}
	if (InClass->IsChildOf(UFGRecipe::StaticClass()))
	{
		return UFGRecipe::GetAllGameplayTags(InClass);
	}
	if (InClass->IsChildOf(UFGSchematic::StaticClass()))
	{
		return UFGSchematic::GetAllGameplayTags(InClass);
	}
	if (InClass->IsChildOf(UFGResearchTree::StaticClass()))
	{
		return UFGResearchTree::GetAllGameplayTags(InClass);
	}

	if (const IGameplayTagAssetInterface* TagInterface = Cast<IGameplayTagAssetInterface>(InClass->GetDefaultObject()))
	{
		FGameplayTagContainer Tags;
		TagInterface->GetOwnedGameplayTags(Tags);
		return Tags;
	}

	return FGameplayTagContainer();
}

FGameplayTagContainer UKAPIGameplayTagFunctionLibrary::GetGameplayTagsForObject(const UObject* Object)
{
	if (!IsValid(Object))
	{
		return FGameplayTagContainer();
	}

	// A UClass passed as object is a class query.
	if (const UClass* AsClass = Cast<UClass>(Object))
	{
		return GetGameplayTagsForClass(const_cast<UClass*>(AsClass));
	}

	if (const IGameplayTagAssetInterface* TagInterface = Cast<IGameplayTagAssetInterface>(Object))
	{
		FGameplayTagContainer Tags;
		TagInterface->GetOwnedGameplayTags(Tags);
		return Tags;
	}

	if (const UKAPIDataAssetBase* DataAsset = Cast<UKAPIDataAssetBase>(Object))
	{
		return DataAsset->mTags;
	}

	// Instances of tag carriers (e.g. a recipe or descriptor CDO) fall back to their class.
	return GetGameplayTagsForClass(Object->GetClass());
}

bool UKAPIGameplayTagFunctionLibrary::ClassHasTag(UClass* InClass, FGameplayTag Tag, bool bExactMatch)
{
	const FGameplayTagContainer Tags = GetGameplayTagsForClass(InClass);
	return bExactMatch ? Tags.HasTagExact(Tag) : Tags.HasTag(Tag);
}

bool UKAPIGameplayTagFunctionLibrary::ClassHasAnyTags(UClass* InClass, const FGameplayTagContainer& Tags,
													  bool bExactMatch)
{
	const FGameplayTagContainer ClassTags = GetGameplayTagsForClass(InClass);
	return bExactMatch ? ClassTags.HasAnyExact(Tags) : ClassTags.HasAny(Tags);
}

bool UKAPIGameplayTagFunctionLibrary::ClassHasAllTags(UClass* InClass, const FGameplayTagContainer& Tags,
													  bool bExactMatch)
{
	const FGameplayTagContainer ClassTags = GetGameplayTagsForClass(InClass);
	return bExactMatch ? ClassTags.HasAllExact(Tags) : ClassTags.HasAll(Tags);
}

bool UKAPIGameplayTagFunctionLibrary::ObjectHasTag(const UObject* Object, FGameplayTag Tag, bool bExactMatch)
{
	const FGameplayTagContainer Tags = GetGameplayTagsForObject(Object);
	return bExactMatch ? Tags.HasTagExact(Tag) : Tags.HasTag(Tag);
}

bool UKAPIGameplayTagFunctionLibrary::ObjectHasAnyTags(const UObject* Object, const FGameplayTagContainer& Tags,
													   bool bExactMatch)
{
	const FGameplayTagContainer ObjectTags = GetGameplayTagsForObject(Object);
	return bExactMatch ? ObjectTags.HasAnyExact(Tags) : ObjectTags.HasAny(Tags);
}

bool UKAPIGameplayTagFunctionLibrary::ObjectHasAllTags(const UObject* Object, const FGameplayTagContainer& Tags,
													   bool bExactMatch)
{
	const FGameplayTagContainer ObjectTags = GetGameplayTagsForObject(Object);
	return bExactMatch ? ObjectTags.HasAllExact(Tags) : ObjectTags.HasAll(Tags);
}

bool UKAPIGameplayTagFunctionLibrary::AddTagToContainer(FGameplayTagContainer& Container, FGameplayTag Tag)
{
	if (!Tag.IsValid() || Container.HasTagExact(Tag))
	{
		return false;
	}
	Container.AddTag(Tag);
	return true;
}

void UKAPIGameplayTagFunctionLibrary::AppendTagsToContainer(FGameplayTagContainer& Container,
															const FGameplayTagContainer& Other)
{
	Container.AppendTags(Other);
}

bool UKAPIGameplayTagFunctionLibrary::RemoveTagFromContainer(FGameplayTagContainer& Container, FGameplayTag Tag)
{
	return Container.RemoveTag(Tag);
}

void UKAPIGameplayTagFunctionLibrary::RemoveTagsFromContainer(FGameplayTagContainer& Container,
															  const FGameplayTagContainer& Other)
{
	Container.RemoveTags(Other);
}

bool UKAPIGameplayTagFunctionLibrary::ContainersAreEqual(const FGameplayTagContainer& A, const FGameplayTagContainer& B,
														 bool bExactMatch)
{
	if (bExactMatch)
	{
		return A == B;
	}
	return A.HasAll(B) && B.HasAll(A);
}

FGameplayTagContainer UKAPIGameplayTagFunctionLibrary::FilterContainer(const FGameplayTagContainer& Container,
																	   const FGameplayTagContainer& Filter,
																	   bool bExactMatch)
{
	return bExactMatch ? Container.FilterExact(Filter) : Container.Filter(Filter);
}
