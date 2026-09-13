// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "ApiJsonStruct.generated.h"

USTRUCT(BlueprintType)
struct SBS_API FApiJsonStruct
{
	GENERATED_BODY()

	virtual ~FApiJsonStruct() = default;

	TSharedPtr<FJsonObject> mJsonObject;

	void setJsonObject(TSharedPtr<FJsonObject> Object) { mJsonObject = MoveTemp(Object); }

	virtual void parse(UObject* WorldContext) {}
};

USTRUCT(BlueprintType)
struct SBS_API FBlueprintJsonTagStructure : public FApiJsonStruct
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "BlueprintJsonStructure")
	FString ID;

	UPROPERTY(BlueprintReadOnly, Category = "BlueprintJsonStructure")
	FString DisplayName;

	virtual void parse(UObject* WorldContext) override;
};

USTRUCT(BlueprintType)
struct SBS_API FBlueprintJsonColorStructure : public FApiJsonStruct
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "BlueprintJsonStructure")
	int32 IconID = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BlueprintJsonStructure")
	FLinearColor Color = FLinearColor::White;

	virtual void parse(UObject* WorldContext) override;
};

USTRUCT(BlueprintType)
struct SBS_API FBlueprintJsonStructure : public FApiJsonStruct
{
	GENERATED_BODY()

	bool operator==(const FBlueprintJsonStructure& Other) const { return ID == Other.ID; }

	bool operator!=(const FBlueprintJsonStructure& Other) const { return ID != Other.ID; }

	UPROPERTY(BlueprintReadOnly, Category = "BlueprintJsonStructure")
	FString ID;

	UPROPERTY(BlueprintReadOnly, Category = "BlueprintJsonStructure")
	TArray<FString> Mods;

	UPROPERTY(BlueprintReadOnly, Category = "BlueprintJsonStructure")
	TArray<FBlueprintJsonTagStructure> Tags;

	UPROPERTY(BlueprintReadOnly, Category = "BlueprintJsonStructure")
	FString Owner;

	UPROPERTY(BlueprintReadOnly, Category = "BlueprintJsonStructure")
	FString DesignerSize;

	UPROPERTY(BlueprintReadOnly, Category = "BlueprintJsonStructure")
	FString Name;

	UPROPERTY(BlueprintReadOnly, Category = "BlueprintJsonStructure")
	FString CreatedAt;

	UPROPERTY(BlueprintReadOnly, Category = "BlueprintJsonStructure")
	FString UpdatedAt;

	UPROPERTY(BlueprintReadOnly, Category = "BlueprintJsonStructure")
	int32 Downloads = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BlueprintJsonStructure")
	int32 TotalRating = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BlueprintJsonStructure")
	int32 TotalRatingCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BlueprintJsonStructure")
	TArray<FString> Images;

	UPROPERTY(BlueprintReadOnly, Category = "BlueprintJsonStructure")
	FString OriginalName;

	UPROPERTY(BlueprintReadOnly, Category = "BlueprintJsonStructure")
	FString SCIM;

	UPROPERTY(BlueprintReadOnly, Category = "BlueprintJsonStructure")
	FBlueprintJsonColorStructure IconData;

	virtual void parse(UObject* WorldContext) override;
};

USTRUCT(BlueprintType)
struct SBS_API FBlueprintInPackJsonStructure : public FApiJsonStruct
{
	GENERATED_BODY()

	bool operator==(const FBlueprintJsonStructure& Other) const { return ID == Other.ID; }

	bool operator!=(const FBlueprintJsonStructure& Other) const { return ID != Other.ID; }

	UPROPERTY(BlueprintReadOnly, Category = "BlueprintJsonStructure")
	FString ID;

	UPROPERTY(BlueprintReadOnly, Category = "BlueprintJsonStructure")
	FString Name;

	UPROPERTY(BlueprintReadOnly, Category = "BlueprintJsonStructure")
	FString OriginalName;

	UPROPERTY(BlueprintReadOnly, Category = "BlueprintJsonStructure")
	FBlueprintJsonColorStructure IconData;

	virtual void parse(UObject* WorldContext) override;
};

USTRUCT(BlueprintType)
struct SBS_API FBlueprintPackJsonStructure : public FApiJsonStruct
{
	GENERATED_BODY()

	bool operator==(const FBlueprintJsonStructure& Other) const { return ID == Other.ID; }

	bool operator!=(const FBlueprintJsonStructure& Other) const { return ID != Other.ID; }

	UPROPERTY(BlueprintReadOnly, Category = "BlueprintJsonStructure")
	FString ID;

	UPROPERTY(BlueprintReadOnly, Category = "BlueprintJsonStructure")
	TArray<FString> Mods;

	UPROPERTY(BlueprintReadOnly, Category = "BlueprintJsonStructure")
	TArray<FBlueprintJsonTagStructure> Tags;

	UPROPERTY(BlueprintReadOnly, Category = "BlueprintJsonStructure")
	FString Owner;

	UPROPERTY(BlueprintReadOnly, Category = "BlueprintJsonStructure")
	FString Name;

	UPROPERTY(BlueprintReadOnly, Category = "BlueprintJsonStructure")
	FString CreatedAt;

	UPROPERTY(BlueprintReadOnly, Category = "BlueprintJsonStructure")
	FString UpdatedAt;

	UPROPERTY(BlueprintReadOnly, Category = "BlueprintJsonStructure")
	int32 TotalRating = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BlueprintJsonStructure")
	int32 TotalRatingCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BlueprintJsonStructure")
	TArray<FBlueprintInPackJsonStructure> Blueprints;

	UPROPERTY(BlueprintReadOnly, Category = "BlueprintJsonStructure")
	FString ImageUrl;

	UPROPERTY(BlueprintReadOnly, Category = "BlueprintJsonStructure")
	FBlueprintJsonColorStructure FirstIconData;

	virtual void parse(UObject* WorldContext) override;
};
