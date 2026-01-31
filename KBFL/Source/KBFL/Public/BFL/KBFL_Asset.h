#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "KBFL_Asset.generated.h"

/**
 *
 */
UCLASS()
class KBFL_API UKBFL_Asset : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Return the Index where the ItemClass allowed on Slot in Inventory */
	template <class T>
	static bool GetAllClassesOfSubclass(TArray<FAssetData> AllAssets, TArray<TSubclassOf<T>>& OutClasses);

	template <class T>
	static bool GetAllClassesOfSubclass(TArray<FAssetData> AllAssets, TSet<TSubclassOf<T>>& OutClasses);

	/** Return the Index where the ItemClass allowed on Slot in Inventory */
	UFUNCTION(BlueprintCallable, Category = "KMods|Items", meta = (DeterminesOutputType = "TSubclassOf<UObject>"))
	static bool GetAllClassesOfSubclass(TArray<FAssetData> AllAssets, TArray<TSubclassOf<UObject>>& OutClasses);

	/** Return the Index where the ItemClass allowed on Slot in Inventory */
	UFUNCTION(BlueprintCallable, Category = "KMods|Items", meta = (DeterminesOutputType = "TSubclassOf<UObject>"))
	static bool FindChildsByClass(TArray<TSubclassOf<UObject>>& OutClasses, TSubclassOf<UObject> Search,
								  bool bSearchSubClasses = false);

	UFUNCTION(BlueprintCallable, Category = "KMods|Items", CallInEditor)
	static void MarkDefaultDirty(UClass* Class);
};
