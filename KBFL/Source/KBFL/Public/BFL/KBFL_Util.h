#pragma once

#include "Buildables/FGBuildableResourceExtractorBase.h"
#include "CoreMinimal.h"
#include "FGPlayerState.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Resources/FGResourceDescriptor.h"
#include "Resources/FGResourceNode.h"
#include "Subsystem/ModSubsystem.h"

#include "KBFL_Util.generated.h"

UCLASS()
class KBFL_API UKBFL_Util : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Sorting a Item Array */
	UFUNCTION(BlueprintCallable, Category = "KMods|Util")
	static void SortItemArray(TArray<TSubclassOf<UFGItemDescriptor>>& Out_Items,
							  const TArray<TSubclassOf<UFGItemDescriptor>>& In_Items,
							  const TArray<TSubclassOf<UFGItemDescriptor>>& ForceFirstItems, bool Reverse = false);

	/**
	 * Get Subsystem by Class
	 * @note Alias for GetSubsystemFromChild
	 */
	UFUNCTION(BlueprintPure, meta = (WorldContext = "WorldContext"), Category = "KMods|Util")
	static AModSubsystem* GetSubsystem(UObject* WorldContext, TSubclassOf<AModSubsystem> Subsystem);

	UFUNCTION(BlueprintPure, meta = (WorldContext = "WorldContext", AutoCreateRefTerm), Category = "KMods|Util")
	static AModSubsystem* GetSubsystemFromChild(UObject* WorldContext, TSubclassOf<AModSubsystem> SubsystemClass);

	template <class T>
	static T* GetSubsystem(UObject* WorldContext)
	{
		return Cast<T>(GetSubsystemFromChild(WorldContext, T::StaticClass()));
	}

	UFUNCTION(BlueprintPure, meta = (WorldContext = "WorldContext"), Category = "KMods|Util")
	static void GetAllSubsystemsFromChild(UObject* WorldContext, TSubclassOf<AModSubsystem> SubsystemClass,
										  TArray<AModSubsystem*>& Subsystems);

	/** Remove all Resource not what NOT in the Array */
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContext"), Category = "KMods|Util")
	static bool DoPlayerViewLineTrace(UObject* WorldContext, FHitResult& Hit, float Distance,
									  TArray<AActor*> ActorsToIgnore, ETraceTypeQuery TraceChannel,
									  bool TraceComplex = true);

	/** Remove all Resource not what NOT in the Array */
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContext"), Category = "KMods|Util")
	static bool DoPlayerViewLineTraceSphere(UObject* WorldContext, TArray<AActor*>& OutActors, float Distance,
											TArray<AActor*> ActorsToIgnore, ETraceTypeQuery TraceChannel,
											TArray<TEnumAsByte<EObjectTypeQuery>> ObjTypes,
											TSubclassOf<AActor> ActorClass, float SphereSize = 20.f,
											bool TraceComplex = true);
};
