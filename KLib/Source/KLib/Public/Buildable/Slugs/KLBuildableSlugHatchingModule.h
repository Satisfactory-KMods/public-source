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
	TObjectPtr<UCurveFloat> mCurve = nullptr;

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

UCLASS()
class KLIB_API AKLBuildableSlugHatchingModule : public AKLBuildableSlugBuildingBase
{
	GENERATED_BODY()

public:
	AKLBuildableSlugHatchingModule() : mModuleType(), mTier(0) {}

	//~ Begin AFGBuildableFactory Interface
	virtual bool CanProduce_Implementation() const override;
	virtual bool CanUseFactoryClipboard_Implementation() override;
	virtual bool Factory_HasPower() const override;
	virtual void Factory_Tick(float dt) override;
	virtual void GetChildDismantleActors_Implementation(TArray<AActor*>& out_ChildDismantleActors) const override;
	virtual void GetConditionalReplicatedProps(TArray<FFGCondReplicatedProperty>& outProps) const override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ End AFGBuildableFactory Interface

	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	//~ End AActor Interface

	//~ Begin AKLBuildableSlugBuildingBase Interface
	virtual void HandlePower(float dt) override;
	//~ End AKLBuildableSlugBuildingBase Interface

	//~ Begin IFGStackingActorInterface Interface
	virtual void Stacker_AddBuildingHeight_Implementation(float& Height) override;
	//~ End IFGStackingActorInterface Interface

	virtual bool GetHasPower() const override;
	virtual bool IsModuleTypeOf_Implementation(uint8 Type) override;

	UFUNCTION(BlueprintCallable, Category = "KMods|Hatching")
	void ApplyNewHumidity(float Humidity);

	UFUNCTION(BlueprintCallable, Category = "KMods|Hatching")
	void ApplyNewOverwriteTime(EKAPISlugTime OverwriteTime);

	UFUNCTION(BlueprintCallable, Category = "KMods|Hatching")
	void ApplyNewTemperature(float Temperature);

	void HandleHumidityModule(float dt);
	void HandleIncubatorModule(float dt);
	void HandleTankModule(float dt);
	void HandleTemperatureModule(float dt);
	void HandleTimeModule(float dt);
	void UpdateIncubatorVisuals();
	void UpdateTemperatureVisuals();

	/** Centralised dirty-marking for the replicated stat properties. */
	void CommitTemperature();
	void CommitHumidity();
	void CommitOverwriteTime();

	FSmartTimer mIncubatorTimer = FSmartTimer(0.5f, true);
	TSubclassOf<UFGItemDescriptor> mLastEgg;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|Hatching")
	float mBuildingHeight = 150.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|Hatching")
	EHatchingModuleType mModuleType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|Hatching")
	uint8 mTier;

	UPROPERTY(EditDefaultsOnly, SaveGame, BlueprintReadOnly, meta = (FGReplicated), Category = "KMods|Hatching")
	EKAPISlugTime mOverwriteTime = EKAPISlugTime::NONE;

	UPROPERTY(EditDefaultsOnly, SaveGame, BlueprintReadOnly, meta = (FGReplicated), Category = "KMods|Hatching")
	FKLHatchingModeleStats mHumidity;

	UPROPERTY(EditDefaultsOnly, SaveGame, BlueprintReadOnly, meta = (FGReplicated), Category = "KMods|Hatching")
	FKLHatchingModeleStats mTemperature;
};
