#include "Api/RssApiClient.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

#include "Engine/World.h"
#include "Serialization/RssJsonSerializer.h"

DECLARE_LOG_CATEGORY_EXTERN(RSSApiClientLog, Log, All)
DEFINE_LOG_CATEGORY(RSSApiClientLog)

namespace
{
	bool DeserializeSignDataObject(const TSharedPtr<FJsonObject>& SignDataObject, FRssSignData& OutSignData)
	{
		if (!SignDataObject.IsValid())
		{
			return false;
		}

		TSharedPtr<FJsonObject> RootObject = MakeShared<FJsonObject>();
		RootObject->SetNumberField(TEXT("schemaVersion"), 1);
		RootObject->SetObjectField(TEXT("signData"), SignDataObject);

		FString JsonString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
		return FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer) &&
			FRssJsonSerializer::JsonToSignData(JsonString, OutSignData);
	}
}

TMap<TWeakObjectPtr<UWorld>, TWeakObjectPtr<URssApiClient>> URssApiClient::Instances;

URssApiClient* URssApiClient::Get(UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	TWeakObjectPtr<UWorld> WorldKey(World);
	if (Instances.Contains(WorldKey) && Instances[WorldKey].IsValid())
	{
		return Instances[WorldKey].Get();
	}

	URssApiClient* NewClient = NewObject<URssApiClient>();
	Instances.Add(WorldKey, NewClient);
	return NewClient;
}

void URssApiClient::SetBaseUrl(const FString& NewBaseUrl)
{
	BaseUrl = NewBaseUrl;
	UE_LOG(RSSApiClientLog, Log, TEXT("API base URL set to: %s"), *BaseUrl);
}

void URssApiClient::SetBearerToken(const FString& NewToken)
{
	BearerToken = NewToken;
	if (!NewToken.IsEmpty())
	{
		UE_LOG(RSSApiClientLog, Log, TEXT("Bearer token configured"));
	}
	else
	{
		UE_LOG(RSSApiClientLog, Log, TEXT("Bearer token cleared"));
	}
}

void URssApiClient::FetchOwnDesigns(UObject* WorldContextObject, int32 Limit, int32 Offset)
{
	FString QueryParams = FString::Printf(TEXT("?limit=%d&offset=%d"), Limit, Offset);
	MakeAuthenticatedRequest(TEXT("/designs"), QueryParams);
}

void URssApiClient::FetchDesignById(UObject* WorldContextObject, const FString& DesignId)
{
	FString Endpoint = FString::Printf(TEXT("/designs/%s"), *DesignId);
	MakeAuthenticatedRequest(Endpoint);
}

void URssApiClient::FetchPublicDesigns(UObject* WorldContextObject, int32 Limit, int32 Offset)
{
	FString QueryParams = FString::Printf(TEXT("?limit=%d&offset=%d"), Limit, Offset);
	MakeAuthenticatedRequest(TEXT("/designs/public"), QueryParams);
}

void URssApiClient::FetchDesignLimits(UObject* WorldContextObject) { MakeAuthenticatedRequest(TEXT("/limits")); }

