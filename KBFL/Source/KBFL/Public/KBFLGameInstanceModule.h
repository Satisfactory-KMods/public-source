

#pragma once

#include "CoreMinimal.h"
#include "Module/GameInstanceModule.h"
#include "UObject/Object.h"

#include "KBFLGameInstanceModule.generated.h"

UCLASS()
class KBFL_API UKBFLGameInstanceModule : public UGameInstanceModule
{
	GENERATED_BODY()

public:
	UKBFLGameInstanceModule();

	virtual void DispatchLifecycleEvent(ELifecyclePhase Phase) override;

	bool IsOwnerModObject(UObject* Object) const;

	UFUNCTION(BlueprintNativeEvent, Category = "LifecyclePhase")
	void ConstructionPhase();

	UFUNCTION(BlueprintNativeEvent, Category = "LifecyclePhase")
	void InitPhase();

	UFUNCTION(BlueprintNativeEvent, Category = "LifecyclePhase")
	void PostInitPhase();

	UFUNCTION(BlueprintCallable, Category = "KMods|AssetRegistry")
	void ScanModConfigurations();

	UFUNCTION(BlueprintCallable, Category = "KMods|AssetRegistry")
	void ScanSessionSettings();

	UFUNCTION(BlueprintCallable, Category = "KMods|AssetRegistry")
	void ScanRemoteCallObjects();

private:

	template <typename T>
	void ScanModClassesInto(TArray<TSubclassOf<T>>& OutArray, const TCHAR* Label);

	template <typename T>
	void ScanModInstancesInto(TArray<TObjectPtr<T>>& OutArray, const TCHAR* Label);

public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|AssetRegistry")
	TArray<TSubclassOf<UObject>> mBlacklistedClasses;
};
