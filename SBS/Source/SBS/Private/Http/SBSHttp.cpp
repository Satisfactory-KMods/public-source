// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#include "Http/SBSHttp.h"

#include "HttpModule.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Sharing/SBSSharingProtocol.h"
#include "Structures/ApiPostStruct.h"

void FSBSResponseBuffer::Serialize(void* Data, int64 Length)
{
	if (IsError() || Length < 0 || Length > mLimit - mData.Num() || (Length > 0 && !Data))
	{
		SetError();
		return;
	}
	if (Length > 0)
	{
		mData.Append(static_cast<const uint8*>(Data), static_cast<int32>(Length));
	}
}

TSharedPtr<FSBSRequest> FSBSRequest::Create(const FString& Url, const FString& Verb, const FString& Payload,
											UObject* WorldContext, int32 MaxBytes, float Timeout,
											bool bUseLegacyHeaders)
{
	check(IsInGameThread());
	const TSharedRef<FSBSRequest> State = MakeShared<FSBSRequest>();
	State->mBody = MakeShared<FSBSResponseBuffer, ESPMode::ThreadSafe>(MaxBytes);
	State->mRequest = FHttpModule::Get().CreateRequest();
	State->mRequest->SetURL(Url);
	State->mRequest->SetVerb(Verb);
	State->mRequest->SetTimeout(Timeout);
	State->mRequest->SetDelegateThreadPolicy(EHttpRequestDelegateThreadPolicy::CompleteOnGameThread);
	State->bStreamReady = State->mRequest->SetResponseBodyReceiveStream(State->mBody.ToSharedRef());
	TMap<FString, FString> Headers;
	if (bUseLegacyHeaders)
	{
		FApiPostStruct::MakeHeader(Headers, WorldContext);
	}
	else
	{
		Headers.Add(TEXT("User-Agent"), TEXT("SBS/Satisfactory-1.2"));
		Headers.Add(TEXT("Content-Type"), TEXT("application/json"));
		Headers.Add(TEXT("Accept"), TEXT("application/json"));
	}

	if (Headers.Contains(TEXT("Authorization")) &&
		!FSBSSharingProtocol::IsCredentialUrl(Url, FSBSStatics::MakeUrl(TEXT(""), WorldContext)))
	{
		Headers.Remove(TEXT("Authorization"));
	}
	for (const TPair<FString, FString>& Header : Headers)
	{
		if (!Header.Value.IsEmpty() && Header.Value.Len() <= 4096 && !Header.Value.Contains(TEXT("\r")) &&
			!Header.Value.Contains(TEXT("\n")))
		{
			State->mRequest->SetHeader(Header.Key, Header.Value);
		}
	}
	if (Verb != TEXT("GET"))
	{
		State->mRequest->SetContentAsString(Payload);
	}
	return State;
}

bool FSBSRequest::Start() { return bStreamReady && mRequest->ProcessRequest(); }

void FSBSRequest::Cancel()
{
	if (mRequest)
	{
		mRequest->OnProcessRequestComplete().Unbind();
		mRequest->OnRequestProgress64().Unbind();
		mRequest->CancelRequest();
	}
}

bool FSBSRequest::ReadJson(FHttpResponsePtr Response, bool bSuccess, TSharedPtr<FJsonObject>& Json) const
{
	Json.Reset();
	if (!bSuccess || !Response || !EHttpResponseCodes::IsOk(Response->GetResponseCode()) || mBody->IsError() ||
		mBody->mData.IsEmpty())
	{
		return false;
	}
	const FUTF8ToTCHAR Text(reinterpret_cast<const ANSICHAR*>(mBody->mData.GetData()), mBody->mData.Num());
	return FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(FString(Text.Length(), Text.Get())), Json) &&
		Json.IsValid() && !Json->HasField(TEXT("error"));
}
