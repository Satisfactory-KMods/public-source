// ILikeBanas

#pragma once
#include "Configuration/ModConfiguration.h"
#include "Interfaces/IHttpRequest.h"

#include "KPCLPatreonSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPatreonBenefitHasChanged, bool, NewPatreonState);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FKPCLOnSettingUpdated);

UCLASS(Blueprintable, BlueprintType)
class KPRIVATECODELIB_API UKPCLPatreonSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

protected:
	//~ Begin Tickable Object Interface
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual UWorld* GetTickableGameObjectWorld() const override;
	virtual TStatId GetStatId() const override;
	//~ End Tickable Object Interface

	//~ Begin UObject Interface
	virtual UWorld* GetWorld() const override;
	//~ End UObject Interface

	UKPCLPatreonSubsystem();

public:
	static UKPCLPatreonSubsystem* Get(UObject* WorldContext);

	UFUNCTION()
	void Query();

	UFUNCTION()
	void OnSettingChanged();

	static void QueryApi(FHttpRequestRef& Request, FString QueryName, TArray<FString> Parameter = {},
	                     bool Execute = true, bool IsPost = false);
	static bool ParseApiQuery(FHttpResponsePtr Response, TSharedPtr<FJsonObject>& Json);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContext"))
	static void StartReQueryPatreon(UObject* WorldContext);

	UPROPERTY(BlueprintAssignable)
	FOnPatreonBenefitHasChanged OnPatreonBenefitHasChanged;

	UPROPERTY(BlueprintAssignable)
	FKPCLOnSettingUpdated OnSettingsUpdated;

	UFUNCTION(BlueprintPure)
	bool ActivePatreonList() const { return bActivePatreonList; };

	UFUNCTION(BlueprintPure)
	bool ActivePatreonButton() const { return bActivePatreonButton; };

	UFUNCTION(BlueprintPure)
	bool ActiveNewsFeed() const { return bActiveNewsFeed; };

private:
	bool bPatreonIsActive = false;
	bool bOldState = false;

	bool bActivePatreonList = false;
	bool bActivePatreonButton = false;
	bool bActiveNewsFeed = false;

	FString mCode = "";
	FString mDiscordID = "";

	TSharedPtr<FJsonObject> mJsonObject;

	TSubclassOf<UModConfiguration> mConfig;

	bool bValid = false;
};
