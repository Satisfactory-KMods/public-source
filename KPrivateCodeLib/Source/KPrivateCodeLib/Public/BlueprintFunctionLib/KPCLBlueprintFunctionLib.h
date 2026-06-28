// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "Buildables/FGBuildableRailroadTrack.h"
#include "Buildables/FGBuildableResourceExtractorBase.h"
#include "FGIconLibrary.h"
#include "FGInventoryComponent.h"
#include "FGPowerConnectionComponent.h"
#include "UObject/Object.h"

#include "KPCLBlueprintFunctionLib.generated.h"

UCLASS()
class KPRIVATECODELIB_API UKPCLBlueprintFunctionLib : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static void SetAllowOnIndex_ThreadSafe(UFGInventoryComponent* Component, int32 Index,
										   TSubclassOf<UFGItemDescriptor> ItemClass);

	template <class TActorClass>
	static TActorClass* GetClostestActor(const TArray<TActorClass*>& Actors, const FVector& ScanLocation,
										 float MaxRangeSquare = 0.f,
										 TFunction<bool(TActorClass*)> RequirementMet = nullptr);

	UFUNCTION(BlueprintCallable, Category = "KMods|BPFL")
	static void DestroyAndRemoveFromReplicationGraph(AActor* Actor);

	UFUNCTION(BlueprintCallable, Category = "KMods|BPFL")
	static UPanelSlot* InjectWidgetAt(UPanelWidget* TargetPanel, UUserWidget* WidgetToInject, FMargin Padding,
									  int32 Index, bool bRequireTFITCheck);

	UFUNCTION(BlueprintCallable, Category = "KMods|BPFL", meta = (DeterminesOutputType = "InClass"))
	static void UpdateIconLib(UFGIconLibrary* IconLib, TArray<TSubclassOf<UObject>> InObjects, bool bRemoveList);

	UFUNCTION(BlueprintCallable, Category = "KMods|BPFL", meta = (DeterminesOutputType = "InClass"))
	static UObject* GetDefaultSilent(TSubclassOf<UObject> InClass);

	UFUNCTION(BlueprintCallable, Category = "KMods|BPFL")
	static AActor* ResolveHitResult(const FHitResult& InHitResult);

	UFUNCTION(BlueprintCallable, Category = "Widget | Advanced",
			  meta = (DefaultToSelf = "CurrentWidget", DeterminesOutputType = "WidgetClass"))
	static TArray<UWidget*> FindChildWidgetsOfClassDeep(UUserWidget* CurrentWidget, TSubclassOf<UWidget> WidgetClass);
};

template <class TActorClass>
TActorClass* UKPCLBlueprintFunctionLib::GetClostestActor(const TArray<TActorClass*>& Actors,
														 const FVector& ScanLocation, float MaxRangeSquare,
														 TFunction<bool(TActorClass*)> RequirementMet)
{
	TActorClass* ClosestActor = nullptr;
	float ClosestDistSq = MaxRangeSquare > 0.f ? MaxRangeSquare : FLT_MAX;

	for (TActorClass* Actor : Actors)
	{
		if (!IsValid(Actor))
		{
			continue;
		}

		if (RequirementMet && !RequirementMet(Actor))
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(ScanLocation, Actor->GetActorLocation());
		if (DistSq < ClosestDistSq)
		{
			ClosestDistSq = DistSq;
			ClosestActor = Actor;
		}
	}

	return ClosestActor;
}
