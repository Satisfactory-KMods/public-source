// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Interfaces/IHttpRequest.h"
#include "Kismet/BlueprintAsyncActionBase.h"

#include "EnumStruc/RssStruc.h"

#include "RssApiClient.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSignDesignFetched, bool, bSuccess, const FRssSignData&, SignData);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSignDesignListFetched, bool, bSuccess, const TArray<FRssSignData>&,
											 Designs);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnApiError, int32, ErrorCode, const FString&, ErrorMessage);

UCLASS()
class RSS_API URssApiClient : public UObject
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintPure, Category = "RSS API", meta = (WorldContext = "WorldContextObject"))
	static URssApiClient* Get(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "RSS API")
	void SetBaseUrl(const FString& NewBaseUrl);

	UFUNCTION(BlueprintCallable, Category = "RSS API")
	void SetBearerToken(const FString& NewToken);

	UFUNCTION(BlueprintCallable, Category = "RSS API", meta = (WorldContext = "WorldContextObject"))
	void FetchOwnDesigns(UObject* WorldContextObject, int32 Limit = 50, int32 Offset = 0);

	UFUNCTION(BlueprintCallable, Category = "RSS API", meta = (WorldContext = "WorldContextObject"))
	void FetchDesignById(UObject* WorldContextObject, const FString& DesignId);

	UFUNCTION(BlueprintCallable, Category = "RSS API", meta = (WorldContext = "WorldContextObject"))
	void FetchPublicDesigns(UObject* WorldContextObject, int32 Limit = 50, int32 Offset = 0);

	UFUNCTION(BlueprintCallable, Category = "RSS API", meta = (WorldContext = "WorldContextObject"))
	void FetchDesignLimits(UObject* WorldContextObject);

	UPROPERTY(BlueprintAssignable, Category = "RSS API")
	FOnSignDesignFetched OnSignDesignFetched;

	UPROPERTY(BlueprintAssignable, Category = "RSS API")
	FOnSignDesignListFetched OnSignDesignListFetched;

	UPROPERTY(BlueprintAssignable, Category = "RSS API")
	FOnApiError OnApiError;

private:

	void OnFetchOwnDesignsResponse(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded);
	void OnFetchDesignByIdResponse(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded);
	void OnFetchPublicDesignsResponse(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded);
	void OnFetchDesignLimitsResponse(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded);

	void MakeAuthenticatedRequest(const FString& Endpoint, const FString& QueryParams = FString());

	UPROPERTY()
	FString BaseUrl = TEXT("https://k-mods.com/api/v2/rss");

	UPROPERTY()
	FString BearerToken;

	TMap<FHttpRequestPtr, FString> PendingRequests;

	static TMap<TWeakObjectPtr<UWorld>, TWeakObjectPtr<URssApiClient>> Instances;
};

UCLASS()
class RSS_API URssApiFetchOwnDesignsAction : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "RSS API",
			  meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"))
	static URssApiFetchOwnDesignsAction* FetchOwnDesigns(UObject* WorldContextObject, int32 Limit = 50,
														 int32 Offset = 0);

	virtual void Activate() override;

	UPROPERTY(BlueprintAssignable, Category = "RSS API")
	FOnSignDesignListFetched OnSuccess;

	UPROPERTY(BlueprintAssignable, Category = "RSS API")
	FOnApiError OnFailed;

private:
	UPROPERTY()
	URssApiClient* ApiClient;

	int32 LimitParam;
	int32 OffsetParam;

	UFUNCTION()
	void OnListFetched(bool bSuccess, const TArray<FRssSignData>& Designs);

	UFUNCTION()
	void OnError(int32 ErrorCode, const FString& ErrorMessage);
};

UCLASS()
class RSS_API URssApiFetchDesignByIdAction : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "RSS API",
			  meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"))
	static URssApiFetchDesignByIdAction* FetchDesignById(UObject* WorldContextObject, const FString& DesignId);

	virtual void Activate() override;

	UPROPERTY(BlueprintAssignable, Category = "RSS API")
	FOnSignDesignFetched OnSuccess;

	UPROPERTY(BlueprintAssignable, Category = "RSS API")
	FOnApiError OnFailed;

private:
	UPROPERTY()
	URssApiClient* ApiClient;

	FString DesignId;

	UFUNCTION()
	void OnDesignFetched(bool bSuccess, const FRssSignData& SignData);

	UFUNCTION()
	void OnError(int32 ErrorCode, const FString& ErrorMessage);
};

UCLASS()
class RSS_API URssApiFetchPublicDesignsAction : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "RSS API",
			  meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"))
	static URssApiFetchPublicDesignsAction* FetchPublicDesigns(UObject* WorldContextObject, int32 Limit = 50,
															   int32 Offset = 0);

	virtual void Activate() override;

	UPROPERTY(BlueprintAssignable, Category = "RSS API")
	FOnSignDesignListFetched OnSuccess;

	UPROPERTY(BlueprintAssignable, Category = "RSS API")
	FOnApiError OnFailed;

private:
	UPROPERTY()
	URssApiClient* ApiClient;

	int32 LimitParam;
	int32 OffsetParam;

	UFUNCTION()
	void OnListFetched(bool bSuccess, const TArray<FRssSignData>& Designs);

	UFUNCTION()
	void OnError(int32 ErrorCode, const FString& ErrorMessage);
};

UCLASS()
class RSS_API URssApiFetchDesignLimitsAction : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "RSS API",
			  meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"))
	static URssApiFetchDesignLimitsAction* FetchDesignLimits(UObject* WorldContextObject);

	virtual void Activate() override;

	UPROPERTY(BlueprintAssignable, Category = "RSS API")
	FOnApiError OnSuccess;

	UPROPERTY(BlueprintAssignable, Category = "RSS API")
	FOnApiError OnFailed;

private:
	UPROPERTY()
	URssApiClient* ApiClient;

	UFUNCTION()
	void OnError(int32 ErrorCode, const FString& ErrorMessage);
};
