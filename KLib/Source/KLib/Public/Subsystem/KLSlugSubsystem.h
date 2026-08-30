

#pragma once

#include "CoreMinimal.h"

#include "BFL/KBFL_Util.h"
#include "DataAssets/KAPISugHatchingData.h"
#include "FGSaveInterface.h"
#include "Structures/KPCLAsgHelper.h"
#include "Subsystem/KPCLModSubsystem.h"
#include "Subsystem/ModSubsystem.h"

#include "KLSlugSubsystem.generated.h"

UCLASS(Blueprintable, BlueprintType)
class KLIB_API AKLSlugSubsystem : public AKPCLModSubsystem
{
	GENERATED_BODY()

public:
	AKLSlugSubsystem();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool ShouldSave_Implementation() const override;

	UFUNCTION(BlueprintPure, Category = "SlugSubsystem", DisplayName = "GetSlugSubsystem",
			  meta = (DefaultToSelf = "WorldContext"))
	static AKLSlugSubsystem* Get(UObject* worldContext);

	UFUNCTION(BlueprintCallable, Category = "SlugSubsystem")
	void AddBreededEgg(TSubclassOf<UFGItemDescriptor> Egg);

	UFUNCTION(BlueprintCallable, Category = "SlugSubsystem")
	void AddBreededSlugs(TSubclassOf<UFGItemDescriptor> Slug);

	UFUNCTION(BlueprintPure, Category = "SlugSubsystem")
	TArray<TSubclassOf<UFGItemDescriptor>> GetAllBreededEgg() const;

	UFUNCTION(BlueprintPure, Category = "SlugSubsystem")
	TArray<TSubclassOf<UFGItemDescriptor>> GetAllBreededSlugs() const;

	UFUNCTION(BlueprintPure, Category = "SlugSubsystem")
	bool WasEggBreeded(TSubclassOf<UFGItemDescriptor> Egg) const;

	UFUNCTION(BlueprintPure, Category = "SlugSubsystem")
	bool WasSlugBreeded(TSubclassOf<UFGItemDescriptor> Slug) const;

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (AllowPrivateAccess = true))
	FKPCLAsgHelperBool mSkipUnlock;

	UPROPERTY(SaveGame, Replicated)
	TArray<TSubclassOf<UFGItemDescriptor>> mBreededEggs;

	UPROPERTY(SaveGame, Replicated)
	TArray<TSubclassOf<UFGItemDescriptor>> mBreededSlugs;
};
