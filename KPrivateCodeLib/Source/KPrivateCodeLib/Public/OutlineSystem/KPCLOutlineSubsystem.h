#pragma once

#include "CoreMinimal.h"
#include "Components/PostProcessComponent.h"
#include "Subsystem/KPCLModSubsystem.h"
#include "KPCLOutlineSubsystem.generated.h"

UENUM(BlueprintType)
enum class EOutlineType : uint8
{
	Fill = 0 UMETA(DisplayName = "Fill"),
	OutlineAndFill = 1 UMETA(DisplayName = "Outline And Fill"),
	Outline = 2 UMETA(DisplayName = "Outline")
};

UENUM(BlueprintType)
enum class EOutlineColorSlot : uint8
{
	Invalid = 0 UMETA(Hidden),
	ColorSlot0 = 100 UMETA(DisplayName = "Color 1"),
	ColorSlot1 = 103 UMETA(DisplayName = "Color 2"),
	ColorSlot2 = 106 UMETA(DisplayName = "Color 3"),
	ColorSlot3 = 109 UMETA(DisplayName = "Color 4"),
	ColorSlot4 = 112 UMETA(DisplayName = "Color 5")
};

USTRUCT(BlueprintType)
struct FOutlineData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EOutlineType mOutlineType = EOutlineType::OutlineAndFill;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EOutlineColorSlot mOutlineColorSlot = EOutlineColorSlot::ColorSlot0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	AActor* mActorToOutline = nullptr;
};

USTRUCT(BlueprintType)
struct FParameterInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName mTag = TEXT("Tag");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor mDefaultColor = FLinearColor();
};

/**
Subsystem To create and replicate Outlines
*/
UCLASS()
class KPRIVATECODELIB_API AKPCLOutlineSubsystem : public AKPCLModSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Subsystem", DisplayName = "GetCustomOutlineSubsystem",
		meta = ( DefaultToSelf = "WorldContext" ))
	static AKPCLOutlineSubsystem* Get(UObject* worldContext);

	AKPCLOutlineSubsystem();
	virtual void BeginPlay() override;

	UFUNCTION(NetMulticast, Reliable)
	void MultiCast_CreateOutlineForActor(FOutlineData OutlineData);

	UFUNCTION(NetMulticast, Reliable)
	void MultiCast_ClearOutlines();

	UFUNCTION(NetMulticast, Reliable)
	void MultiCast_ClearOutlinesForActor(AActor* Actor);

	UFUNCTION(NetMulticast, Reliable)
	void MultiCast_SetOutlineColor(FLinearColor Color, EOutlineColorSlot ColorSlot);

	/** Create a new Outline Actor based on the target Actor */
	UFUNCTION(BlueprintCallable)
	void CreateOutline(FOutlineData OutlineData, bool Multicast = false);

	/** Remove all Outline Actors */
	UFUNCTION(BlueprintCallable)
	void ClearOutlines(bool Multicast = false);

	/** Remove all Outline Actors */
	UFUNCTION(BlueprintCallable)
	void ClearOutlinesForActor(AActor* Actor, bool Multicast = false);

	/** Overwrite the DismantleColor */
	UFUNCTION(BlueprintCallable)
	void SetOutlineColor(FLinearColor Color, EOutlineColorSlot ColorSlot, bool Multicast = false);

	/** Get Outline Actor for Actor if it exsists */
	UFUNCTION(BlueprintPure)
	FORCEINLINE class AKPCLOutlineActor* GetOutlineActorForActor(AActor* Actor) const
	{
		if (mOutlineMap.Contains(Actor))
		{
			return mOutlineMap[Actor];
		}
		return nullptr;
	};

	/** Check if we have any outlines */
	UFUNCTION(BlueprintPure)
	FORCEINLINE bool HasAnyOutlines() const
	{
		return mOutlineMap.Num() > 0;
	};

	UPROPERTY(Transient)
	TMap<AActor*, class AKPCLOutlineActor*> mOutlineMap;

	UPROPERTY(EditDefaultsOnly, Category="KMods")
	TArray<UMaterialInterface*> mPPMaterials;

	UPROPERTY(EditDefaultsOnly, Category="KMods")
	UMaterialParameterCollection* mMaterialCollection;

	UPROPERTY()
	UMaterialParameterCollectionInstance* mMaterialCollectionInstance;

	UPROPERTY(EditDefaultsOnly, Category="KMods")
	TArray<FParameterInfo> mMaterialCollectionTags = {
		FParameterInfo(), FParameterInfo(), FParameterInfo(), FParameterInfo(), FParameterInfo()
	};

	// Components
	UPROPERTY(EditAnywhere)
	USceneComponent* mScene;

	UPROPERTY(EditAnywhere)
	UPostProcessComponent* mPostProcess;
};