void URssApiClient::MakeAuthenticatedRequest(const FString& Endpoint, const FString& QueryParams)
{
	FHttpModule& HttpModule = FHttpModule::Get();
	FHttpRequestPtr Request = HttpModule.CreateRequest();

	FString Url = BaseUrl + Endpoint + QueryParams;
	Request->SetURL(Url);
	Request->SetVerb(TEXT("GET"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

	if (!BearerToken.IsEmpty())
	{
		FString AuthHeader = FString::Printf(TEXT("Bearer %s"), *BearerToken);
		Request->SetHeader(TEXT("Authorization"), AuthHeader);
	}

	UE_LOG(RSSApiClientLog, Log, TEXT("Making request to: %s"), *Url);

	if (Endpoint.Contains(TEXT("public")))
	{
		Request->OnProcessRequestComplete().BindUObject(this, &URssApiClient::OnFetchPublicDesignsResponse);
	}
	else if (Endpoint.Contains(TEXT("limits")))
	{
		Request->OnProcessRequestComplete().BindUObject(this, &URssApiClient::OnFetchDesignLimitsResponse);
	}
	else if (Endpoint.StartsWith(TEXT("/designs/")) && !Endpoint.EndsWith(TEXT("/public")))
	{

		Request->OnProcessRequestComplete().BindUObject(this, &URssApiClient::OnFetchDesignByIdResponse);
	}
	else
	{

		Request->OnProcessRequestComplete().BindUObject(this, &URssApiClient::OnFetchOwnDesignsResponse);
	}

	PendingRequests.Add(Request, Endpoint);
	Request->ProcessRequest();
}

void URssApiClient::OnFetchOwnDesignsResponse(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse,
											  bool bSucceeded)
{
	TArray<FRssSignData> Designs;

	if (bSucceeded && HttpResponse.IsValid() && EHttpResponseCodes::IsOk(HttpResponse->GetResponseCode()))
	{
		FString ResponseStr = HttpResponse->GetContentAsString();
		UE_LOG(RSSApiClientLog, Verbose, TEXT("OwnDesigns response: %s"), *ResponseStr);

		TSharedPtr<FJsonObject> RootObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseStr);
		if (FJsonSerializer::Deserialize(Reader, RootObject) && RootObject.IsValid())
		{
			const TArray<TSharedPtr<FJsonValue>>* DesignArray = nullptr;
			if (RootObject->TryGetArrayField(TEXT("data"), DesignArray) && DesignArray != nullptr)
			{
				for (const TSharedPtr<FJsonValue>& Value : *DesignArray)
				{
					if (Value.IsValid() && Value->Type == EJson::Object)
					{
						TSharedPtr<FJsonObject> DesignObj = Value->AsObject();
						const TSharedPtr<FJsonObject>* SignDataObj = nullptr;
						if (DesignObj.IsValid() && DesignObj->TryGetObjectField(TEXT("signData"), SignDataObj) &&
							SignDataObj != nullptr)
						{
							FRssSignData SignData;
							if (DeserializeSignDataObject(*SignDataObj, SignData))
							{
								Designs.Add(SignData);
							}
						}
					}
				}
			}
		}

		OnSignDesignListFetched.Broadcast(true, Designs);
		PendingRequests.Remove(HttpRequest);
		return;
	}

	int32 ErrorCode = HttpResponse.IsValid() ? HttpResponse->GetResponseCode() : 0;
	FString ErrorMsg = bSucceeded ? TEXT("Unknown error") : TEXT("Request failed");
	OnApiError.Broadcast(ErrorCode, ErrorMsg);
	OnSignDesignListFetched.Broadcast(false, Designs);
	PendingRequests.Remove(HttpRequest);
}

void URssApiClient::OnFetchDesignByIdResponse(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse,
											  bool bSucceeded)
{
	FRssSignData SignData;

	if (bSucceeded && HttpResponse.IsValid() && EHttpResponseCodes::IsOk(HttpResponse->GetResponseCode()))
	{
		FString ResponseStr = HttpResponse->GetContentAsString();
		UE_LOG(RSSApiClientLog, Verbose, TEXT("DesignById response: %s"), *ResponseStr);

		TSharedPtr<FJsonObject> RootObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseStr);
		if (FJsonSerializer::Deserialize(Reader, RootObject) && RootObject.IsValid())
		{
			const TSharedPtr<FJsonObject>* DataObj = nullptr;
			if (RootObject->TryGetObjectField(TEXT("data"), DataObj) && DataObj != nullptr)
			{
				const TSharedPtr<FJsonObject>* SignDataObj = nullptr;
				if ((*DataObj).IsValid() && (*DataObj)->TryGetObjectField(TEXT("signData"), SignDataObj) &&
					SignDataObj != nullptr)
				{
					if (DeserializeSignDataObject(*SignDataObj, SignData))
					{
						OnSignDesignFetched.Broadcast(true, SignData);
						PendingRequests.Remove(HttpRequest);
						return;
					}
				}
			}
		}
	}

	int32 ErrorCode = HttpResponse.IsValid() ? HttpResponse->GetResponseCode() : 0;
	FString ErrorMsg = bSucceeded ? TEXT("Failed to parse response") : TEXT("Request failed");
	OnApiError.Broadcast(ErrorCode, ErrorMsg);
	OnSignDesignFetched.Broadcast(false, SignData);
	PendingRequests.Remove(HttpRequest);
}

