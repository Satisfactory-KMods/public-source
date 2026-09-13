// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Structures/ApiPostStruct.h"
#include "SBSSharingTypes.generated.h"

UENUM(BlueprintType)
enum class ESBSLoginState : uint8
{
	SignedOut,
	SigningIn,
	SignedIn
};

USTRUCT(BlueprintType)
struct SBS_API FSBSOperationResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "SBS")
	bool bSuccess = false;
	UPROPERTY(BlueprintReadOnly, Category = "SBS")
	FString Code;
	UPROPERTY(BlueprintReadOnly, Category = "SBS")
	FText Message;
	UPROPERTY(BlueprintReadOnly, Category = "SBS")
	int32 HttpStatus = 0;
	UPROPERTY(BlueprintReadOnly, Category = "SBS")
	int32 RetryAfterSeconds = 0;

	static FSBSOperationResult Make(const TCHAR* Code, const FText& Message, bool bSuccess = false, int32 Status = 0);
};

USTRUCT(BlueprintType)
struct SBS_API FSBSSession
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "SBS")
	ESBSLoginState State = ESBSLoginState::SignedOut;
	UPROPERTY(BlueprintReadOnly, Category = "SBS")
	FSBSUserData User;
	UPROPERTY(BlueprintReadOnly, Category = "SBS")
	bool bCanPublish = false;
	UPROPERTY(BlueprintReadOnly, Category = "SBS")
	bool bRemembered = false;
};

USTRUCT(BlueprintType)
struct SBS_API FSBSBlueprintUpload
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SBS")
	FString LocalBlueprintName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SBS")
	FString Name;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SBS")
	FString Description;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SBS")
	TArray<FString> TagIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SBS")
	bool bPublishPublicly = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SBS")
	FGuid RequestId;
};

USTRUCT(BlueprintType)
struct SBS_API FSBSLocalBlueprint
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "SBS")
	FString LocalBlueprintName;
	UPROPERTY(BlueprintReadOnly, Category = "SBS")
	int64 BlueprintBytes = 0;
	UPROPERTY(BlueprintReadOnly, Category = "SBS")
	int64 ConfigBytes = 0;
};

USTRUCT(BlueprintType)
struct SBS_API FSBSBlueprintUploadResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "SBS")
	FGuid RequestId;
	UPROPERTY(BlueprintReadOnly, Category = "SBS")
	FSBSOperationResult Result;
	UPROPERTY(BlueprintReadOnly, Category = "SBS")
	FString BlueprintId;
	UPROPERTY(BlueprintReadOnly, Category = "SBS")
	FString PublicUrl;
};
