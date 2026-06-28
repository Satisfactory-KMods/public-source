// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Module/GameInstanceModule.h"
#include "UObject/Object.h"

#include "KBFLGameInstanceModule.generated.h"

/**
 *
 */
UCLASS()
class KBFL_API UKBFLGameInstanceModule : public UGameInstanceModule
{
	GENERATED_BODY()

public:
	UKBFLGameInstanceModule();

	// BEGIN UGameInstanceModule
	virtual void DispatchLifecycleEvent(ELifecyclePhase Phase) override;
	// END UGameInstanceModule

	bool IsOwnerModObject(UObject* Object) const;

	UFUNCTION(BlueprintNativeEvent, Category = "LifecyclePhase")
	void ConstructionPhase();

	UFUNCTION(BlueprintNativeEvent, Category = "LifecyclePhase")
	void InitPhase();

	UFUNCTION(BlueprintNativeEvent, Category = "LifecyclePhase")
	void PostInitPhase();

	/** Editor button: scan this mod's plugin path and AddUnique all mod configurations into ModConfigurations. */
	UFUNCTION(BlueprintCallable, Category = "KMods|AssetRegistry")
	void ScanModConfigurations();

	/** Editor button: scan this mod's plugin path and AddUnique all session settings into SessionSettings. */
	UFUNCTION(BlueprintCallable, Category = "KMods|AssetRegistry")
	void ScanSessionSettings();

	/** Editor button: scan this mod's plugin path and AddUnique all remote call objects into RemoteCallObjects. */
	UFUNCTION(BlueprintCallable, Category = "KMods|AssetRegistry")
	void ScanRemoteCallObjects();

private:
	// Class scan: BP classes derived from T in this mod's plugin path -> AddUnique TSubclassOf.
	template <typename T>
	void ScanModClassesInto(TArray<TSubclassOf<T>>& OutArray, const TCHAR* Label);

	// Instance scan: data-asset instances of T in this mod's plugin path -> AddUnique TObjectPtr.
	template <typename T>
	void ScanModInstancesInto(TArray<TObjectPtr<T>>& OutArray, const TCHAR* Label);

public:
	/** Classes excluded from the editor scan buttons. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|AssetRegistry")
	TArray<TSubclassOf<UObject>> mBlacklistedClasses;
};
