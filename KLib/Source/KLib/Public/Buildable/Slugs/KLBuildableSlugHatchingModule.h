// ILikeBanas

#pragma once

#include "CoreMinimal.h"
#include "KLBuildableSlugBuildingBase.h"
#include "KLBuildableSlugHatchingModule.generated.h"

UENUM()
enum class EHatchingModuleType : uint8
{
	None = 0 UMETA(Displayname = "Invalid"),
	Temperature = 1 UMETA(Displayname = "Temperature"),
	Tank = 2 UMETA(Displayname = "Tank"),
	Incubator = 3 UMETA(Displayname = "Incubator"),
	Humidity = 4 UMETA(Displayname = "Humidity"),
	Time = 5 UMETA(Displayname = "Time")
};

USTRUCT(BlueprintType)
struct FKLHatchingModeleStats
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly, NotReplicated)
	float mMin = .2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, NotReplicated)
	float mMax = .8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, NotReplicated)
	float mTimeToReachValue = 20.0f;

	/** Only Default after once Saved it will not apply */
	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadOnly)
	float mValue = 0;

	UPROPERTY(SaveGame, BlueprintReadOnly, NotReplicated)
	float mCurveTime = 0;

	UPROPERTY(EditDefaultsOnly, SaveGame, BlueprintReadOnly)
	float mTargetValue = 0;

	UPROPERTY(SaveGame, BlueprintReadOnly, NotReplicated)
	float mOldValue = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, NotReplicated)
	UCurveFloat* mCurve = nullptr;

	bool Tick(float dt)
	{
		if (mValue != mTargetValue)
		{
			mCurveTime += dt;
			mCurveTime = FMath::Clamp(mCurveTime, 0.01f, mTimeToReachValue);
			mValue = FMath::Clamp(
				mOldValue + ((mTargetValue - mOldValue) * mCurve->GetFloatValue(mCurveTime / mTimeToReachValue)), mMin,
				mMax);
			return true;
		}
		return false;
	}

	void SetNewTarget(float Target)
	{
		mTargetValue = FMath::Clamp(Target, mMin, mMax);
		mOldValue = mValue;
		mCurveTime = 0;
	}
};

/**
 * 
 */
UCLASS()
class KLIB_API AKLBuildableSlugHatchingModule : public AKLBuildableSlugBuildingBase
{
	GENERATED_BODY()

public:
	AKLBuildableSlugHatchingModule()
		: mTier(0)
		  , mModuleType()
	{
	}

	// Repl
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void GetConditionalReplicatedProps(TArray<FFGCondReplicatedProperty>& outProps) const override;
	virtual bool CanUseFactoryClipboard_Implementation() override;

	virtual void BeginPlay() override;

	virtual bool CanProduce_Implementation() const override;
	virtual bool IsModuleTypeOf_Implementation(uint8 Type) override;
	virtual void Stacker_AddBuildingHeight_Implementation(float& Height) override;
	virtual void GetChildDismantleActors_Implementation(TArray<AActor*>& out_ChildDismantleActors) const override;
	virtual void HandlePower(float dt) override;
	virtual bool Factory_HasPower() const override;
	virtual bool GetHasPower() const override;

	virtual void Factory_Tick(float dt) override;

	virtual void HandleTemperatureModule(float dt);
	void UpdateTemperatureVisuals();
	virtual void HandleTankModule(float dt);
	virtual void HandleIncubatorModule(float dt);
	void UpdateIncubatorVisuals();
	virtual void HandleHumidityModule(float dt);
	virtual void HandleTimeModule(float dt);

	/** Set Target - can called from both sides Server and Client */
	UFUNCTION(BlueprintCallable, Category="KMods|Hatching")
	void ApplyNewHumidity(float Humidity);

	/** Set Target - can called from both sides Server and Client */
	UFUNCTION(BlueprintCallable, Category="KMods|Hatching")
	void ApplyNewTemperature(float Temperature);

	/** Set Target - can called from both sides Server and Client */
	UFUNCTION(BlueprintCallable, Category="KMods|Hatching")
	void ApplyNewOverwriteTime(EKAPISlugTime OverwriteTime);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="KMods|Hatching")
	uint8 mTier;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="KMods|Hatching")
	EHatchingModuleType mModuleType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="KMods|Hatching")
	float mBuildingHeight = 150.f;

	UPROPERTY(EditDefaultsOnly, SaveGame, BlueprintReadOnly, Replicated, Category="KMods|Hatching")
	FKLHatchingModeleStats mTemperature;

	UPROPERTY(EditDefaultsOnly, SaveGame, BlueprintReadOnly, meta = ( FGReplicated ), Category="KMods|Hatching")
	FKLHatchingModeleStats mHumidity;

	UPROPERTY(EditDefaultsOnly, SaveGame, BlueprintReadOnly, meta = ( FGReplicated ), Category="KMods|Hatching")
	EKAPISlugTime mOverwriteTime = EKAPISlugTime::NONE;

	FSmartTimer mIncubatorTimer = FSmartTimer(0.5f, true);
	TSubclassOf<UFGItemDescriptor> mLastEgg;
};
