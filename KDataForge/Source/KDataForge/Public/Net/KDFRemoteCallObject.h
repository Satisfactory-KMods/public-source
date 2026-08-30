#pragma once

#include "CoreMinimal.h"

#include "FGRemoteCallObject.h"

#include "KDFRemoteCallObject.generated.h"

UCLASS()
class KDATAFORGE_API UKDFRemoteCallObject : public UFGRemoteCallObject
{
	GENERATED_BODY()

public:
	static UKDFRemoteCallObject* Get(UWorld* World);

	UFUNCTION(Server, WithValidation, Reliable, BlueprintCallable, Category = "KDataForge")
	void Server_ApplySetOp(const FString& TargetPath, const FString& PropertyPath, const FString& ValueText);

private:

	UPROPERTY(Replicated)
	bool mDummy = false;

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