void URssApiClient::OnFetchPublicDesignsResponse(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse,
												 bool bSucceeded)
{
	TArray<FRssSignData> Designs;

	if (bSucceeded && HttpResponse.IsValid() && EHttpResponseCodes::IsOk(HttpResponse->GetResponseCode()))
	{
		FString ResponseStr = HttpResponse->GetContentAsString();
		UE_LOG(RSSApiClientLog, Verbose, TEXT("PublicDesigns response size: %d bytes"), ResponseStr.Len());

		TSharedPtr<FJsonObject> RootObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseStr);
		if (FJsonSerializer::Deserialize(Reader, RootObject) && RootObject.IsValid())
		{
			const TArray<TSharedPtr<FJsonValue>>* DesignArray = nullptr;
			if (RootObject->TryGetArrayField(TEXT("data"), DesignArray) && DesignArray != nullptr)
			{
				for (const TSharedPtr<FJsonValue>& Value : *DesignArray)
				{
					if (Value.IsValid() && Value->Type == EJson::Object)
					{
						TSharedPtr<FJsonObject> DesignObj = Value->AsObject();
						const TSharedPtr<FJsonObject>* SignDataObj = nullptr;
						if (DesignObj.IsValid() && DesignObj->TryGetObjectField(TEXT("signData"), SignDataObj) &&
							SignDataObj != nullptr)
						{
							FRssSignData SignData;
							if (DeserializeSignDataObject(*SignDataObj, SignData))
							{
								Designs.Add(SignData);
							}
						}
					}
				}
			}
		}

		OnSignDesignListFetched.Broadcast(true, Designs);
		PendingRequests.Remove(HttpRequest);
		return;
	}

	int32 ErrorCode = HttpResponse.IsValid() ? HttpResponse->GetResponseCode() : 0;
	FString ErrorMsg = bSucceeded ? TEXT("Unknown error") : TEXT("Request failed");
	OnApiError.Broadcast(ErrorCode, ErrorMsg);
	OnSignDesignListFetched.Broadcast(false, Designs);
	PendingRequests.Remove(HttpRequest);
}

void URssApiClient::OnFetchDesignLimitsResponse(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse,
												bool bSucceeded)
{
	if (bSucceeded && HttpResponse.IsValid() && EHttpResponseCodes::IsOk(HttpResponse->GetResponseCode()))
	{
		FString ResponseStr = HttpResponse->GetContentAsString();
		UE_LOG(RSSApiClientLog, Verbose, TEXT("DesignLimits response: %s"), *ResponseStr);

		TSharedPtr<FJsonObject> RootObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseStr);
		if (FJsonSerializer::Deserialize(Reader, RootObject) && RootObject.IsValid())
		{
			if (RootObject->HasField(TEXT("data")))
			{

				OnApiError.Broadcast(200, TEXT("Limits fetched"));
				PendingRequests.Remove(HttpRequest);
				return;
			}
		}
	}

	int32 ErrorCode = HttpResponse.IsValid() ? HttpResponse->GetResponseCode() : 0;
	FString ErrorMsg = bSucceeded ? TEXT("Failed to parse response") : TEXT("Request failed");
	OnApiError.Broadcast(ErrorCode, ErrorMsg);
	PendingRequests.Remove(HttpRequest);
}

URssApiFetchOwnDesignsAction* URssApiFetchOwnDesignsAction::FetchOwnDesigns(UObject* WorldContextObject, int32 Limit,
																			int32 Offset)
{
	URssApiFetchOwnDesignsAction* Action = NewObject<URssApiFetchOwnDesignsAction>();
	Action->ApiClient = URssApiClient::Get(WorldContextObject);
	Action->LimitParam = Limit;
	Action->OffsetParam = Offset;
	return Action;
}

void URssApiFetchOwnDesignsAction::Activate()
{
	if (!ApiClient)
	{
		OnFailed.Broadcast(400, TEXT("API client not available"));
		return;
	}

	ApiClient->OnSignDesignListFetched.AddDynamic(this, &URssApiFetchOwnDesignsAction::OnListFetched);
	ApiClient->OnApiError.AddDynamic(this, &URssApiFetchOwnDesignsAction::OnError);

	ApiClient->FetchOwnDesigns(ApiClient, LimitParam, OffsetParam);
}

void URssApiFetchOwnDesignsAction::OnListFetched(bool bSuccess, const TArray<FRssSignData>& Designs)
{
	if (bSuccess)
	{
		OnSuccess.Broadcast(true, Designs);
	}
	else
	{
		OnFailed.Broadcast(500, TEXT("Failed to fetch designs"));
	}
	SetReadyToDestroy();
}

