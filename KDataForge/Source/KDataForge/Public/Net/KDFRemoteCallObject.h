#pragma once

#include "CoreMinimal.h"

#include "FGRemoteCallObject.h"

#include "KDFRemoteCallObject.generated.h"

/**
 * Phase 6: remote call object for editor-originated edits in multiplayer sessions.
 * Host-authoritative: the server applies the op through the standard pipeline; clients other than
 * the host are rejected (permission widening is a later phase). Registered via the game instance
 * module's RemoteCallObjects list.
 */
UCLASS()
class KDATAFORGE_API UKDFRemoteCallObject : public UFGRemoteCallObject
{
	GENERATED_BODY()

public:
	static UKDFRemoteCallObject* Get(UWorld* World);

	/** Applies `set` of ValueText to TargetPath.PropertyPath on the server. */
	UFUNCTION(Server, WithValidation, Reliable, BlueprintCallable, Category = "KDataForge")
	void Server_ApplySetOp(const FString& TargetPath, const FString& PropertyPath, const FString& ValueText);

private:
	/** Dummy replicated property — UFGRemoteCallObject requires at least one for registration. */
	UPROPERTY(Replicated)
	bool mDummy = false;

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
