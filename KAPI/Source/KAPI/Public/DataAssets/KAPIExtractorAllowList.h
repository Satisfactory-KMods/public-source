// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "KAPIDataAssetBase.h"
#include "Buildables/FGBuildableResourceExtractorBase.h"

#include "KAPIExtractorAllowList.generated.h"


UCLASS( BlueprintType )
class KAPI_API UKAPIExtractorAllowList : public UKAPIDataAssetBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Extractors")
	TArray<TSubclassOf<AFGBuildableResourceExtractorBase>> mAllowedExtractors;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Extractors")
	TArray<TSubclassOf<AFGBuildableResourceExtractorBase>> mDisallowExtractors;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Resources")
	TArray<TSubclassOf<UFGResourceDescriptor>> mAllowedResources;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Resources")
	TArray<TSubclassOf<UFGResourceDescriptor>> mDisallowResources;
};