void URssApiFetchOwnDesignsAction::OnError(int32 ErrorCode, const FString& ErrorMessage)
{
	OnFailed.Broadcast(ErrorCode, ErrorMessage);
	SetReadyToDestroy();
}

URssApiFetchDesignByIdAction* URssApiFetchDesignByIdAction::FetchDesignById(UObject* WorldContextObject,
																			const FString& DesignId)
{
	URssApiFetchDesignByIdAction* Action = NewObject<URssApiFetchDesignByIdAction>();
	Action->ApiClient = URssApiClient::Get(WorldContextObject);
	Action->DesignId = DesignId;
	return Action;
}

void URssApiFetchDesignByIdAction::Activate()
{
	if (!ApiClient)
	{
		OnFailed.Broadcast(400, TEXT("API client not available"));
		return;
	}

	ApiClient->OnSignDesignFetched.AddDynamic(this, &URssApiFetchDesignByIdAction::OnDesignFetched);
	ApiClient->OnApiError.AddDynamic(this, &URssApiFetchDesignByIdAction::OnError);

	ApiClient->FetchDesignById(ApiClient, DesignId);
}

void URssApiFetchDesignByIdAction::OnDesignFetched(bool bSuccess, const FRssSignData& SignData)
{
	if (bSuccess)
	{
		OnSuccess.Broadcast(true, SignData);
	}
	else
	{
		OnFailed.Broadcast(500, TEXT("Failed to fetch design"));
	}
	SetReadyToDestroy();
}

void URssApiFetchDesignByIdAction::OnError(int32 ErrorCode, const FString& ErrorMessage)
{
	OnFailed.Broadcast(ErrorCode, ErrorMessage);
	SetReadyToDestroy();
}

URssApiFetchPublicDesignsAction* URssApiFetchPublicDesignsAction::FetchPublicDesigns(UObject* WorldContextObject,
																					 int32 Limit, int32 Offset)
{
	URssApiFetchPublicDesignsAction* Action = NewObject<URssApiFetchPublicDesignsAction>();
	Action->ApiClient = URssApiClient::Get(WorldContextObject);
	Action->LimitParam = Limit;
	Action->OffsetParam = Offset;
	return Action;
}

void URssApiFetchPublicDesignsAction::Activate()
{
	if (!ApiClient)
	{
		OnFailed.Broadcast(400, TEXT("API client not available"));
		return;
	}

	ApiClient->OnSignDesignListFetched.AddDynamic(this, &URssApiFetchPublicDesignsAction::OnListFetched);
	ApiClient->OnApiError.AddDynamic(this, &URssApiFetchPublicDesignsAction::OnError);

	ApiClient->FetchPublicDesigns(ApiClient, LimitParam, OffsetParam);
}

void URssApiFetchPublicDesignsAction::OnListFetched(bool bSuccess, const TArray<FRssSignData>& Designs)
{
	if (bSuccess)
	{
		OnSuccess.Broadcast(true, Designs);
	}
	else
	{
		OnFailed.Broadcast(500, TEXT("Failed to fetch designs"));
	}
	SetReadyToDestroy();
}

void URssApiFetchPublicDesignsAction::OnError(int32 ErrorCode, const FString& ErrorMessage)
{
	OnFailed.Broadcast(ErrorCode, ErrorMessage);
	SetReadyToDestroy();
}

URssApiFetchDesignLimitsAction* URssApiFetchDesignLimitsAction::FetchDesignLimits(UObject* WorldContextObject)
{
	URssApiFetchDesignLimitsAction* Action = NewObject<URssApiFetchDesignLimitsAction>();
	Action->ApiClient = URssApiClient::Get(WorldContextObject);
	return Action;
}

void URssApiFetchDesignLimitsAction::Activate()
{
	if (!ApiClient)
	{
		OnFailed.Broadcast(400, TEXT("API client not available"));
		return;
	}

	ApiClient->OnApiError.AddDynamic(this, &URssApiFetchDesignLimitsAction::OnError);
	ApiClient->FetchDesignLimits(ApiClient);
}

void URssApiFetchDesignLimitsAction::OnError(int32 ErrorCode, const FString& ErrorMessage)
{
	if (ErrorCode == 200)
	{
		OnSuccess.Broadcast(200, TEXT("Limits fetched successfully"));
	}
	else
	{
		OnFailed.Broadcast(ErrorCode, ErrorMessage);
	}
	SetReadyToDestroy();
}
