// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/Archive.h"

class SBS_API FSBSResponseBuffer final : public FArchive
{
public:
	explicit FSBSResponseBuffer(int32 Limit) : mLimit(Limit) {}
	virtual void Serialize(void* Data, int64 Length) override;
	TArray<uint8> mData;

private:
	int32 mLimit;
};

struct SBS_API FSBSRequest
{
	FHttpRequestPtr mRequest;
	TSharedPtr<FSBSResponseBuffer, ESPMode::ThreadSafe> mBody;
	bool bStreamReady = false;

	static TSharedPtr<FSBSRequest> Create(const FString& Url, const FString& Verb, const FString& Payload,
										  UObject* WorldContext, int32 MaxBytes = 4 * 1024 * 1024,
										  float Timeout = 20.0f, bool bUseLegacyHeaders = true);
	bool Start();
	void Cancel();
	bool ReadJson(FHttpResponsePtr Response, bool bSuccess, TSharedPtr<FJsonObject>& Json) const;
};
