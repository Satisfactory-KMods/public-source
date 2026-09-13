// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once
#include "ApiStatics.h"
#include "CoreMinimal.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "ApiPostStruct.generated.h"

USTRUCT(BlueprintType)
struct SBS_API FApiPostStruct
{
	GENERATED_BODY()

	virtual ~FApiPostStruct() = default;

	static void MakeHeader(TMap<FString, FString>& Headers, UObject* WorldContext);

	FString ToString();

	virtual FString getUrl(UObject* WorldContext) { return FSBSStatics::MakeUrl("", WorldContext); }

protected:

	virtual void ToJson(TSharedPtr<FJsonObject>& JsonObject) {}
};

USTRUCT(BlueprintType)
struct SBS_API FDynamicApiPostStruct : public FApiPostStruct
{
	GENERATED_BODY()

	virtual ~FDynamicApiPostStruct() override = default;

	bool operator==(const FDynamicApiPostStruct& Other) const { return Indetifier == Other.Indetifier; }

	bool operator!=(const FDynamicApiPostStruct& Other) const { return Indetifier != Other.Indetifier; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Filter Post Struct")
	FText NotifyText = FText::GetEmpty();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Filter Post Struct")
	FString Indetifier = FString();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Filter Post Struct")
	FString FailedText = FString();

	virtual void OnRequestDone(FHttpRequestPtr Request, FHttpResponsePtr Response, class USBSApiSubsystem* Subsystem,
							   TSharedPtr<FJsonObject> JsonObject, UObject* WorldContext);
};

USTRUCT(BlueprintType)
struct SBS_API FAuthCheckPostStruct : public FDynamicApiPostStruct
{
	GENERATED_BODY()

	virtual FString getUrl(UObject* WorldContext) override
	{
		return FSBSStatics::MakeUrl(FSBSStatics::API_AUTH, WorldContext);
	};
};

USTRUCT(BlueprintType)
struct SBS_API FFilterPostStruct : public FApiPostStruct
{
	GENERATED_BODY()

	virtual ~FFilterPostStruct() override = default;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Filter Post Struct")
	int32 Limit = 20;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Filter Post Struct")
	int32 Skip = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Filter Post Struct")
	FString SortByKey = "";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Filter Post Struct")
	bool SortByUp = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Filter Post Struct")
	FString Name = "";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Filter Post Struct")
	TArray<FString> Tags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Filter Post Struct")
	TArray<FString> Mods;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Filter Post Struct")
	bool OnlyVanilla = false;

	virtual FString getUrl(UObject* WorldContext) override
	{
		return FSBSStatics::MakeUrl(FSBSStatics::API_BLUEPRINT, WorldContext);
	};

protected:
	virtual void ToJson(TSharedPtr<FJsonObject>& JsonObject) override;
};

USTRUCT(BlueprintType)
struct SBS_API FPackFilterPostStruct : public FFilterPostStruct
{
	GENERATED_BODY()

	virtual FString getUrl(UObject* WorldContext) override
	{
		return FSBSStatics::MakeUrl(FSBSStatics::API_BLUEPRINTPACKS, WorldContext);
	};
};

USTRUCT(BlueprintType)
struct SBS_API FRatingPostStruct : public FDynamicApiPostStruct
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rating Post Struct")
	FString BlueprintID = "";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rating Post Struct")
	int32 Rating = 1;

	virtual FString getUrl(UObject* WorldContext) override
	{
		return FSBSStatics::MakeUrl(FSBSStatics::API_RATEBP, WorldContext);
	};

protected:
	virtual void ToJson(TSharedPtr<FJsonObject>& JsonObject) override;
};

USTRUCT()
struct SBS_API FDownloadSbpStruct : public FApiPostStruct
{
	GENERATED_BODY()

	FString ID = "";

	virtual FString getUrl(UObject* WorldContext) override
	{
		return FSBSStatics::MakeUrl(FSBSStatics::API_BLUEPRINTDOWNLOAD + FSBSStatics::EncodePathSegment(ID) + "/sbp",
									WorldContext);
	};
};

USTRUCT()
struct SBS_API FDownloadSbpcfgStruct : public FDownloadSbpStruct
{
	GENERATED_BODY()

	virtual FString getUrl(UObject* WorldContext) override
	{
		return FSBSStatics::MakeUrl(FSBSStatics::API_BLUEPRINTDOWNLOAD + FSBSStatics::EncodePathSegment(ID) + "/sbpcfg",
									WorldContext);
	};
};

USTRUCT(BlueprintType)
struct SBS_API FSBSUserData : public FDynamicApiPostStruct
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString ID = "";

	UPROPERTY(BlueprintReadOnly)
	FString Username = "";

	UPROPERTY(BlueprintReadOnly, Category = "SBS|Account")
	TArray<FString> Permissions;

	virtual FString getUrl(UObject* WorldContext) override
	{
		return FSBSStatics::MakeUrl(FSBSStatics::API_AUTH, WorldContext);
	};
};

USTRUCT(BlueprintType)
struct SBS_API FGetTagsStruct : public FDynamicApiPostStruct
{
	GENERATED_BODY()

	virtual FString getUrl(UObject* WorldContext) override
	{
		return FSBSStatics::MakeUrl(FSBSStatics::API_TAGS, WorldContext);
	};

	virtual void OnRequestDone(FHttpRequestPtr Request, FHttpResponsePtr Response, class USBSApiSubsystem* Subsystem,
							   TSharedPtr<FJsonObject> JsonObject, UObject* WorldContext) override;
};
