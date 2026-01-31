// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "KPCLJsonObject.generated.h"

UENUM(BlueprintType)
enum class EHttpRequest : uint8
{
	GET = 0 UMETA(DisplayName = "GET"),
	POST = 1 UMETA(DisplayName = "POST"),
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHttpRequestComplete, bool, Success);

/**
 * 
 */
UCLASS(Blueprintable, meta=(AutoJSON=true))
class KPRIVATECODELIB_API UKPCLJsonObject : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Create Json Object"), Category = "KMods|Json")
	static UKPCLJsonObject* Create();

	// Native Helper
	static UKPCLJsonObject* CreateFromJson(TSharedPtr<FJsonObject> JsonObject);

	UFUNCTION(BlueprintCallable, Category = "KMods|Json")
	void CreateJsonFromUrl(FString Url, TMap<FString, FString> Headers, EHttpRequest Method, FString PostContent);

	UFUNCTION(BlueprintCallable, Category = "KMods|Json")
	bool TryRedoRequest();

	UFUNCTION(BlueprintGetter, Category = "KMods|Json")
	FORCEINLINE bool HasJsonObject() const { return mJsonObject.IsValid(); }

	UFUNCTION(BlueprintGetter, Category = "KMods|Json")
	FORCEINLINE bool WasLastRequestSuccessful() const { return bLastRequestWasSuccessful && !mRequestUrl.IsEmpty(); }

	UFUNCTION(BlueprintGetter, Category = "KMods|Json")
	FORCEINLINE bool GetLastRequestInformations(FString& Url, TMap<FString, FString>& Headers, EHttpRequest& Method,
	                                            FString& PostContent) const
	{
		Headers = mRequestHeaders;
		Url = mRequestUrl;
		Method = mRequestMethod;
		PostContent = mRequestContent;

		return WasLastRequestSuccessful();
	}

	UFUNCTION(BlueprintCallable, Category="KMods|Json")
	bool TryGetStringJson(FString StringField, FString& Result);

	UFUNCTION(BlueprintCallable, Category="KMods|Json")
	bool TryGetBool(FString StringField, bool& Result);

	UFUNCTION(BlueprintCallable, Category="KMods|Json")
	bool TryGetFloat(FString StringField, float& Result);

	UFUNCTION(BlueprintCallable, Category="KMods|Json")
	bool TryGetInt(FString StringField, int& Result);

	UFUNCTION(BlueprintCallable, Category="KMods|Json")
	bool TryGetStringArray(FString StringField, TArray<FString>& Result);

	UFUNCTION(BlueprintCallable, Category="KMods|Json")
	bool TryGetJsonArray(FString StringField, TArray<UKPCLJsonObject*>& Result);

	UFUNCTION(BlueprintCallable, Category="KMods|Json")
	bool TryGetJson(FString StringField, UKPCLJsonObject*& Result);

	UFUNCTION(BlueprintCallable, Category="KMods|Json")
	bool TryGetJsonObject(FString StringField, UKPCLJsonObject*& Result);

	UPROPERTY(BlueprintAssignable)
	FHttpRequestComplete OnHttpRequestComplete;

	FORCEINLINE void SetJson(TSharedPtr<FJsonObject> JsonObject) { mJsonObject = JsonObject; }
	FORCEINLINE TSharedPtr<FJsonObject> GetJson() { return mJsonObject; }

private:
	TSharedPtr<FJsonObject> mJsonObject;

	TMap<FString, FString> mRequestHeaders;
	FString mRequestUrl;
	FString mRequestContent;
	EHttpRequest mRequestMethod = EHttpRequest::GET;
	bool bLastRequestWasSuccessful = false;
};
