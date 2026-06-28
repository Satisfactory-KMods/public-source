// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "KPCLModSubsystem.h"

#include "KPCLSwatchSystem.generated.h"

USTRUCT(BlueprintType)
struct FCustomSwatchData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	FName mSlotName = "None";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	FLinearColor mPrimaryColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	FLinearColor mSecondaryColor;

	friend bool operator==(const FCustomSwatchData& A, const FCustomSwatchData& B)
	{
		return A.mSlotName == B.mSlotName && A.mPrimaryColor == B.mPrimaryColor &&
			A.mSecondaryColor == B.mSecondaryColor;
	}

	friend bool operator!=(const FCustomSwatchData& A, const FCustomSwatchData& B)
	{
		return A.mSlotName != B.mSlotName || A.mPrimaryColor != B.mPrimaryColor ||
			A.mSecondaryColor != B.mSecondaryColor;
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSwatchDataUpdated);

UCLASS()
class KPRIVATECODELIB_API AKPCLSwatchSystem : public AKPCLModSubsystem
{
	GENERATED_BODY()

public:
	AKPCLSwatchSystem();

	UFUNCTION(BlueprintPure, Category = "Subsystem", DisplayName = "GetKPCLSwatchSystem",
			  meta = (DefaultToSelf = "worldContext"))
	static AKPCLSwatchSystem* Get(UObject* worldContext);

	UFUNCTION(BlueprintCallable, Category = "SwatchSystem")
	void AddCustomSwatchData(FCustomSwatchData Data);

	UFUNCTION(BlueprintCallable, Category = "SwatchSystem")
	void RemoveCustomSwatchData(int32 Idx);

	UFUNCTION(BlueprintCallable, Category = "SwatchSystem")
	void UseCustomSwatchData(int32 Idx);

	UFUNCTION(BlueprintCallable, Category = "SwatchSystem")
	void UpdateCustomSwatchData(FCustomSwatchData Data, int32 Idx);

	UFUNCTION(BlueprintPure, Category = "SwatchSystem")
	TArray<FCustomSwatchData> GetCustomSwatchData() const;

	UFUNCTION()
	void OnRep_CustomSwatchData();

	UPROPERTY(BlueprintAssignable, Category = "SwatchSystem")
	FOnSwatchDataUpdated mOnSwatchDataUpdated;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UPROPERTY(EditDefaultsOnly, SaveGame, ReplicatedUsing = OnRep_CustomSwatchData, Category = "KMods")
	TArray<FCustomSwatchData> mCustomSwatchData;
};
